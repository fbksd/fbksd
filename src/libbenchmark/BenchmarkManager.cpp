/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "BenchmarkManager.h"

#include "BenchmarkServer.h"
#include "RenderClient.h"
#include "exr_utils.h"
#include "tcp_utils.h"
#include "fbksd/renderer/samples.h"
using namespace fbksd;

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QEventLoop>
#include <QThread>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>


// ==========================================================
// Auxiliary free functions.
// ==========================================================
namespace
{

// Maximum number of tiles the renderer can work on in parallel.
// A tiles only become available once the client consumes it.
constexpr int64_t NUM_TILES = 20;

// Converts milliseconds to h:m:s:ms format
void convertMillisecons(int time, int* h, int* m, int* s, int* ms)
{
    *ms = time % 1000;
    *s = (time / 1000) % 60;
    *m = (time / 60000) % 60;
    *h = (time / 3600000);
}

void getResolution(const SceneInfo& info, int64_t *w, int64_t *h)
{
    *w = info.get<int64_t>("width");
    *h = info.get<int64_t>("height");
}

int64_t getPixelCount(const SceneInfo& info)
{
    int64_t w = 0;
    int64_t h = 0;
    getResolution(info, &w, &h);
    return w*h;
}

int64_t getInitSampleBudget(const SceneInfo& info)
{
    return info.get<int64_t>("max_spp") * getPixelCount(info);
}
}


// ==========================================================
// BenchmarkManager
// ==========================================================
BenchmarkManager::BenchmarkManager():
    m_tilesMemory("TILES_MEMORY"),
    m_resultMemory("RESULT_MEMORY")
{
    m_benchmarkServer = std::make_unique<BenchmarkServer>();
    m_benchmarkServer->onGetSceneInfo([this]()
        {return onGetSceneInfo();} );
    m_benchmarkServer->onSetParameters([this](const SampleLayout& layout)
        {onSetSampleLayout(layout);});
    m_benchmarkServer->onEvaluateSamples([this](bool isSpp, int64_t numSamples)
        {return onEvaluateSamples(isSpp, numSamples);});
    m_benchmarkServer->onGetNextTile([this](int64_t index)
        {return m_renderClient->getNextTile(index);});
    m_benchmarkServer->onEvaluateInputSamples([this](bool isSpp, int64_t numSamples)
        {return onEvaluateInputSamples(isSpp, numSamples);});
    m_benchmarkServer->onGetNextInputTile([this](int64_t index, bool wasInput)
        {return m_renderClient->getNextInputTile(index, wasInput);});
    m_benchmarkServer->onLastTileConsumed([this](int64_t index)
        {m_renderClient->lastTileConsumed(index);});
    m_benchmarkServer->onSendResult([this]()
        {onSendResult();});
}

BenchmarkManager::~BenchmarkManager() = default;

void BenchmarkManager::runPassive(int spp)
{
    m_passiveMode = true;
    m_benchmarkServer->run(/*2226*/);
    m_renderClient = std::make_unique<RenderClient>(2227);
    m_tileSize = m_renderClient->getTileSize();
    m_currentSceneInfo = m_renderClient->getSceneInfo();
    m_currentSceneInfo.set<int64_t>("max_spp", spp);
    m_currentSceneInfo.set<int64_t>("max_samples", spp * getPixelCount(m_currentSceneInfo));
    allocateResultShm(getPixelCount(m_currentSceneInfo));
    m_currentSampleBudget = getInitSampleBudget(m_currentSceneInfo);
    m_currentExecTime = 0;
    m_timer.start();
}

