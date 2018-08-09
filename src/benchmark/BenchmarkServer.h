#ifndef BENCHMARK_SERVER_H
#define BENCHMARK_SERVER_H

#include "fbksd/core/Definitions.h"
#include "fbksd/core/RPC.h"
#include <QObject>
#include <QBuffer>
#include <vector>
#include <QSharedMemory>
class BenchmarkManager;

/**
 * \defgroup BenchmarkServer Benchmark Server Application
 * @{
 */

/**
 * This class implements the benchmark server.
 */
class BenchmarkServer: public RPCServer
{
public:
    BenchmarkServer(BenchmarkManager *);

private:
    void getSceneInfo(QDataStream&, QDataStream&);
    void setSampleLayout(QDataStream&, QDataStream&);
    void evaluateSamples(QDataStream&, QDataStream&);
    void evaluateSamplesCrop(QDataStream&, QDataStream&);
    void evaluateSamplesPDF(QDataStream&, QDataStream&);
    void sendResult(QDataStream&, QDataStream&);

    BenchmarkManager *manager;
};

/**@}*/

#endif // BENCHMARKSERVER

