
#include "BenchmarkServer.h"

#include "BenchmarkManager.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTextStream>
#include <QBuffer>


BenchmarkServer::BenchmarkServer(BenchmarkManager *manager)
{
    this->manager = manager;

    registerCall("EVALUATE_SAMPLES", [this](QDataStream& in, QDataStream& out){ evaluateSamples(in, out); });
    registerCall("EVALUATE_SAMPLES_CROP", [this](QDataStream& in, QDataStream& out){ evaluateSamplesCrop(in, out); });
    registerCall("EVALUATE_SAMPLES_PDF", [this](QDataStream& in, QDataStream& out){ evaluateSamplesPDF(in, out); });
    registerCall("GET_SCENE_DESCRIPTION", [this](QDataStream& in, QDataStream& out){ getSceneInfo(in, out); });
    registerCall("SET_SAMPLE_LAYOUT", [this](QDataStream& in, QDataStream& out){ setSampleLayout(in, out); });
    registerCall("SEND_RESULT", [this](QDataStream& in, QDataStream& out){ sendResult(in, out); });

    registerCall("GET_MODE", [this](QDataStream&, QDataStream& out)
    {
        bool mode = false;
        out << (quint64)sizeof(bool);
        out << mode;
    });
}

void BenchmarkServer::getSceneInfo(QDataStream &, QDataStream& outStream)
{
    SceneInfo desc = manager->getSceneInfo();

    SerialSize serialSize;
    quint64 msgSize = serialSize(desc);
    outStream << msgSize;
    outStream << desc;
}

void BenchmarkServer::setSampleLayout(QDataStream& inDataStream, QDataStream& outStream)
{
    SampleLayout layout;
    inDataStream >> layout;
    int returnCode = manager->setSampleLayout(layout);

    quint64 msgSize = 0;
    msgSize += sizeof(int);
    outStream << msgSize;
    outStream << returnCode;
}

void BenchmarkServer::evaluateSamples(QDataStream& inDataStream, QDataStream& outStream)
{
    bool isSpp = false;
    inDataStream >> isSpp;
    int numSamples = 0;
    inDataStream >> numSamples;

    int numCompSamples = manager->evaluateSamples(isSpp, numSamples);

    quint64 msgSize = 0;
    msgSize += sizeof(int); // numCompSamples

    outStream << msgSize;
    outStream << numCompSamples;
}

void BenchmarkServer::evaluateSamplesCrop(QDataStream& inDataStream, QDataStream& outStream)
{
    bool isSpp = false;
    inDataStream >> isSpp;
    int numSamples = 0;
    inDataStream >> numSamples;
    CropWindow crop;
    inDataStream >> crop;

    int numCompSamples = manager->evaluateSamples(isSpp, numSamples, crop);

    quint64 msgSize = 0;
    msgSize += sizeof(int); // numCompSamples

    outStream << msgSize;
    outStream << numCompSamples;
}

void BenchmarkServer::evaluateSamplesPDF(QDataStream& inDataStream, QDataStream& outStream)
{
    bool isSpp = false;
    inDataStream >> isSpp;
    int numSamples = 0;
    inDataStream >> numSamples;

    int numCompSamples = manager->evaluateAdaptiveSamples(isSpp, numSamples);

    quint64 msgSize = 0;
    msgSize += sizeof(int); // numCompSamples

    outStream << msgSize;
    outStream << numCompSamples;
}

void BenchmarkServer::sendResult(QDataStream&, QDataStream& outStream)
{
    manager->sendResult();
    outStream << quint64(0);
}