void BenchmarkManager::runScene(const QString& rendererPath,
                                const QString& scenePath,
                                const QString& filterPath,
                                const QString& resultPath,
                                int n,
                                int spp)
{
    // Start the benchmark server
    m_benchmarkServer->run(/*2226*/);

    // Start rendering server with the given scene
    QProcess renderingServer;
    startProcess(rendererPath, scenePath, renderingServer);

    // Start the render client
    // FIXME: setting client port manually here. Maybe all renderers should use the same port anyway?
    waitPortOpen(2227);
    m_renderClient = std::make_unique<RenderClient>(2227);
    m_tileSize = m_renderClient->getTileSize();
    m_currentSceneInfo = m_renderClient->getSceneInfo();
    if(spp)
    {
        m_currentSceneInfo.set<int64_t>("max_spp", spp);
        m_currentSceneInfo.set<int64_t>("max_samples", spp * getPixelCount(m_currentSceneInfo));
    }
    allocateResultShm(getPixelCount(m_currentSceneInfo));

    int exitType = FILTER_SUCCESS;
    for(int i = 0; i < n; ++i)
    {
        qDebug("running %d of %d", i+1, n);
        m_currentSampleBudget = getInitSampleBudget(m_currentSceneInfo);
        // Start benchmark client
        QProcess filterApp;
        startProcess(filterPath, "", filterApp);

        m_currentExecTime = 0;
        m_timer.start();

        exitType = startEventLoop(&renderingServer, &filterApp);
        if(exitType == FILTER_SUCCESS)
        {
            QString currentDir = QDir::currentPath();
            QDir::setCurrent(resultPath);
            QString baseResultFilename = QFileInfo(rendererPath).baseName() + "_" + QFileInfo(scenePath).baseName() + "_" + QFileInfo(filterPath).baseName();
            baseResultFilename.append("_" + QString::number(i));
            saveResult(baseResultFilename, exitType != FILTER_SUCCESS);
            QDir::setCurrent(currentDir);
        }
        else
            break;
    }

    // Finish rendering server and client
    if(exitType != RENDERER_CRASH)
    {
        m_renderClient->finishRender();
        renderingServer.kill();
        renderingServer.waitForFinished();
    }
}

