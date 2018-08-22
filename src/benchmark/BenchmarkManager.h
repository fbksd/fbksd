#ifndef BENCHMARKMANAGER_H
#define BENCHMARKMANAGER_H

#include "fbksd/core/Definitions.h"
#include "fbksd/core/SharedMemory.h"
#include "RenderClient.h"
#include "CfgParser.h"

#include <memory>
#include <QObject>
#include <QTime>
#include <QProcess>
#include <QEventLoop>

class BenchmarkServer;


class BenchmarkManager
{
public:
    BenchmarkManager();

    /**
     * \brief Runs the benchmark using the given renderer and scene.
     *
     * \param renderPath    Path of the renderer executable
     * \param scenePath     Path of the scene file for the renderer
     * \param filterPath    Path of the filter executable
     * \param resultPath    Path of the folder where to save the results
     * \param n             Number of times to run the benchmark
     * \param spp           Number of samples per pixel
     */
    void runScene(const QString& rendererPath, const QString& scenePath, const QString& filterPath, const QString& resultPath, int n, int spp);

    /**
     * \brief Runs the benchmark for all renderers/scenes in the configuration file.
     */
    void runAll(const QString& configPath, const QString& filterPath, const QString& resultPath, int n, bool resume = false);

    // Methods used by the BenchmarkServer
    SceneInfo getSceneInfo();
    int setSampleLayout(const SampleLayout& layout);
    int evaluateSamples(bool isSPP, int numSamples);
    void sendResult();

    int getPixelCount(const SceneInfo& info)
    {
        int w = info.get<int>("width");
        int h = info.get<int>("height");
        return w*h;
    }

    void getResolution(const SceneInfo& info, int *w, int *h)
    {
        *w = info.get<int>("width");
        *h = info.get<int>("height");
    }

    int getInitSampleBudget(const SceneInfo& info)
    {
        return info.get<int>("max_spp") * getPixelCount(info);
    }


private:
    enum ProcessExitStatus
    {
        FILTER_SUCCESS,
        FILTER_ERROR,
        FILTER_CRASH,
        RENDERER_CRASH
    };

    void allocateSharedMemory(int);

    ProcessExitStatus startEventLoop(QProcess* renderer, QProcess* asr);

    void startProcess(const QString& execPath, const QString& arg, QProcess& process);

    void saveResult(const QString& filename, bool aborted);

    // Converts milliseconds to h:m:s:ms format
    void convertMillisecons(int time, int* h, int* m, int* s, int* ms)
    {
        *ms = time % 1000;
        *s = (time / 1000) % 60;
        *m = (time / 60000) % 60;
        *h = (time / 3600000);
    }

    BenchmarkConfig config;
    int currentRenderIndex;
    int currentSceneIndex;
    int currentSppIndex;
    int currentSampleBudget;
    SceneInfo currentSceneInfo;
    SharedMemory samplesMemory;
    SharedMemory pdfMemory;
    SharedMemory resultMemory;

    QMetaObject::Connection rendererConnection;
    QMetaObject::Connection asrConnection;

    std::unique_ptr<RenderClient> renderClient;
    QTime timer;
    int currentExecTime;
    QTime renderTimer;
    int currentRenderingTime;
};

#endif // BENCHMARKMANAGER_H
