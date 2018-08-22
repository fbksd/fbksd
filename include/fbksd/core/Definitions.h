#ifndef DEFINITIONS
#define DEFINITIONS

#include "SampleLayout.h"
#include "SceneInfo.h"


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
};

/**@}*/

#endif // DEFINITIONS

