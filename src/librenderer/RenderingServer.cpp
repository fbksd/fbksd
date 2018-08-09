#include "fbksd/renderer/RenderingServer.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTextStream>
#include <QBuffer>


RenderingServer::RenderingServer() :
    samplesMemory("SAMPLES_MEMORY"),
    pdfMemory("PDF_MEMORY")
{
    registerCall("GET_SCENE_DESCRIPTION", [this](QDataStream& in, QDataStream& out){ _getSceneInfo(in, out); });
    registerCall("SET_PARAMETERS", [this](QDataStream& in, QDataStream& out){ _setParameters(in, out); });
    registerCall("DETACH_MEMORY", [this](QDataStream& in, QDataStream& out){ _detachMemory(in, out); });
    registerCall("EVALUATE_SAMPLES", [this](QDataStream& in, QDataStream& out){ _evaluateSamples(in, out); });
    registerCall("EVALUATE_SAMPLES_CROP", [this](QDataStream& in, QDataStream& out){ _evaluateSamplesCrop(in, out); });
    registerCall("EVALUATE_SAMPLES_PDF", [this](QDataStream& in, QDataStream& out){ _evaluateSamplesPDF(in, out); });
    registerCall("FINISH_RENDER", [this](QDataStream& in, QDataStream& out){ _finishRender(in, out); });
}

void RenderingServer::_getSceneInfo(QDataStream&, QDataStream& outStream)
{
    SceneInfo scene;
    emit getSceneInfo(&scene);

    pixelCount = scene.get<int>("width") * scene.get<int>("height");

    SerialSize serialSize;
    quint64 msgSize = serialSize(scene);
    outStream << msgSize;
    outStream << scene;
}

void RenderingServer::_detachMemory(QDataStream &, QDataStream& outStream)
{
    samplesMemory.detach();
    outStream << quint64(0);
}

void RenderingServer::_setParameters(QDataStream& inDataStream, QDataStream& outStream)
{
    int maxSPP = 0;
    inDataStream >> maxSPP;
    SampleLayout layout;
    inDataStream >> layout;

    if(!samplesMemory.attach())
        qDebug() << samplesMemory.error().c_str();
    if(!pdfMemory.attach())
        qDebug() << pdfMemory.error().c_str();

    SamplesPipe::setLayout(layout);
    SamplesPipe::samples = static_cast<float*>(samplesMemory.data());

    emit setParameters(maxSPP, layout,
                       static_cast<float*>(samplesMemory.data()),
                       static_cast<float*>(pdfMemory.data()) );

    outStream << quint64(0);
}

void RenderingServer::_evaluateSamples(QDataStream& inDataStream, QDataStream& outStream)
{
    bool isSPP = false;
    inDataStream >> isSPP;
    int numSamples = 0;
    inDataStream >> numSamples;

    SamplesPipe::numSamples = isSPP ? numSamples * pixelCount : numSamples;

    int resultSize = 0;
    emit evaluateSamples(isSPP, numSamples, &resultSize);

    quint64 msgSize = 0;
    msgSize += sizeof(int); // resultSize

    outStream << msgSize;
    outStream << resultSize;
}

void RenderingServer::_evaluateSamplesCrop(QDataStream& inDataStream, QDataStream& outStream)
{
    bool isSPP = false;
    inDataStream >> isSPP;
    int numSamples = 0;
    inDataStream >> numSamples;
    CropWindow crop;
    inDataStream >> crop;

    int resultSize = 0;
    emit evaluateSamplesCrop(isSPP, numSamples, crop, &resultSize);

    quint64 msgSize = 0;
    msgSize += sizeof(int); // resultSize

    outStream << msgSize;
    outStream << resultSize;
}

void RenderingServer::_evaluateSamplesPDF(QDataStream& inDataStream, QDataStream& outStream)
{
    bool isSPP = false;
    inDataStream >> isSPP;
    int numSamples = 0;
    inDataStream >> numSamples;

    int resultSize = 0;
    emit evaluateSamplesPDF(isSPP, numSamples, static_cast<float*>(pdfMemory.data()), &resultSize);

    quint64 msgSize = 0;
    msgSize += sizeof(int); // resultSize

    outStream << msgSize;
    outStream << resultSize;
}

void RenderingServer::_finishRender(QDataStream &, QDataStream &outStream)
{
    emit finishRender();
    outStream << quint64(0);
}
