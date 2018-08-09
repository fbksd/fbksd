#include "RenderClient.h"
#include "BenchmarkManager.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QBuffer>

RenderClient::RenderClient(BenchmarkManager *manager, int port):
    RPCClient(port, true)
{
    this->manager = manager;
}

SceneInfo RenderClient::getSceneInfo()
{
    SceneInfo scene;

    call("GET_SCENE_DESCRIPTION", 0,
        [](QDataStream&){},
        [&](QDataStream& inStream)
        {
            inStream >> scene;
        }
    );

    return scene;
}

void RenderClient::detachMemory()
{
    call("DETACH_MEMORY", 0,
        [](QDataStream&){},
        [](QDataStream&){}
    );
}

void RenderClient::setParameters(int maxSPP, const SampleLayout& layout)
{
    quint64 msgSize = 0;
    msgSize += sizeof(int);
    msgSize += SerialSize()(layout);

    call("SET_PARAMETERS", msgSize,
        [&](QDataStream& dataStream)
        {
            dataStream << maxSPP;
            dataStream << layout;
        },
        [](QDataStream&){}
    );
}

int RenderClient::evaluateSamples(bool isSPP, int numSamples)
{
    int numGenSamples = 0;

    quint64 msgSize = 0;
    msgSize += sizeof(bool); // isSpp
    msgSize += sizeof(int); // numSamples

    call("EVALUATE_SAMPLES", msgSize,
        [&](QDataStream& dataStream)
        {
            dataStream << isSPP;
            dataStream << numSamples;
        },
        [&](QDataStream& inDataStream)
        {
            inDataStream >> numGenSamples;
        }
    );

    return numGenSamples;
}

int RenderClient::evaluateSamples(bool isSPP, int numSamples, const CropWindow &crop)
{
    int numGenSamples = 0;

    quint64 msgSize = 0;
    msgSize += sizeof(bool); // isSpp
    msgSize += sizeof(int); // numSamples
    msgSize += 4*sizeof(int); // crop

    call("EVALUATE_SAMPLES_CROP", msgSize,
        [&](QDataStream& dataStream)
        {
            dataStream << isSPP;
            dataStream << numSamples;
            dataStream << crop;
        },
        [&](QDataStream& inDataStream)
        {
            inDataStream >> numGenSamples;
        }
    );

    return numGenSamples;
}

int RenderClient::evaluateAdaptiveSamples(bool isSPP, int numSamples)
{
    int numGenSamples = 0;

    quint64 msgSize = 0;
    msgSize += sizeof(bool); // isSpp
    msgSize += sizeof(int); // numSamples

    call("EVALUATE_SAMPLES_PDF", msgSize,
        [&](QDataStream& dataStream)
        {
            dataStream << isSPP;
            dataStream << numSamples;
        },
        [&](QDataStream& inDataStream)
        {
            inDataStream >> numGenSamples;
        }
    );

    return numGenSamples;
}

void RenderClient::finishRender()
{
    call("FINISH_RENDER", 0, [](QDataStream&){}, [](QDataStream&){});
}
