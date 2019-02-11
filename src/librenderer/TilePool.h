#ifndef TILEPOOL_H
#define TILEPOOL_H

#include "fbksd/renderer/SamplesPipe.h"
#include "fbksd/core/definitions.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace fbksd
{

#ifndef EXPORT_LIB
#define EXPORT_LIB __attribute__((visibility("default")))
#endif

/**
 * @brief Manages a shared memory region in tiles.
 */
class EXPORT_LIB TilePool
{
public:
    /**
     * @brief Initializes the pool.
     *
     * This must be called once in a EVALUTE_SAMPLES before the the
     * the renderer callback is called.
     *
     * @param numSamples
     * Total number of samples requested by the client.
     * @param tileNumSamples
     * Maximum number of samples that fits in a tile.
     * @param sampleSize
     * Number of float values in a sample.
     * @param samples
     * Pointer to the shared memory block.
     * @param waitInput
     * true if the client has input samples.
     */
    static void init(int64_t numSamples,
                     int64_t tileNumSamples,
                     int sampleSize,
                     float* samples,
                     bool waitInput);

    /**
     * @brief Get hold of a free tile to start working on it.
     *
     * If the init() was called with waitInput true, the to-be acquired tile
     * needs input from the client. This means that this method will only return
     * when another thread: acquires the tile using getClientTile(), sent it to
     * the client, and then releases it using releaseInputTile().
     *
     * @param begin
     * Tile starting pixel.
     * @param end
     * Tile last pixel.
     * @param numSamples
     * Number of samples that will be inserted in the tile.
     */
    static float* getFreeTile(const Point2l& begin, const Point2l& end, int64_t numSamples);

    /**
     * @brief Returns a tile to be sent to the client.
     *
     * The returned tile can be an input tile (request input values from the client),
     * or a worked tile (the renderer finished rendering it).
     *
     * This call blocks until a tile is available.
     *
     * @param[out] hasNext
     * Is true if the returned tile is not the last one.
     * @param[out] isInput
     * Is true if the returned tile is a input request tile. Input tiles must be released
     * using the releaseInputTile() method (instead of the releaseWorkedTile() method).
     */
    static fbksd::Tile getClientTile(bool& hasNext, bool& isInput);

    /**
     * @brief Releases an input tile.
     *
     * The thread waiting on getFreeTile() for the tile is unblocked.
     */
    static void releaseInputTile(int index);

    /**
     * @brief Releases a worked tile.
     *
     * A worked tile is released when the renderer finished rendering it.
     * The tile becomes available to getClientTile().
     */
    static void releaseWorkedTile(const SamplesPipe& pipe);

    /**
     * @brief Releases a consumed tile.
     *
     * This informs that the corresponding memory for the tile is free
     * to be used again by calling getFreeTile().
     */
    static void releaseConsumedTile(int index);

private:
    static float* sm_samples;
    static int64_t sm_tileNumSamples;
    static int sm_numTiles;
    static std::vector<int> sm_freeIndices; // indexes free to be written by the server
    static std::queue<fbksd::Tile> sm_waitingInputTiles;
    static std::queue<int> sm_readyInputTiles;
    static std::queue<fbksd::Tile> sm_workedIndices; // indexes waiting to be consumed by the client
    static std::mutex sm_mutex;
    static std::condition_variable sm_hasFreeTile;
    static std::condition_variable sm_hasTileForClient;
    static std::condition_variable sm_isInputReady;
    static int64_t sm_numSamples;
    static int64_t sm_numWorkedSamples;
    static bool sm_waitInput;
};

} // namespace fbksd

#endif // TILEPOOL_H