void BenchmarkManager::runAll(const QString& configPath,
                              const QString& filterPath,
                              const QString& resultPath,
                              int n,
                              bool resume)
{
    try{ m_config = loadConfig(configPath); }
    catch(const std::runtime_error& error)
    {
        qCritical() << error.what();
        return;
    }

    // Start the benchmark server
    m_benchmarkServer->run(/*2226*/);

    std::string filterName = QFileInfo(filterPath).baseName().toStdString();

    // Launch the ASR app for each renderer, scene and spp.
    for(m_currentRenderIndex = 0; m_currentRenderIndex < m_config.renderers.size(); ++m_currentRenderIndex)
    {
        const auto& renderAtt = m_config.renderers[m_currentRenderIndex];

        for(m_currentSceneIndex = 0; m_currentSceneIndex < renderAtt.scenes.size(); ++m_currentSceneIndex)
        {
            const auto& scene = renderAtt.scenes[m_currentSceneIndex];

            QProcess renderingServer;
            bool startRenderer = true;

            bool skipRemainingSPPs = false;
            for(m_currentSppIndex = 0; m_currentSppIndex < scene.spps.size(); ++m_currentSppIndex)
            {
                if(resume)
                {
                    // If the log file exists and does not indicate a crash, skip to next iteration
                    int spp = scene.spps[m_currentSppIndex];
                    //TODO: support the case with multiple iterations
                    QString baseFilename = scene.name +
                            "/" + QString::number(spp) + "_0_log.json";
                    QFileInfo fileInfo(resultPath, baseFilename);
                    if(fileInfo.exists())
                    {
                        QFile file(fileInfo.filePath());
                        if(!file.open(QIODevice::ReadOnly))
                            qDebug() << "Couldn't open log file.";
                        else
                        {
                            QJsonDocument doc(QJsonDocument::fromJson(file.readAll()));
                            QJsonObject obj = doc.object();
                            if(!obj.contains("aborted") || !obj["aborted"].toBool())
                            {
                                std::cout << "Filter: " << filterName << ". ";
                                std::cout << "Scene: " << scene.name.toStdString() << ". ";
                                std::cout << "SPP: " << scene.spps[m_currentSppIndex] << ". ";
                                std::cout << "Iteration: 1/" << n << ". (skipping: already has results saved)" << std::endl;
                                continue;
                            }
                        }
                    }
                }

                // Start current rendering server with the current scene
                if(startRenderer)
                {
                    startRenderer = false;
                    startProcess(renderAtt.path, scene.path, renderingServer);
                    // Start the render client
                    waitPortOpen(2227);
                    m_renderClient = std::make_unique<RenderClient>(2227);
                    m_tileSize = m_renderClient->getTileSize();
                    m_currentSceneInfo = m_renderClient->getSceneInfo();
                    allocateResultShm(getPixelCount(m_currentSceneInfo));
                }

                // NOTE: The SceneInfo from the configuration file has priority over the one from the rendering system, e.g.
                //  if they have two items with the same key, the item from the rendering system is overwritten by the one
                //  from the configuration file.
                m_currentSceneInfo = m_currentSceneInfo.merged(scene.info);
                m_currentSceneInfo.set("max_spp", scene.spps[m_currentSppIndex]);
                auto sampleBudget = getInitSampleBudget(m_currentSceneInfo);
                m_currentSceneInfo.set("max_samples", sampleBudget);

                for(int i = 0; i < n; ++i)
                {
                    std::cout << "Filter: " << filterName << ". ";
                    std::cout << "Scene: " << scene.name.toStdString() << ". ";
                    std::cout << "SPP: " << scene.spps[m_currentSppIndex] << ". ";
                    std::cout << "Iteration: " << i+1 << "/" << n << "." << std::endl;

                    m_currentSampleBudget = sampleBudget;
                    // Start benchmark client
                    QProcess filterApp;
                    startProcess(filterPath, "", filterApp);
                    m_currentExecTime = 0;
                    m_timer.start();

                    ProcessExitStatus exitType = startEventLoop(&renderingServer, &filterApp);
                    int spp = scene.spps[m_currentSppIndex];
                    QString sceneName = scene.name;
                    QString prevCurrentDir = QDir::currentPath();
                    QDir::setCurrent(resultPath);
                    QDir::current().mkdir(sceneName);
                    QString baseFilename = sceneName + "/" + QString::number(spp);
                    baseFilename.append("_" + QString::number(i));
                    saveResult(baseFilename, exitType != FILTER_SUCCESS);
                    QDir::setCurrent(prevCurrentDir);
                    //move output text files
                    {
                        QString destRendererLog = QFileInfo(resultPath, baseFilename + "_renderer_output.log").filePath();
                        if(QFile::exists(destRendererLog))
                            QFile::remove(destRendererLog);
                        QFile rendererLog(QFileInfo(renderAtt.path).baseName() + ".log");
                        if(rendererLog.exists())
                            rendererLog.rename(destRendererLog);

                        QString destFilterLog = QFileInfo(resultPath, baseFilename + "_filter_output.log").filePath();
                        if(QFile::exists(destFilterLog))
                            QFile::remove(destFilterLog);
                        QFile filterLog(QString::fromStdString(filterName) + ".log");
                        if(filterLog.exists())
                            filterLog.rename(destFilterLog);
                    }
                    if(exitType == FILTER_CRASH)
                    {
                        skipRemainingSPPs = true;
                        break;
                    }
                    if(exitType == RENDERER_CRASH)
                    {
                        startRenderer = true;
                        break;
                    }
                }

                if(skipRemainingSPPs)
                    break;
            }

            // Finish rendering server and client
            if(!startRenderer)
            {
                m_renderClient->finishRender();
                renderingServer.kill();
                renderingServer.waitForFinished();
            }
        }
    }
}

void BenchmarkManager::allocateTilesMemory(int spp)
{
    auto tileSize = m_tileSize * m_tileSize * spp * m_currentSampleSize * NUM_TILES;
    auto prevSize = m_tilesMemory.size();
    auto newSize = tileSize * sizeof(float);
    if(newSize > prevSize)
    {
        m_tilesMemory.detach();
        if(!m_tilesMemory.create(newSize))
            qDebug() << "Couldn't allocate tiles memory: " << m_tilesMemory.error().c_str();
    }
}

