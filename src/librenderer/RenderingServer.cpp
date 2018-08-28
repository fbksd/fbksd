#include "fbksd/renderer/RenderingServer.h"

#include <rpc/server.h>
#include <rpc/this_server.h>
#include <iostream>


RenderingServer::RenderingServer() :
    m_server(std::make_unique<rpc::server>("127.0.0.1", 2227)),
    m_samplesMemory("SAMPLES_MEMORY")
{
    m_server->bind("GET_SCENE_DESCRIPTION",
        [this](){return _getSceneInfo();});
    m_server->bind("SET_PARAMETERS",
        [this](const SampleLayout& layout){ _setParameters(layout); });
    m_server->bind("DETACH_MEMORY",
        [this](){ _detachMemory(); });
    m_server->bind("EVALUATE_SAMPLES",
        [this](int64_t spp, int64_t remainingCount){ return _evaluateSamples(spp, remainingCount); });
    m_server->bind("FINISH_RENDER",
        [this](){ _finishRender(); });
}

RenderingServer::~RenderingServer() = default;

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

void RenderingServer::_setParameters(const SampleLayout& layout)
{
    if(!m_samplesMemory.attach())
        std::cout << m_samplesMemory.error() << std::endl;

    SamplesPipe::init(layout, static_cast<float*>(m_samplesMemory.data()));
    m_setParameters(layout);
}

bool RenderingServer::_evaluateSamples(int64_t spp, int64_t remainingCount)
{
    return m_evalSamples(spp, remainingCount);
}

void RenderingServer::_finishRender()
{
    if(m_finish)
        m_finish();
    rpc::this_server().stop();
}
