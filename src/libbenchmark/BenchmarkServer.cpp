/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "BenchmarkServer.h"
#include "BenchmarkManager.h"
#include "version.h"
#include <rpc/server.h>
using namespace fbksd;


BenchmarkServer::BenchmarkServer():
    m_server(std::make_unique<rpc::server>("127.0.0.1", 2226))
{
    m_server->bind("GET_VERSION", []()
    { return std::make_pair(FBKSD_VERSION_MAJOR, FBKSD_VERSION_MINOR); });
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

void BenchmarkServer::onGetNextTile(const GetNextTile &callback)
{
    m_getNextTile = true;
    m_server->bind("GET_NEXT_TILE", callback);
}

void BenchmarkServer::onEvaluateInputSamples(const EvaluateSamples &callback)
{
    m_server->bind("EVALUATE_INPUT_SAMPLES", callback);
}

void BenchmarkServer::onGetNextInputTile(const GetNextInputTile &callback)
{
    m_server->bind("GET_NEXT_INPUT_TILE", callback);
}

void BenchmarkServer::onLastTileConsumed(const LastTileConsumed &callback)
{
    m_lastTileConsumed = true;
    m_server->bind("LAST_TILE_CONSUMED", callback);
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
    if(!m_getNextTile)
        missingFunc += "GetNextTile ";
    if(!m_lastTileConsumed)
        missingFunc += "LastTileConsumed ";
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
