#include "BenchmarkServer.h"
#include "BenchmarkManager.h"
#include <rpc/server.h>



BenchmarkServer::BenchmarkServer():
    m_server(std::make_unique<rpc::server>("127.0.0.1", 2226))
{
}

BenchmarkServer::~BenchmarkServer() = default;

void BenchmarkServer::onGetSceneInfo(const GetSceneInfo& callback)
{
    m_getSceneInfoSet = true;
    m_server->bind("GET_SCENE_DESCRIPTION", callback);
}

void BenchmarkServer::onSetParameters(const SetParameters& callback)
{
    m_setParametersSet = true;
    m_server->bind("SET_SAMPLE_LAYOUT", callback);
}

void BenchmarkServer::onEvaluateSamples(const EvaluateSamples& callback)
{
    m_evalSamplesSet = true;
    m_server->bind("EVALUATE_SAMPLES", callback);
}

void BenchmarkServer::onSendResult(const SendResult& callback)
{
    m_sendResultSet = true;
    m_server->bind("SEND_RESULT", callback);
}

void BenchmarkServer::run()
{
    std::string missingFunc;
    if(!m_getSceneInfoSet)
        missingFunc += "GetSceneInfo ";
    if(!m_setParametersSet)
        missingFunc += "SetParameters ";
    if(!m_evalSamplesSet)
        missingFunc += "EvaluateSamples ";
    if(!m_sendResultSet)
        missingFunc += "SendResult";
    if(!missingFunc.empty())
        throw std::logic_error("Missing callback function(s):\n" + missingFunc);

    m_server->async_run(1);
}

void BenchmarkServer::stop()
{
    m_server->stop();
}
