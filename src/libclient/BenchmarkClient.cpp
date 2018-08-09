#include "fbksd/client/BenchmarkClient.h"
#include "fbksd/core/Definitions.h"

#include <iostream>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QBuffer>
#include <QDataStream>


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
    RPCClient(2226),
    samplesMemory("SAMPLES_MEMORY"),
    pdfMemory("PDF_MEMORY"),
    resultMemory("RESULT_MEMORY")
{
    if(samplesMemory.isAttached())
        exit(1);

    maxNumSamples = 0;

    bool isTest = false;
    call("GET_MODE", 0, [](QDataStream&){}, [&](QDataStream& inDataStream)
    {
        inDataStream >> isTest;
    });

    if(isTest)
        test();
    else
        fetchSceneInfo();
}

BenchmarkClient::~BenchmarkClient()
{}

SceneInfo BenchmarkClient::getSceneInfo()
{
    return sceneInfo;
}

void BenchmarkClient::setSampleLayout(const SampleLayout& layout)
{
    int returnCode = 0;

    SerialSize serialSize;
    call("SET_SAMPLE_LAYOUT", serialSize(layout),
        [&](QDataStream& dataStream)
        {
            dataStream << layout;
        },
        [&](QDataStream& dataStream)
        {
            dataStream >> returnCode;
        }
    );

    if(returnCode != 0)
        exit(1);

    if(!samplesMemory.attach())
    {
        qDebug() << samplesMemory.error().c_str();
        exit(1);
    }
}

float *BenchmarkClient::getSamplesBuffer()
{
    return static_cast<float*>(samplesMemory.data());
}

float *BenchmarkClient::getPdfBuffer()
{
    return static_cast<float*>(pdfMemory.data());
}

float *BenchmarkClient::getResultBuffer()
{
    return static_cast<float*>(resultMemory.data());
}

int BenchmarkClient::evaluateSamples(SamplesCountUnit unit, int numSamples)
{
    int numGenSamples = 0;

    quint64 msgSize = 0;
    msgSize += sizeof(bool); // unit
    msgSize += sizeof(int); // nSamples

    call("EVALUATE_SAMPLES", msgSize,
        [&](QDataStream& dataStream)
        {
            dataStream << (bool)unit;
            dataStream << numSamples;
        },
        [&](QDataStream& inDataStream)
        {
            numGenSamples = 0;
            inDataStream >> numGenSamples;
        }
    );

    return numGenSamples;
}

int BenchmarkClient::evaluateSamples(SamplesCountUnit unit, int numSamples, const CropWindow &crop)
{
    int numGenSamples = 0;

    quint64 msgSize = 0;
    msgSize += sizeof(bool);  // unit
    msgSize += sizeof(int);   // nSamples
    msgSize += sizeof(int)*4; // crop

    call("EVALUATE_SAMPLES_CROP", msgSize,
        [&](QDataStream& dataStream)
        {
            dataStream << (bool)unit;
            dataStream << numSamples;
            dataStream << crop;
        },
        [&](QDataStream& inDataStream)
        {
            numGenSamples = 0;
            inDataStream >> numGenSamples;
        }
    );

    return numGenSamples;
}

int BenchmarkClient::evaluateAdaptiveSamples(SamplesCountUnit unit, int numSamples)
{
    int numGenSamples = 0;

    quint64 msgSize = 0;
    msgSize += sizeof(bool); // unit
    msgSize += sizeof(int); // nSamples

    call("EVALUATE_SAMPLES_PDF", msgSize,
        [&](QDataStream& dataStream)
        {
            dataStream << (bool)unit;
            dataStream << numSamples;
        },
        [&](QDataStream& inDataStream)
        {
            numGenSamples = 0;
            inDataStream >> numGenSamples;
        }
    );

    return numGenSamples;
}

void BenchmarkClient::sendResult()
{
    quint64 msgSize = 0;
    call("SEND_RESULT", msgSize, [](QDataStream&){}, [](QDataStream&){});
}

void BenchmarkClient::test()
{
    bool boolVar = true;
    int intVar = 123;
    quint64 qint32Var = 456;
    float floatVar = 123.546f;
    float floatArrayVar[10];
    for(int i = 0; i < 10; ++i)
    {
        floatArrayVar[i] = (i + 0.5f);
    }

    quint64 msgSize = 0;
    msgSize += sizeof(bool);
    msgSize += sizeof(int);
    msgSize += sizeof(quint64);
    msgSize += sizeof(float);
    msgSize += sizeof(float) * 10;

    call("TEST", msgSize,
        [&](QDataStream& dataStream)
        {
            dataStream << boolVar;
            dataStream << intVar;
            dataStream << qint32Var;
            dataStream << floatVar;
            dataStream.writeRawData((char*)floatArrayVar, 10*sizeof(float));
        },
        [&](QDataStream& inDataStream)
        {
            inDataStream >> boolVar;
            inDataStream >> intVar;
            inDataStream >> qint32Var;
            inDataStream >> floatVar;
            inDataStream.readRawData((char*)floatArrayVar, 10*sizeof(float));
        }
    );

    bool passed = true;
    if((boolVar == false) && (intVar == 321) && (qint32Var == 654) && (floatVar == 654.321f))
    {
        for(int i = 0; i < 10; ++i)
        {
            if(floatArrayVar[i] != (10 - i + 0.5f))
            {
                passed = false;
                break;
            }
        }
    }

    if(passed)
        call("CLIENT_OK", 0, [](QDataStream&){}, [](QDataStream&){});
    else
        call("CLIENT_ERROR", 0, [](QDataStream&){}, [](QDataStream&){});

    exit(0);
}

void BenchmarkClient::fetchSceneInfo()
{
    call("GET_SCENE_DESCRIPTION", 0, [](QDataStream&){},
        [&](QDataStream& inDataStream)
        {
            inDataStream >> sceneInfo;
        }
    );

    maxNumSamples = sceneInfo.get<int>("max_samples");

    if(!pdfMemory.attach())
        qDebug() << pdfMemory.error().c_str();
    if(!resultMemory.attach())
        qDebug() << resultMemory.error().c_str();
}
