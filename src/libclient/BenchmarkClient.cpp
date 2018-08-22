#include "fbksd/client/BenchmarkClient.h"
#include "fbksd/core/Definitions.h"

#include <rpc/client.h>
#include <iostream>


MSGPACK_ADD_ENUM(BenchmarkClient::SamplesCountUnit);


/*
 * ===== Note about the use of shared memory ======
 *
 * On Linux, if a process attached to a shared memory block crashes,
 * the memory will not be correctly detached, causing the memory block to not be
 * released by the SO. This will prevent the creation of the block with
 * the same key on a subsequent run of the system.
 *
 * In this case, you have to manually delete the "zombie" shared memory block:
 *  - run 'ipcs -m' end look for a block with zero nattch;
 *  - run 'ipcrm -m <shmid>' to delete the block.
 *
 * You can also see what processes are using the shm blocks:
 *  - 'lsof | egrep "<shmid>|COMMAND"'.
 *
 * This command deletes blocks with zero nattch automatically:
 * $ ipcs -m | awk '$6 == 0' | tr -s " " | cut -d " " -f 2 | xargs -L1 ipcrm -m
 */


BenchmarkClient::BenchmarkClient():
    m_client(std::make_unique<rpc::client>("127.0.0.1", 2226)),
    m_samplesMemory("SAMPLES_MEMORY"),
    m_pdfMemory("PDF_MEMORY"),
    m_resultMemory("RESULT_MEMORY")
{
    if(m_samplesMemory.isAttached())
        exit(1);

    m_maxNumSamples = 0;

//    bool isTest = false;
//    call("GET_MODE", 0, [](QDataStream&){}, [&](QDataStream& inDataStream)
//    {
//        inDataStream >> isTest;
//    });

//    if(isTest)
//        test();
//    else
    fetchSceneInfo();
}

BenchmarkClient::~BenchmarkClient() = default;

SceneInfo BenchmarkClient::getSceneInfo()
{
    return m_sceneInfo;
}

void BenchmarkClient::setSampleLayout(const SampleLayout& layout)
{
    m_client->call("SET_SAMPLE_LAYOUT", layout);

    if(!m_samplesMemory.attach())
    {
        std::cerr << m_samplesMemory.error() << std::endl;
        exit(1);
    }
}

float *BenchmarkClient::getSamplesBuffer()
{
    return static_cast<float*>(m_samplesMemory.data());
}

float *BenchmarkClient::getPdfBuffer()
{
    return static_cast<float*>(m_pdfMemory.data());
}

float *BenchmarkClient::getResultBuffer()
{
    return static_cast<float*>(m_resultMemory.data());
}

int BenchmarkClient::evaluateSamples(SamplesCountUnit unit, int numSamples)
{
    return m_client->call("EVALUATE_SAMPLES", unit, numSamples).as<int>();
}

void BenchmarkClient::sendResult()
{
    return m_client->send("SEND_RESULT");
}

void BenchmarkClient::fetchSceneInfo()
{
    m_sceneInfo = m_client->call("GET_SCENE_DESCRIPTION").as<SceneInfo>();
    m_maxNumSamples = m_sceneInfo.get<int>("max_samples");

    if(!m_pdfMemory.attach())
        std::cerr << m_pdfMemory.error() << std::endl;
    if(!m_resultMemory.attach())
        std::cerr << m_resultMemory.error() << std::endl;
}
