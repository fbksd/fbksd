/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef BENCHMARKMANAGER_H
#define BENCHMARKMANAGER_H

#include "fbksd/core/definitions.h"
#include "fbksd/core/SharedMemory.h"
#include "RenderClient.h"
#include "CfgParser.h"

#include <memory>
#include <QObject>
#include <QTime>
#include <QProcess>
#include <QEventLoop>

namespace fbksd
{

class BenchmarkServer;


class BenchmarkManager
{
public:
    BenchmarkManager();

    ~BenchmarkManager();

    /**
     * @brief runPassive
     *
     * @pre Rendering server started.
     */
    void runPassive(int spp);

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
    void runScene(const QString& rendererPath,
                  const QString& scenePath,
                  const QString& filterPath,
                  const QString& resultPath,
                  int n,
                  int spp);

    /**
     * \brief Runs the benchmark for all renderers/scenes in the configuration file.
     */
    void runAll(const QString& configPath,
                const QString& filterPath,
                const QString& resultPath,
                int n,
                bool resume = false);


private:
    enum ProcessExitStatus
    {
        FILTER_SUCCESS,
        FILTER_ERROR,
        FILTER_CRASH,
        RENDERER_CRASH
    };

    void allocateTilesMemory(int spp);
    void allocateResultShm(int64_t);
    ProcessExitStatus startEventLoop(QProcess* renderer, QProcess* asr);
    void startProcess(const QString& execPath, const QString& arg, QProcess& process);
    void saveResult(const QString& filename, bool aborted);

    // Methods used by the BenchmarkServer
    SceneInfo onGetSceneInfo();
    int onSetSampleLayout(const SampleLayout& layout);
    TilePkg onEvaluateSamples(bool isSPP, int64_t numSamples);
    TilePkg onEvaluateInputSamples(bool isSPP, int64_t numSamples);
    void onLastTileConsumed(int64_t prevTileIndex);
    void onSendResult();

    std::unique_ptr<BenchmarkServer> m_benchmarkServer;
    BenchmarkConfig m_config;
    int m_currentRenderIndex = 0;
    int m_currentSceneIndex = 0;
    int m_currentSppIndex = 0;
    int64_t m_currentSampleBudget = 0;
    int m_currentSampleSize = 0;
    int m_tileSize = 0;
    SceneInfo m_currentSceneInfo;
    SharedMemory m_tilesMemory;
    SharedMemory m_resultMemory;

    QMetaObject::Connection m_rendererConnection;
    QMetaObject::Connection m_asrConnection;

    std::unique_ptr<RenderClient> m_renderClient;
    QTime m_timer; // tracks the technique execution time (accumulated in m_currentExecTime)
    int m_currentExecTime = 0;
    bool m_passiveMode = false;
};

} // namespace fbksd

#endif // BENCHMARKMANAGER_H
