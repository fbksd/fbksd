/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "RenderClient.h"
#include "BenchmarkManager.h"
#include "version.h"
#include <rpc/client.h>
using namespace fbksd;


RenderClient::RenderClient(int port):
    m_client(std::make_unique<rpc::client>("127.0.0.1", port))
{
    auto version = m_client->call("GET_VERSION").as<std::pair<int,int>>();
    if(version.first != FBKSD_VERSION_MAJOR)
    {
        auto error = std::string("Renderer server version (") +
                     std::to_string(version.first) + ") is incompatible with client version (" +
                     std::to_string(FBKSD_VERSION_MAJOR) + ").";
        throw std::runtime_error(error);
    }
}

RenderClient::~RenderClient() = default;

int RenderClient::getTileSize()
{
    return m_client->call("GET_TILE_SIZE").as<int>();
}

SceneInfo RenderClient::getSceneInfo()
{
    return m_client->call("GET_SCENE_DESCRIPTION").as<SceneInfo>();
}

void RenderClient::setParameters(const SampleLayout& layout)
{
    m_client->call("SET_PARAMETERS", layout);
}

TilePkg RenderClient::evaluateSamples(int64_t spp, int64_t remainintCount)
{
    return m_client->call("EVALUATE_SAMPLES", spp, remainintCount).as<TilePkg>();
}

TilePkg RenderClient::getNextTile(int64_t prevTileIndex)
{
    return m_client->call("GET_NEXT_TILE", prevTileIndex).as<TilePkg>();
}

TilePkg RenderClient::evaluateInputSamples(int64_t spp, int64_t remainingCount)
{
    return m_client->call("EVALUATE_INPUT_SAMPLES", spp, remainingCount).as<TilePkg>();
}

TilePkg RenderClient::getNextInputTile(int64_t prevTileIndex, bool prevWasInput)
{
    return m_client->call("GET_NEXT_INPUT_TILE", prevTileIndex, prevWasInput).as<TilePkg>();
}

void RenderClient::lastTileConsumed(int64_t prevTileIndex)
{
    m_client->call("LAST_TILE_CONSUMED", prevTileIndex);
}

void RenderClient::finishRender()
{
    m_client->async_call("FINISH_RENDER");
}