SceneInfo BenchmarkManager::onGetSceneInfo()
{
    m_currentExecTime += m_timer.elapsed();
    m_timer.start();
    return m_currentSceneInfo;
}

int BenchmarkManager::onSetSampleLayout(const SampleLayout& layout)
{
    enum ReturnCode
    {
        OK = 0,
        INVALID_LAYOUT,
        SHARED_MEMORY_ERROR,
    };

    m_currentExecTime += m_timer.elapsed();

    if(!layout.isValid(getAllElements()))
    {
        qDebug() << "ERROR: Invalid SampleLayout requested! Aborting filter process...";
        return INVALID_LAYOUT; // invalid layout
    }

    m_currentSampleSize = layout.getSampleSize();
    m_renderClient->setParameters(layout);

    m_timer.start();
    return OK;
}

TilePkg BenchmarkManager::onEvaluateSamples(bool isSPP, int64_t numSamples)
{
    m_currentExecTime += m_timer.elapsed();

    auto numPixels = getPixelCount(m_currentSceneInfo);
    auto numGenSamples = std::min(m_currentSampleBudget, isSPP ? numSamples * numPixels : numSamples);
    m_currentSampleBudget = m_currentSampleBudget - numGenSamples;
    if(numGenSamples == 0)
        return {};

    auto spp = numGenSamples / numPixels;
    auto remaining = numGenSamples % numPixels;
    allocateTilesMemory(std::max(spp, 1L));
    TilePkg tilePkg = m_renderClient->evaluateSamples(spp, remaining);

    m_timer.start();
    return tilePkg;
}

TilePkg BenchmarkManager::onGetNextTile(int64_t prevTileIndex)
{
    m_currentExecTime += m_timer.elapsed();
    m_timer.start();
    return m_renderClient->getNextTile(prevTileIndex);
}

TilePkg BenchmarkManager::onEvaluateInputSamples(bool isSPP, int64_t numSamples)
{
    m_currentExecTime += m_timer.elapsed();

    auto numPixels = getPixelCount(m_currentSceneInfo);
    auto numGenSamples = std::min(m_currentSampleBudget, isSPP ? numSamples * numPixels : numSamples);
    m_currentSampleBudget = m_currentSampleBudget - numGenSamples;
    if(numGenSamples == 0)
        return {};

    auto spp = numGenSamples / numPixels;
    auto remaining = numGenSamples % numPixels;
    allocateTilesMemory(std::max(spp, 1L));
    TilePkg tilePkg = m_renderClient->evaluateInputSamples(spp, remaining);

    m_timer.start();
    return tilePkg;
}

TilePkg BenchmarkManager::onGetNextInputTile(int64_t prevTileIndex, bool prevWasInput)
{
    m_currentExecTime += m_timer.elapsed();
    m_timer.start();
    return m_renderClient->getNextInputTile(prevTileIndex, prevWasInput);
}

void BenchmarkManager::onLastTileConsumed(int64_t prevTileIndex)
{
    m_currentExecTime += m_timer.elapsed();
    m_renderClient->lastTileConsumed(prevTileIndex);
    m_timer.start();
}

void BenchmarkManager::onSendResult()
{
    m_currentExecTime += m_timer.elapsed();
    int h, m, s, ms;
    convertMillisecons(m_currentExecTime, &h, &m, &s, &ms);
    qDebug("Execution time = %02d:%02d:%02d:%03d", h, m, s, ms);

    if(m_passiveMode)
        saveResult("result", false);
}

void BenchmarkManager::allocateResultShm(int64_t pixelCount)
{
    if(m_resultMemory.isAttached())
        m_resultMemory.detach();

    int64_t resultMemorySize = pixelCount * 3 * static_cast<int64_t>(sizeof(float));
    if(!m_resultMemory.create(resultMemorySize))
    {
        qDebug() << "Couldn't create result memory: " << m_resultMemory.error().c_str();
        return;
    }

    auto* resultPtr = static_cast<float*>(m_resultMemory.data());
    memset(resultPtr, 0, resultMemorySize);
}

