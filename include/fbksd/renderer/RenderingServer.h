#ifndef RENDERINGSERVER_H
#define RENDERINGSERVER_H

#include "SamplesPipe.h"
#include "fbksd/core/Definitions.h"
#include "fbksd/core/SharedMemory.h"
#include <map>
#include <memory>
#include <functional>

namespace rpc { class server; }


/**
 * \defgroup RenderingServer Rendering Server Library
 * @{
 */


/**
 * \brief The RenderingServer class implements the server by which a rendering system provides sampling
 * computations to the benchmark system.
 *
 * This class provides an asynchronous communication interface using signals. Each time a request is received,
 * the appropriate signal is emitted.
 *
 */
class RenderingServer
{
public:
    using GetSceneInfo
        = std::function<SceneInfo()>;
    using SetParameters
        = std::function<void(const SampleLayout& layout)>;
    using EvaluateSamples
        = std::function<bool(int64_t spp, int64_t remainingCount)>;
    using Finish
        = std::function<void()>;

    RenderingServer();

    ~RenderingServer();

    void onGetSceneInfo(const GetSceneInfo& callback);

    void onSetParameters(const SetParameters& callback);

    void onEvaluateSamples(const EvaluateSamples& callback);

    void onFinish(const Finish& callback);

    void run();

private:
    SceneInfo _getSceneInfo();
    void _detachMemory();
    void _setParameters(const SampleLayout& layout);
    bool _evaluateSamples(int64_t spp, int64_t remainingCount);
    void _finishRender();

    std::unique_ptr<rpc::server> m_server;
    SharedMemory m_samplesMemory;
    int64_t m_pixelCount;

    GetSceneInfo m_getSceneInfo;
    SetParameters m_setParameters;
    EvaluateSamples m_evalSamples;
    Finish m_finish;
};

/**@}*/

#endif // RENDERINGSERVER_H
