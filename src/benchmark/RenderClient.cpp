#include "RenderClient.h"
#include "BenchmarkManager.h"
#include <rpc/client.h>


RenderClient::RenderClient(BenchmarkManager *manager, int port):
    m_client(std::make_unique<rpc::client>("127.0.0.1", port))
{
    this->manager = manager;
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

void RenderClient::setParameters(int64_t maxSPP, const SampleLayout& layout)
{
    m_client->call("SET_PARAMETERS", maxSPP, layout);
}

int64_t RenderClient::evaluateSamples(bool isSPP, int64_t numSamples)
{
    return m_client->call("EVALUATE_SAMPLES", isSPP, numSamples).as<int64_t>();
}

void RenderClient::finishRender()
{
    m_client->async_call("FINISH_RENDER");
}
