#include "BenchmarkManager.h"

#include "BenchmarkServer.h"
#include "RenderClient.h"
#include "fbksd/renderer/samples.h"

#include <memory>
#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QEventLoop>
#include <QThread>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>


BenchmarkManager::BenchmarkManager():
    samplesMemory("SAMPLES_MEMORY"),
    pdfMemory("PDF_MEMORY"),
    resultMemory("RESULT_MEMORY")
{
    currentExecTime = 0;
    currentRenderingTime = 0;
}

void BenchmarkManager::runScene(const QString& rendererPath, const QString& scenePath, const QString& filterPath, const QString& resultPath, int n, int spp)
{
    // Start the benchmark server
    BenchmarkServer benchmarkServer(this);
    benchmarkServer.startServer(2226);

    // Start rendering server with the given scene
    QProcess renderingServer;
#ifndef MANUAL_RENDERER
    startProcess(rendererPath, scenePath, renderingServer);
#endif

    // Start the render client
    // FIXME: setting client port manually here. Maybe all renderers should use the same port anyway?
    renderClient.reset(new RenderClient(this, 2227));
    currentSceneInfo = renderClient->getSceneInfo();
    if(spp)
    {
        currentSceneInfo.set<int>("max_spp", spp);
        currentSceneInfo.set<int>("max_samples", spp * getPixelCount(currentSceneInfo));
    }
    allocateSharedMemory(getPixelCount(currentSceneInfo));

    int exitType = FILTER_SUCCESS;
    for(int i = 0; i < n; ++i)
    {
        qDebug("running %d of %d", i+1, n);
        currentSampleBudget = getInitSampleBudget(currentSceneInfo);
        // Start benchmark client
        QProcess filterApp;
#ifndef MANUAL_ASR
        startProcess(filterPath, "", filterApp);
#endif

        currentExecTime = 0;
        timer.start();
        currentRenderingTime = 0;

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
        renderClient->finishRender();
        renderingServer.waitForFinished();
    }
}

