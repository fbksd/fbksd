/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/renderer/RenderingServer.h"
#include "version.h"
using namespace fbksd;

#include <rpc/server.h>
#include <rpc/this_server.h>
#include <iostream>


struct RenderingServer::Imp
{
    Imp():
        m_server(std::make_unique<rpc::server>("127.0.0.1", 2227)),
        m_samplesMemory("SAMPLES_MEMORY")
    {}

    SceneInfo _getSceneInfo()
    {
        SceneInfo scene = m_getSceneInfo();
        m_pixelCount = scene.get<int64_t>("width") * scene.get<int64_t>("height");
        return scene;
    }

    void _detachMemory()
    {
        m_samplesMemory.detach();
    }

    void _setParameters(const SampleLayout& layout)
    {
        if(!m_samplesMemory.attach())
            std::cout << m_samplesMemory.error() << std::endl;

        SamplesPipe::init(layout, static_cast<float*>(m_samplesMemory.data()));
        m_setParameters(layout);
    }

    bool _evaluateSamples(int64_t spp, int64_t remainingCount)
    {
        return m_evalSamples(spp, remainingCount);
    }

    void _finishRender()
    {
        if(m_finish)
            m_finish();
        rpc::this_server().stop();
    }

    std::unique_ptr<rpc::server> m_server;
    SharedMemory m_samplesMemory;
    int64_t m_pixelCount;

    GetSceneInfo m_getSceneInfo;
    SetParameters m_setParameters;
    EvaluateSamples m_evalSamples;
    Finish m_finish;
};


RenderingServer::RenderingServer() :
    m_imp(std::make_unique<Imp>())
{
    m_imp->m_server->bind("GET_VERSION", []()
    { return std::make_pair(FBKSD_VERSION_MAJOR, FBKSD_VERSION_MINOR); });
    m_imp->m_server->bind("GET_SCENE_DESCRIPTION",
        [this](){return m_imp->_getSceneInfo();});
    m_imp->m_server->bind("SET_PARAMETERS",
        [this](const SampleLayout& layout){ m_imp->_setParameters(layout); });
    m_imp->m_server->bind("DETACH_MEMORY",
        [this](){ m_imp->_detachMemory(); });
    m_imp->m_server->bind("EVALUATE_SAMPLES",
        [this](int64_t spp, int64_t remainingCount){ return m_imp->_evaluateSamples(spp, remainingCount); });
    m_imp->m_server->bind("FINISH_RENDER",
        [this](){ m_imp->_finishRender(); });
}

RenderingServer::~RenderingServer() = default;

void RenderingServer::onGetSceneInfo(const GetSceneInfo& callback)
{
    m_imp->m_getSceneInfo = callback;
}

void RenderingServer::onSetParameters(const SetParameters& callback)
{
    m_imp->m_setParameters = callback;
}

void RenderingServer::onEvaluateSamples(const EvaluateSamples& callback)
{
    m_imp->m_evalSamples = callback;
}

void RenderingServer::onFinish(const Finish& callback)
{
    m_imp->m_finish = callback;
}

void RenderingServer::run()
{
    m_imp->m_server->run();
}

