#include "fbksd/renderer/RenderingServer.h"

#include <rpc/server.h>
#include <rpc/this_server.h>
#include <iostream>


RenderingServer::RenderingServer() :
    m_server(std::make_unique<rpc::server>("127.0.0.1", 2227)),
    m_samplesMemory("SAMPLES_MEMORY")
{
    m_server->bind("GET_SCENE_DESCRIPTION", [this](){return _getSceneInfo();});
    m_server->bind("SET_PARAMETERS", [this](int maxSpp, const SampleLayout& layout)
    { _setParameters(maxSpp, layout); });
    m_server->bind("DETACH_MEMORY", [this](){ _detachMemory(); });
    m_server->bind("EVALUATE_SAMPLES", [this](bool isSpp, int64_t numSamples){ return _evaluateSamples(isSpp, numSamples); });
    m_server->bind("FINISH_RENDER", [this](){ _finishRender(); });
}

void RenderingServer::onGetSceneInfo(const GetSceneInfo& callback)
{
    m_getSceneInfo = callback;
}

void RenderingServer::onSetParameters(const SetParameters& callback)
{
    m_setParameters = callback;
}

void RenderingServer::onEvaluateSamples(const EvaluateSamples& callback)
{
    m_evalSamples = callback;
}

void RenderingServer::onFinish(const Finish& callback)
{
    m_finish = callback;
}

void RenderingServer::run()
{
    m_server->run();
}

RenderingServer::~RenderingServer() = default;

SceneInfo RenderingServer::_getSceneInfo()
{
    SceneInfo scene = m_getSceneInfo();
    m_pixelCount = scene.get<int64_t>("width") * scene.get<int64_t>("height");
    return scene;
}

void RenderingServer::_detachMemory()
{
    m_samplesMemory.detach();
}

void RenderingServer::_setParameters(int maxSpp, const SampleLayout& layout)
{
    if(!m_samplesMemory.attach())
        std::cout << m_samplesMemory.error() << std::endl;

    SamplesPipe::setLayout(layout);
    SamplesPipe::samples = static_cast<float*>(m_samplesMemory.data());
    m_setParameters(maxSpp, layout, static_cast<float*>(m_samplesMemory.data()));
}

int64_t RenderingServer::_evaluateSamples(bool isSPP, int64_t numSamples)
{
    SamplesPipe::numSamples = isSPP ? numSamples * m_pixelCount : numSamples;
    return m_evalSamples(isSPP, numSamples);
}

void RenderingServer::_finishRender()
{
    if(m_finish)
        m_finish();
    rpc::this_server().stop();
}
