#ifndef RENDERINGSERVER_H
#define RENDERINGSERVER_H

#include "samples.h"
#include "fbksd/core/Definitions.h"
#include "fbksd/core/RPC.h"
#include "fbksd/core/SharedMemory.h"
#include <map>
#include <memory>
#include <QObject>


/**
 * \defgroup RenderingServer Rendering Server Library
 * @{
 */


/**
 * \brief The RenderingServer class implements the server by which a rendering system provides sampling
 * computations to the benchmark system.
 *
 * This class provides an asynchronous communication interface using signals. Each time a request is received,
 * the appropriate signal is emitted.
 *
 */
class RenderingServer : public RPCServer
{
    Q_OBJECT
public:
    RenderingServer();

signals:
    /**
     * \brief Requests information about the scene.
     */
    void getSceneInfo(SceneInfo* scene);

    void setParameters(int maxSPP, const SampleLayout& layout, float* samplesBuffer, float* pdfBuffer);

    void evaluateSamples(bool isSPP, int numSamples, int* resultSize);

    void evaluateSamplesCrop(bool isSPP, int numSamples, const CropWindow& crop, int* resultSize);

    void evaluateSamplesPDF(bool isSPP, int numSamples, const float* pdf, int* resultSize);

    void finishRender();

private:
    void _getSceneInfo(QDataStream&, QDataStream&);
    void _detachMemory(QDataStream&, QDataStream&);
    void _setParameters(QDataStream&, QDataStream&);
    void _evaluateSamples(QDataStream&, QDataStream&);
    void _evaluateSamplesCrop(QDataStream&, QDataStream&);
    void _evaluateSamplesPDF(QDataStream&, QDataStream&);
    void _finishRender(QDataStream&, QDataStream&);

    SharedMemory samplesMemory;
    SharedMemory pdfMemory;
    int pixelCount;
};

/**@}*/

#endif // RENDERINGSERVER_H
