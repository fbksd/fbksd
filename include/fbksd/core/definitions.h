/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef DEFINITIONS
#define DEFINITIONS

#include "SampleLayout.h"
#include "SceneInfo.h"
#include "Point.h"


namespace fbksd
{

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
    CropWindow() = default;

    CropWindow(Point2l begin, Point2l end):
        begin(begin),
        end(end)
    {}

    int64_t width()
    { return end.x - begin.x; }

    int64_t height()
    { return end.y - begin.y; }

    Point2l begin; //!< Upper left corner point.
    Point2l end;   //!< Bottom right corner point.

    MSGPACK_DEFINE_ARRAY(begin, end)
};


struct Tile
{
    Tile() = default;

    Tile(const CropWindow& w, int64_t i, int64_t n):
        window(w),
        index(i),
        numSamples(n)
    {}

    CropWindow window;
    int64_t index = 0;
    int64_t numSamples = 0;

    MSGPACK_DEFINE_ARRAY(window, index, numSamples)
};


struct TilePkg
{
    TilePkg() = default;

    TilePkg(const Tile& tile, bool hasNext, bool isInputRequest = false):
        tile(tile),
        isValid(true),
        hasNext(hasNext),
        isInputRequest(isInputRequest)
    {}

    Tile tile;
    bool isValid = false;
    bool hasNext = false;
    bool isInputRequest = false;

    MSGPACK_DEFINE_ARRAY(tile, isValid, hasNext, isInputRequest)
};

/**@}*/

} // namespace fbksd

#endif // DEFINITIONS

