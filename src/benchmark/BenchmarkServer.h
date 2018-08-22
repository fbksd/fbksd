#ifndef BENCHMARK_SERVER_H
#define BENCHMARK_SERVER_H

#include "fbksd/core/Definitions.h"
#include <QObject>
#include <QBuffer>
#include <vector>
#include <QSharedMemory>

class BenchmarkManager;
namespace rpc { class server; }

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
    BenchmarkServer(BenchmarkManager *);

    ~BenchmarkServer();

    void run();

    void stop();

private:
    SceneInfo getSceneInfo();
    int setSampleLayout(const SampleLayout& layout);
    int evaluateSamples(bool isSpp, int numSamples);
    void sendResult();

    std::unique_ptr<rpc::server> m_server;
    BenchmarkManager *manager;
};

/**@}*/

#endif // BENCHMARKSERVER

