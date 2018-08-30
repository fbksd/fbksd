#include "RenderClient.h"
#include "BenchmarkManager.h"
#include "version.h"
#include <rpc/client.h>


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

SceneInfo RenderClient::getSceneInfo()
{
    return m_client->call("GET_SCENE_DESCRIPTION").as<SceneInfo>();
}

void RenderClient::detachMemory()
{
    m_client->call("DETACH_MEMORY");
}

void RenderClient::setParameters(const SampleLayout& layout)
{
    m_client->call("SET_PARAMETERS", layout);
}

bool RenderClient::evaluateSamples(int64_t spp, int64_t remainintCount)
{
    return m_client->call("EVALUATE_SAMPLES", spp, remainintCount).as<bool>();
}

void RenderClient::finishRender()
{
    m_client->async_call("FINISH_RENDER");
}