void BenchmarkManager::runAll(const QString& configPath, const QString& filterPath, const QString& resultPath, int n, bool resume)
{
    config = CfgParser::load(configPath.toStdString());
    if(!config) return;

    // Start the benchmark server
    BenchmarkServer benchmarkServer(this);
    benchmarkServer.startServer(2226);

    std::string filterName = QFileInfo(filterPath).baseName().toStdString();

    // Launch the ASR app for each renderer, scene and spp.
    for(currentRenderIndex = 0; currentRenderIndex < config->renderers.size(); ++currentRenderIndex)
    {
        const auto& renderAtt = config->renderers[currentRenderIndex];

        for(currentSceneIndex = 0; currentSceneIndex < renderAtt.scenes.size(); ++currentSceneIndex)
        {
            const auto& scene = renderAtt.scenes[currentSceneIndex];

            QProcess renderingServer;
            bool startRenderer = true;

            bool skipRemainingSPPs = false;
            for(currentSppIndex = 0; currentSppIndex < scene.spps.size(); ++currentSppIndex)
            {
                if(resume)
                {
                    // If the log file exists and does not indicate a crash, skip to next iteration
                    int spp = scene.spps[currentSppIndex];
                    //TODO: support the case with multiple iterations
                    QString baseFilename = QString::fromStdString(scene.name) +
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
                                std::cout << "Scene: " << scene.name << ". ";
                                std::cout << "SPP: " << scene.spps[currentSppIndex] << ". ";
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
#ifndef MANUAL_RENDERER
                    startProcess(QString::fromStdString(renderAtt.path), QString::fromStdString(scene.path), renderingServer);
#endif
                    // Start the render client
                    renderClient.reset(new RenderClient(this, 2227));
                    currentSceneInfo = renderClient->getSceneInfo();
                    allocateSharedMemory(getPixelCount(currentSceneInfo));
                }

                // NOTE: The SceneInfo from the configuration file has priority over the one from the rendering system, e.g.
                //  if they have two items with the same key, the item from the rendering system is overwritten by the one
                //  from the configuration file.
                currentSceneInfo = currentSceneInfo.merged(scene.info);
                currentSceneInfo.set<int>("max_spp", scene.spps[currentSppIndex]);
                int sampleBudget = getInitSampleBudget(currentSceneInfo);
                currentSceneInfo.set<int>("max_samples", sampleBudget);

                for(int i = 0; i < n; ++i)
                {
                    std::cout << "Filter: " << filterName << ". ";
                    std::cout << "Scene: " << scene.name << ". ";
                    std::cout << "SPP: " << scene.spps[currentSppIndex] << ". ";
                    std::cout << "Iteration: " << i+1 << "/" << n << "." << std::endl;

                    currentSampleBudget = sampleBudget;
                    // Start benchmark client
                    QProcess filterApp;
#ifndef MANUAL_ASR
                    startProcess(filterPath, "", filterApp);
#endif
                    currentExecTime = 0;
                    timer.start();
                    currentRenderingTime = 0;

                    ProcessExitStatus exitType = startEventLoop(&renderingServer, &filterApp);
                    int spp = scene.spps[currentSppIndex];
                    QString sceneName = QString::fromStdString(scene.name);
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
                        QFile rendererLog(QFileInfo(QString::fromStdString(renderAtt.path)).baseName() + ".log");
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
                renderClient->finishRender();
                renderingServer.waitForFinished();
            }
        }
    }
}

SceneInfo BenchmarkManager::getSceneInfo()
{
    currentExecTime += timer.elapsed();
    timer.start();
    return currentSceneInfo;
}

int BenchmarkManager::setSampleLayout(const SampleLayout& layout)
{
    enum ReturnCode
    {
        OK = 0,
        INVALID_LAYOUT,
        SHARED_MEMORY_ERROR,
    };

    currentExecTime += timer.elapsed();

    if(!layout.isValid(getAllElements()))
    {
        qDebug() << "ERROR: Invalid SampleLayout requested! Aborting filter process...";
        return INVALID_LAYOUT; // invalid layout
    }

    auto prevSize = samplesMemory.size();
    size_t sampleSize = layout.getSampleSize();
    auto newSize = currentSampleBudget * sampleSize * sizeof(float);
    if(newSize > prevSize)
    {
        samplesMemory.detach();
        renderClient->detachMemory();
        if(!samplesMemory.create(newSize))
        {
            qDebug() << "Couldn't allocate samplesMemory: " << samplesMemory.error().c_str();
            return SHARED_MEMORY_ERROR;
        }
    }

    int spp = currentSceneInfo.get<int>("max_spp");
    renderClient->setParameters(spp, layout);

    timer.start();
    return OK;
}

int BenchmarkManager::evaluateSamples(bool isSPP, int numSamples)
{
    currentExecTime += timer.elapsed();

    int numPixels = getPixelCount(currentSceneInfo);
    int numGenSamples = std::min(currentSampleBudget, isSPP ? numSamples * numPixels : numSamples);
    currentSampleBudget = currentSampleBudget - numGenSamples;

    renderTimer.start();
    // if number of allowed samples to compute can be expressed as spp
    if(isSPP && ((numGenSamples % numPixels) == 0))
        numGenSamples = renderClient->evaluateSamples(true, numGenSamples / numPixels);
    else
        numGenSamples = renderClient->evaluateSamples(false, numGenSamples);
    currentRenderingTime += renderTimer.elapsed();

    timer.start();
    return numGenSamples;
}

int BenchmarkManager::evaluateSamples(bool isSPP, int numSamples, const CropWindow &crop)
{
    currentExecTime += timer.elapsed();

    int numPixels = getPixelCount(currentSceneInfo);
    int numGenSamples = std::min(currentSampleBudget, isSPP ? numSamples * numPixels : numSamples);
    currentSampleBudget = currentSampleBudget - numGenSamples;

    renderTimer.start();
    // if number of allowed samples to compute can be expressed as spp
    if(isSPP && ((numGenSamples % numPixels) == 0))
        numGenSamples = renderClient->evaluateSamples(true, numGenSamples / numPixels, crop);
    else
        numGenSamples = renderClient->evaluateSamples(false, numGenSamples, crop);
    currentRenderingTime += renderTimer.elapsed();

    timer.start();
    return numGenSamples;
}

int BenchmarkManager::evaluateAdaptiveSamples(bool isSPP, int numSamples)
{
    currentExecTime += timer.elapsed();

    int numPixels = getPixelCount(currentSceneInfo);
    int numGenSamples = std::min(currentSampleBudget, isSPP ? numSamples * numPixels : numSamples);
    currentSampleBudget = currentSampleBudget - numGenSamples;

    renderTimer.start();
    // if number of allowed samples to compute can be expressed as spp
    if(isSPP && ((numGenSamples % numPixels) == 0))
        numGenSamples = renderClient->evaluateAdaptiveSamples(true, numGenSamples / numPixels);
    else
        numGenSamples = renderClient->evaluateAdaptiveSamples(false, numGenSamples);
    currentRenderingTime += renderTimer.elapsed();

    timer.start();
    return numGenSamples;
}

void BenchmarkManager::sendResult()
{
    currentExecTime += timer.elapsed();
    int h, m, s, ms;
    convertMillisecons(currentExecTime, &h, &m, &s, &ms);
    qDebug("Filter execution time = %02d:%02d:%02d:%03d", h, m, s, ms);
    convertMillisecons(currentRenderingTime, &h, &m, &s, &ms);
    qDebug("Render execution time = %02d:%02d:%02d:%03d", h, m, s, ms);

#ifdef MANUAL_ASR
    saveResult("result.exr", false);
#endif
}

void BenchmarkManager::allocateSharedMemory(int pixelCount)
{
    if(pdfMemory.isAttached())
        pdfMemory.detach();
    if(resultMemory.isAttached())
        resultMemory.detach();

    int pdfMemorySize = pixelCount * sizeof(float);
    if(!pdfMemory.create(pdfMemorySize))
    {
        qDebug() << "Couldn't create pdf memory: " << pdfMemory.error().c_str();
        return;
    }
    int resultMemorySize = pixelCount * 3 * sizeof(float);
    if(!resultMemory.create(resultMemorySize))
    {
        qDebug() << "Couldn't create result memory: " << resultMemory.error().c_str();
        return;
    }

    float* pdfPtr = static_cast<float*>(pdfMemory.data());
    memset(pdfPtr, 0, pdfMemorySize);
    float* resultPtr = static_cast<float*>(resultMemory.data());
    memset(resultPtr, 0, resultMemorySize);

}

BenchmarkManager::ProcessExitStatus BenchmarkManager::startEventLoop(QProcess *renderer, QProcess *asr)
{
    ProcessExitStatus exitType = FILTER_SUCCESS;
    QEventLoop eventLoop;
    void (QProcess::*finishedSignal)(int, QProcess::ExitStatus) = &QProcess::finished;

#ifndef MANUAL_RENDERER
    rendererConnection = QObject::connect(renderer, finishedSignal, [&](int, QProcess::ExitStatus status)
    {
        if(status == QProcess::CrashExit && asr->state() == QProcess::Running)
        {
            qDebug() << "Rendering server crashed! Killing filter process trying next spp.";
            QObject::disconnect(asrConnection);
            asr->kill();
            asr->waitForFinished();
            exitType = RENDERER_CRASH;
            eventLoop.quit();
        }
    });
#endif

#ifndef MANUAL_ASR
    asrConnection = QObject::connect(asr, finishedSignal, [&](int exitCode, QProcess::ExitStatus status)
    {
        QObject::disconnect(rendererConnection);
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
#endif

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

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
using namespace Imath;

void BenchmarkManager::saveResult(const QString& filename, bool aborted)
{
    // Write execution time log
    {
        QFile file(filename + "_log.json");
        if(!file.open(QFile::WriteOnly))
            qWarning("Couldn't open save json log file");
        else
        {
            QJsonObject logObj;
            logObj["date"] = QDateTime::currentDateTime().toString();
            logObj["spp_budget"] = currentSceneInfo.get<int>("max_spp");
            logObj["samples_budget"] = getInitSampleBudget(currentSceneInfo);
            logObj["used_samples"] = getInitSampleBudget(currentSceneInfo) - currentSampleBudget;
            logObj["aborted"] = aborted;
            int h, m, s, ms;
            {
                QJsonObject recTimeObj;
                int time = aborted ? 0 : currentExecTime;
                recTimeObj["time_ms"] = time;
                convertMillisecons(time, &h, &m, &s, &ms);
                recTimeObj["time_str"] = QTime(h, m, s, ms).toString("hh:mm:ss.zzz");
                logObj["reconstruction_time"] = recTimeObj;
            }
            {
                QJsonObject renderingTimeObj;
                renderingTimeObj["time_ms"] = currentRenderingTime;
                convertMillisecons(currentRenderingTime, &h, &m, &s, &ms);
                renderingTimeObj["time_str"] = QTime(h, m, s, ms).toString("hh:mm:ss.zzz");
                logObj["rendering_time"] = renderingTimeObj;
            }
            QJsonDocument logDoc(logObj);
            file.write(logDoc.toJson());
        }
    }

    // Write resulting image
    {
        float* result = static_cast<float*>(resultMemory.data());

        int xres, yres;
        getResolution(currentSceneInfo, &xres, &yres);

        Imf::Header header(xres, yres);
        header.channels().insert ("R", Imf::Channel(Imf::FLOAT));
        header.channels().insert ("G", Imf::Channel(Imf::FLOAT));
        header.channels().insert ("B", Imf::Channel(Imf::FLOAT));

        Imf::OutputFile file((filename + ".exr").toStdString().data(), header);
        Imf::FrameBuffer frameBuffer;

        frameBuffer.insert ("R", Imf::Slice( Imf::PixelType(Imf::FLOAT), (char*)result, sizeof(*result)*3, sizeof(*result)*xres*3));
        frameBuffer.insert ("G", Imf::Slice( Imf::PixelType(Imf::FLOAT), (char*)(result + 1), sizeof(*result)*3, sizeof(*result)*xres*3));
        frameBuffer.insert ("B", Imf::Slice( Imf::PixelType(Imf::FLOAT), (char*)(result + 2), sizeof(*result)*3, sizeof(*result)*xres*3));

        file.setFrameBuffer(frameBuffer);
        file.writePixels(yres);
        memset(result, 0, xres*yres*3*sizeof(float));
    }
}
