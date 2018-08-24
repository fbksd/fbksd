#ifndef RENDERINGSERVER_H
#define RENDERINGSERVER_H

#include "samples.h"
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
        = std::function<void(int maxSPP, const SampleLayout& layout, float* samplesBuffer, float* pdfBuffer)>;
    using EvaluateSamples
        = std::function<void(bool isSPP, int numSamples, int* resultSize)>;
    using Finish
        = std::function<void()>;

    RenderingServer();

    ~RenderingServer();

    void onGetSceneInfo(GetSceneInfo callback);

    void onSetParameters(SetParameters callback);

    void onEvaluateSamples(EvaluateSamples callback);

    void onFinish(Finish callback);

    void run();

private:
    SceneInfo _getSceneInfo();
    void _detachMemory();
    void _setParameters(int maxSpp, const SampleLayout& layout);
    int _evaluateSamples(bool isSPP, int numSamples);
    void _finishRender();

    std::unique_ptr<rpc::server> m_server;
    SharedMemory m_samplesMemory;
    SharedMemory m_pdfMemory;
    int m_pixelCount;

    GetSceneInfo m_getSceneInfo;
    SetParameters m_setParameters;
    EvaluateSamples m_evalSamples;
    Finish m_finish;
};

/**@}*/

#endif // RENDERINGSERVER_H
