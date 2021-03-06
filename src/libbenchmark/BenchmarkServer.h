/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef BENCHMARK_SERVER_H
#define BENCHMARK_SERVER_H

#include "fbksd/core/definitions.h"
#include <functional>

namespace rpc { class server; }

namespace fbksd
{

class BenchmarkManager;

/**
 * \defgroup BenchmarkServer Benchmark Server Application
 * @{
 */

/**
 * This class implements the benchmark server.
 */
class BenchmarkServer
{
public:
    using GetSceneInfo
        = std::function<SceneInfo()>;
    using SetParameters
        = std::function<void(const SampleLayout& layout)>;
    using EvaluateSamples
        = std::function<TilePkg(bool isSPP, int64_t numSamples)>;
    using GetNextTile
        = std::function<TilePkg(int64_t prevTileIndex)>;
    using GetNextInputTile
        = std::function<TilePkg(int64_t prevTileIndex, bool prevWasInput)>;
    using LastTileConsumed
        = std::function<void(int64_t tileIndex)>;
    using SendResult
        = std::function<void()>;

    BenchmarkServer();

    ~BenchmarkServer();

    void onGetSceneInfo(const GetSceneInfo& callback);

    void onSetParameters(const SetParameters& callback);

    void onEvaluateSamples(const EvaluateSamples& callback);

    void onGetNextTile(const GetNextTile& callback);

    void onEvaluateInputSamples(const EvaluateSamples& callback);

    void onGetNextInputTile(const GetNextInputTile& callback);

    void onLastTileConsumed(const LastTileConsumed& callback);

    void onSendResult(const SendResult& callback);

    void run();

    void stop();

private:
    std::unique_ptr<rpc::server> m_server;
    bool m_getSceneInfoSet = false;
    bool m_setParametersSet = false;
    bool m_evalSamplesSet = false;
    bool m_getNextTile = false;
    bool m_lastTileConsumed = false;
    bool m_sendResultSet = false;
};

/**@}*/

} // namespace fbksd

#endif // BENCHMARKSERVER

