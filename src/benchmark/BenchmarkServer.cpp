#include "BenchmarkServer.h"
#include "BenchmarkManager.h"
#include <rpc/server.h>



BenchmarkServer::BenchmarkServer(BenchmarkManager *manager):
    m_server(std::make_unique<rpc::server>("127.0.0.1", 2226)),
    manager(manager)
{
    m_server->bind("EVALUATE_SAMPLES", [this](bool isSpp, int numSamples){ return evaluateSamples(isSpp, numSamples); });
    m_server->bind("GET_SCENE_DESCRIPTION", [this](){ return getSceneInfo(); });
    m_server->bind("SET_SAMPLE_LAYOUT", [this](const SampleLayout& layout){ return setSampleLayout(layout); });
    m_server->bind("SEND_RESULT", [this](){ sendResult(); });

//    m_server->bind("GET_MODE", [this](QDataStream&, QDataStream& out)
//    {
//        bool mode = false;
//        out << (quint64)sizeof(bool);
//        out << mode;
    //    });
}

BenchmarkServer::~BenchmarkServer() = default;

void BenchmarkServer::run()
{
    m_server->async_run(1);
}

void BenchmarkServer::stop()
{
    m_server->stop();
}

SceneInfo BenchmarkServer::getSceneInfo()
{
    return manager->getSceneInfo();
}

int BenchmarkServer::setSampleLayout(const SampleLayout& layout)
{
    return manager->setSampleLayout(layout);
}

int BenchmarkServer::evaluateSamples(bool isSpp, int numSamples)
{
    return manager->evaluateSamples(isSpp, numSamples);
}

void BenchmarkServer::sendResult()
{
    manager->sendResult();
}
