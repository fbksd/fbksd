#ifndef DEFINITIONS
#define DEFINITIONS

#include "SampleLayout.h"
#include "SceneInfo.h"
#include <QDataStream>


/**
 * \defgroup Core Benchmark Core Library
 * @{
 */

/**
 * \brief The CropWindow struct defines a window for sampling evaluation.
 *
 * Passing a CropWindow to evaluateSamples() makes the rendering system
 * generate/evaluate samples only inside that window.
 */
struct CropWindow
{
    int width()
    { return endX - beginX; }

    int height()
    { return endY - beginY; }

    int beginX, beginY;
    int endX, endY;

private:
    friend QDataStream& operator<<(QDataStream& stream, const CropWindow& crop)
    {
        stream << crop.beginX << crop.beginY << crop.endX << crop.endY;
        return stream;
    }

    friend QDataStream& operator>>(QDataStream& stream, CropWindow& crop)
    {
        stream >> crop.beginX >> crop.beginY >> crop.endX >> crop.endY;
        return stream;
    }
};

/**@}*/

#endif // DEFINITIONS