BenchmarkManager::ProcessExitStatus BenchmarkManager::startEventLoop(QProcess *renderer, QProcess *asr)
{
    ProcessExitStatus exitType = FILTER_SUCCESS;
    QEventLoop eventLoop;
    void (QProcess::*finishedSignal)(int, QProcess::ExitStatus) = &QProcess::finished;

    m_rendererConnection = QObject::connect(renderer, finishedSignal, [&](int, QProcess::ExitStatus status)
    {
        if(status == QProcess::CrashExit && asr->state() == QProcess::Running)
        {
            qDebug() << "Rendering server crashed! Killing filter process trying next spp.";
            QObject::disconnect(m_asrConnection);
            asr->kill();
            asr->waitForFinished();
            exitType = RENDERER_CRASH;
            eventLoop.quit();
        }
    });

    m_asrConnection = QObject::connect(asr, finishedSignal, [&](int exitCode, QProcess::ExitStatus status)
    {
        QObject::disconnect(m_rendererConnection);
        if(status == QProcess::NormalExit && exitCode != 0)
        {
            qDebug() << "Filter finished with code != 0. Trying next spp.";
            exitType = FILTER_ERROR;
        }
        else if(status == QProcess::CrashExit)
        {
            qDebug() << "Filter process crashed! Trying next scene.";
            exitType = FILTER_CRASH;
        }
        eventLoop.quit();
    });

    eventLoop.exec();
    return exitType;
}

void BenchmarkManager::startProcess(const QString& execPath, const QString& arg, QProcess& process)
{
    QString logFilename = QFileInfo(execPath).baseName().append(".log");
    process.setStandardOutputFile(logFilename);
    process.setStandardErrorFile(logFilename);
    process.setWorkingDirectory(QFileInfo(execPath).absolutePath());
    process.start(QFileInfo(execPath).absoluteFilePath(), {QFileInfo(arg).absoluteFilePath()});
    if(!process.waitForStarted(-1))
    {
        qDebug() << "Error starting process " << execPath;
        qDebug() << "Error code = " << process.error();
        exit(EXIT_FAILURE);
    }
}


void BenchmarkManager::saveResult(const QString& filename, bool aborted)
{
    // Write execution time log
    QFile file(filename + "_log.json");
    if(!file.open(QFile::WriteOnly))
        qWarning("Couldn't open save json log file");
    else
    {
        QJsonObject logObj;
        logObj["date"] = QDateTime::currentDateTime().toString();
        logObj["spp_budget"] = static_cast<qint64>(m_currentSceneInfo.get<int64_t>("max_spp"));
        logObj["samples_budget"] = static_cast<qint64>(getInitSampleBudget(m_currentSceneInfo));
        logObj["used_samples"] = static_cast<qint64>(getInitSampleBudget(m_currentSceneInfo) - m_currentSampleBudget);
        logObj["aborted"] = aborted;
        int h, m, s, ms;
        {
            QJsonObject execTimeObj;
            int time = aborted ? 0 : m_currentExecTime;
            execTimeObj["time_ms"] = time;
            convertMillisecons(time, &h, &m, &s, &ms);
            execTimeObj["time_str"] = QTime(h, m, s, ms).toString("hh:mm:ss.zzz");
            logObj["exec_time"] = execTimeObj;
        }
        QJsonDocument logDoc(logObj);
        file.write(logDoc.toJson());
    }

    // Write resulting image
    auto result = static_cast<float*>(m_resultMemory.data());
    int64_t xres, yres;
    getResolution(m_currentSceneInfo, &xres, &yres);
    saveExr((filename + ".exr").toStdString(), result, xres, yres);
    memset(result, 0, xres*yres*3*sizeof(float));
}
