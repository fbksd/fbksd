/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/renderer/RenderingServer.h"
#include "TilePool.h"
#include "version.h"
using namespace fbksd;

#include <rpc/server.h>
#include <rpc/this_server.h>
#include <iostream>


struct RenderingServer::Imp
{
    Imp():
        m_server(std::make_unique<rpc::server>("127.0.0.1", 2227)),
        m_tilesMemory("TILES_MEMORY")
    {}

    int getTileSize()
    {
        return m_tileSize = m_getTileSize();
    }

    SceneInfo getSceneInfo()
    {
        SceneInfo scene = m_getSceneInfo();
        m_pixelCount = scene.get<int64_t>("width") * scene.get<int64_t>("height");
        return scene;
    }

    void setParameters(const SampleLayout& layout)
    {
        SamplesPipe::setLayout(layout);
        m_setParameters(layout);
    }

    TilePkg evaluateSamples(int64_t spp, int64_t remainingCount)
    {
        m_tilesMemory.detach();
        if(!m_tilesMemory.attach())
            throw std::runtime_error("Error attaching tiles shm: " + m_tilesMemory.error());

        SamplesPipe::sm_numSamples = spp;
        const int pipeMaxNumSamples = std::max(spp, 1L) * m_tileSize * m_tileSize;
        TilePool::init(spp * m_pixelCount + remainingCount,
                       pipeMaxNumSamples,
                       SamplesPipe::sm_sampleSize,
                       static_cast<float*>(m_tilesMemory.data()),
                       false);

        m_evalSamples(spp, remainingCount, pipeMaxNumSamples);

        bool hasNext = false;
        bool isInput = false;
        auto tile = TilePool::getClientTile(hasNext, isInput);
        return {tile, hasNext, isInput};
    }

    TilePkg getNextTile(int64_t prevIndex)
    {
        TilePool::releaseConsumedTile(prevIndex);
        bool hasNext = false;
        bool isInput = false;
        auto tile = TilePool::getClientTile(hasNext, isInput);
        return {tile, hasNext, isInput};
    }

    TilePkg evaluateInputSamples(int64_t spp, int64_t remainingCount)
    {
        m_tilesMemory.detach();
        if(!m_tilesMemory.attach())
            throw std::runtime_error("Error attaching tiles shm: " + m_tilesMemory.error());

        SamplesPipe::sm_numSamples = spp;
        const int pipeMaxNumSamples = std::max(spp, 1L) * m_tileSize * m_tileSize;
        TilePool::init(spp * m_pixelCount + remainingCount,
                       pipeMaxNumSamples,
                       SamplesPipe::sm_sampleSize,
                       static_cast<float*>(m_tilesMemory.data()),
                       true);

        m_evalSamples(spp, remainingCount, pipeMaxNumSamples);

        bool hasNext = false;
        bool isInput = false;
        auto tile = TilePool::getClientTile(hasNext, isInput);
        return {tile, hasNext, isInput};
    }

    TilePkg getNextInputTile(int64_t prevIndex, bool prevWasInput)
    {
        if(prevWasInput)
            TilePool::releaseInputTile(prevIndex);
        else
            TilePool::releaseConsumedTile(prevIndex);

        bool hasNext = false;
        bool isInput = false;
        auto tile = TilePool::getClientTile(hasNext, isInput);
        return {tile, hasNext, isInput};
    }

    void lastTileConsumed(int64_t prevIndex)
    {
        TilePool::releaseConsumedTile(prevIndex);
        m_lastTileConsumed();
    }

    void finishRender()
    {
        m_finish();
        rpc::this_server().stop();
    }

    std::unique_ptr<rpc::server> m_server;
    SharedMemory m_tilesMemory;
    int64_t m_pixelCount = 0;
    int64_t m_tileSize = 0;
    GetTileSize m_getTileSize;
    GetSceneInfo m_getSceneInfo;
    SetParameters m_setParameters;
    EvaluateSamples m_evalSamples;
    LastTileConsumed m_lastTileConsumed = [](){};
    Finish m_finish = [](){};
};


RenderingServer::RenderingServer() :
    m_imp(std::make_unique<Imp>())
{
    m_imp->m_server->bind("GET_VERSION", []()
    { return std::make_pair(FBKSD_VERSION_MAJOR, FBKSD_VERSION_MINOR); });
    m_imp->m_server->bind("GET_TILE_SIZE",
        [this](){return m_imp->getTileSize();});
    m_imp->m_server->bind("GET_SCENE_DESCRIPTION",
        [this](){return m_imp->getSceneInfo();});
    m_imp->m_server->bind("SET_PARAMETERS",
        [this](const SampleLayout& layout){ m_imp->setParameters(layout); });
    m_imp->m_server->bind("EVALUATE_SAMPLES",
        [this](int64_t spp, int64_t remainingCount){ return m_imp->evaluateSamples(spp, remainingCount); });
    m_imp->m_server->bind("GET_NEXT_TILE",
        [this](int64_t prevTileIndex){ return m_imp->getNextTile(prevTileIndex); });
    m_imp->m_server->bind("EVALUATE_INPUT_SAMPLES",
        [this](int64_t spp, int64_t remainingCount){ return m_imp->evaluateInputSamples(spp, remainingCount); });
    m_imp->m_server->bind("GET_NEXT_INPUT_TILE",
        [this](int64_t prevTileIndex, bool prevWasInput){ return m_imp->getNextInputTile(prevTileIndex, prevWasInput); });
    m_imp->m_server->bind("LAST_TILE_CONSUMED",
        [this](int64_t index){ return m_imp->lastTileConsumed(index); });
    m_imp->m_server->bind("FINISH_RENDER",
                          [this](){ m_imp->finishRender(); });
}

RenderingServer::~RenderingServer() = default;

void RenderingServer::onGetTileSize(const RenderingServer::GetTileSize &callback)
{
    m_imp->m_getTileSize = callback;
}

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

void RenderingServer::onLastTileConsumed(const LastTileConsumed &callback)
{
    m_imp->m_lastTileConsumed = callback;
}

void RenderingServer::onFinish(const Finish& callback)
{
    m_imp->m_finish = callback;
}

void RenderingServer::run()
{
    if(!m_imp->m_getTileSize)
        throw std::logic_error("GetTileSize callback not registered.");
    if(!m_imp->m_getSceneInfo)
        throw std::logic_error("GetSceneInfo callback not registered.");
    if(!m_imp->m_setParameters)
        throw std::logic_error("SetParameters callback not registered.");
    if(!m_imp->m_evalSamples)
        throw std::logic_error("EvaluateSamples callback not registered.");

    m_imp->m_server->run();
}
