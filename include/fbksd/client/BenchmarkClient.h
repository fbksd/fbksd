/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef BENCHMARKCLIENT_H
#define BENCHMARKCLIENT_H

#include "fbksd/core/definitions.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>


namespace fbksd
{

/**
 * \file
 * \brief File used by the user to communicate with the benchmark server.
 */

/**
 * \defgroup BenchmarkClient Benchmark Client Library
 * @{
 */


/**
 * @brief Helper class for accessing samples in a tile.
 *
 * A BufferTile is basically a wrapper for a tile of memory used to transfer sample data
 * between client and server.
 *
 * When using the SPP variants of the BenchmarkClient sample evaluation methods, this wrapper
 * is used to provide a more convenient way of accessing samples using `(x,y)` pixel positions.
 */
class BufferTile
{
public:
    /**
     * @brief Returns a pointer to the sample s of the pixel (x,y).
     *
     * @param x Pixel x value (beginX() <= x < endX()).
     * @param y Pixel y value (beginY() <= y < endY()).
     * @param s Sample number (0 <= y < getSPP()).
     */
    float* operator()(int64_t x, int64_t y, int64_t s) const
    {
        int64_t index = (x - m_x)*m_size*m_spp + (y - m_y)*width()*m_spp*m_size + s*m_size;
        return &m_data[index];
    }

    /**
     * @brief Returns a pointer to the sample s of the pixel (x,y).
     *
     * This is a overload method that accepts a generic "Point" object as
     * the pixel position (Point.x and Point.y must be valid).
     */
    template<typename T>
    float* operator()(const T& p, int64_t s)
    { return (*this)(p.x, p.y, s); }

    /**
     * @brief Returns the starting x position of the tile.
     */
    int64_t beginX() const { return m_x; }

    /**
     * @brief Returns the end x position of the tile.
     */
    int64_t endX() const { return m_ex; }

    /**
     * @brief Returns the starting y position of the tile.
     */
    int64_t beginY() const { return m_y; }

    /**
     * @brief Returns the end y position of the tile.
     */
    int64_t endY() const { return m_ey; }

    /**
     * @brief Returns the tile width.
     */
    int64_t width() const { return m_ex - m_x; }

    /**
     * @brief Returns the tile height.
     */
    int64_t height() const { return m_ey - m_y; }

    /**
     * @brief Returns the total number of pixels of the tile.
     */
    int64_t numPixels() const { return (m_ex - m_x)*(m_ey - m_y); }

    /**
     * @brief Returns the number of samples-per-pixel (SPP).
     */
    int64_t getSPP() const { return m_spp; }

    /**
     * @brief Returns the sample size (number of float values).
     */
    int64_t getSampleSize() const { return m_size; }

    /**
     * @brief Sample iterator for the BufferTile.
     *
     * This provides a convenient way of traversing all samples in the tile
     * without using (x, y) pixel positions.
     */
    class SampleIterator
    {
    public:
        SampleIterator(float* data, int64_t size): m_data(data), m_sampleSize(size) {}

        SampleIterator& operator++() { m_data += m_sampleSize; return *this; }
        bool operator==(const SampleIterator& it) const { return m_data == it.m_data; }
        bool operator!=(const SampleIterator& it) const { return m_data != it.m_data; }
        const float* operator*() const { return m_data; }

    private:
        float* m_data;
        int64_t m_sampleSize;
    };

    /**
     * @brief Returns an iterator for first sample in this tile.
     */
    SampleIterator begin() const { return SampleIterator(m_data, m_size); }

    /**
     * @brief Returns a past-the-end iterator for this tile.
     */
    SampleIterator end() const { return SampleIterator(m_dataEnd, m_size); }

private:
    friend class BenchmarkClient;

    BufferTile(int64_t x, int64_t ex,
               int64_t y, int64_t ey,
               int64_t sampleSize,
               int64_t spp,
               float* data);

    int64_t m_x, m_ex, m_y, m_ey, m_size, m_spp;
    float* m_data;
    float* m_dataEnd;
};


/**
 * @brief Represents a number of samples given in samples-per-pixel.
 */
class SPP
{
public:
    explicit SPP(int64_t value);

    int64_t operator=(int64_t v);

    void setValue(int64_t v);

    int64_t getValue() const;

private:
    int64_t m_value = 0;
};


/**
 * \brief The BenchmarkClient class is used to communicate with the benchmark server.
 *
 * This class provides query methods that allows a technique to get information about the scene,
 * request samples to be rendered, end send the final result.
 *
 * Only one instance of this class should be created in your program.
 */
class BenchmarkClient
{
public:
    using TileConsumer =
        std::function<void(const BufferTile&)>; //!< Tile consumer callback function (SPP version).
    using TileProducer =
        std::function<void(const BufferTile&)>; //!< Tile producer callback function (SPP version).
    using TileConsumer2 =
        std::function<void(int64_t count, float* samples)>; //!< Tile consumer callback function (non-SPP version).
    using TileProducer2 =
        std::function<void(int64_t count, float* samples)>; //!< Tile producer callback function (non-SPP version).

    /**
     * \brief Connects with the benchmark server.
     *
     * The BenchmarkClient object should be instantiated at the beginning of the main function.
     *
     * If you pass the argc and argv parameters from main(), the following command-line options
     * become available:
     *
     *     --fbksd-renderer "<renderer_exec> <renderer_args> ..."
     *       Starts the renderer process with the given arguments.
     *     --fbksd-spp <value>
     *       Sets the sample budget available to client.
     *
     * This allows you to run your client program directly (for debugging purposes, for example).
     */
    BenchmarkClient(int argc = 0, char* argv[] = nullptr);

    BenchmarkClient(const BenchmarkClient&) = delete;

    BenchmarkClient(BenchmarkClient&&) = default;

    ~BenchmarkClient();

    /**
     * \brief Get information about the scene being rendered.
     *
     * The more important information are:
     * - image dimensions (width and height);
     * - maximum number of samples available.
     *
     * This information allows you to configure your technique accordingly.
     * See SceneInfo for more details.
     *
     * \return SceneInfo
     */
    SceneInfo getSceneInfo();

    /**
     * \brief Sets the sample layout.
     *
     * The layout is the way to inform the FBKSD server
     * what data your technique requires for each sample, and how the data should
     * be laid out in memory.
     * Common data includes, for example, color RGB and (x,y) image plane position.
     *
     * This method should be called just once.
     */
    void setSampleLayout(const SampleLayout& layout);

    /**
     * \brief Returns a pointer to the buffer where the result image is stored.
     *
     * The user should write the final reconstructed image to this buffer before calling
     * sendResult();
     *
     * The layout of the image in memory follows a scan-line pixel order:
     * matrix of pixels with `width` columns, and `height` lines, with each pixel having
     * R, G, and B values, in this order.
     *
     * @note BenchmarkClient owns the buffer: do not delete.
     */
    float* getResultBuffer();

    /**
     * @brief Request samples.
     *
     * The samples are computed by the renderer and sent in tiles. For each tile, the callback
     * function BenchmarkClient::TileConsumer is called so you can consume the samples.
     *
     * @param spp
     * Number of samples as a multiple of the number of pixels.
     * @param consumer
     * Callback function that will be called for each tile produced by the renderer.
     */
    void evaluateSamples(SPP spp, const TileConsumer& consumer);

    /**
     * @brief Request samples.
     *
     * This is an overload method that request a amount of samples that is not in SPP.
     * Since the amount can be less then the number of pixels, there is no guarantee
     * that all pixels are covered by samples.
     *
     * For each tile, the callback function BenchmarkClient::TileConsumer2 is called so
     * you can consume the samples.
     *
     * @param numSamples
     * Number of samples requested.
     * @param consumer
     * Callback function that will be called for each tile produced by the renderer.
     */
    void evaluateSamples(int64_t numSamples, const TileConsumer2& consumer);

    /**
     * @brief Request samples with input values.
     *
     * This is the method you'll use to implement adaptive techniques.
     *
     * As opposed to the evaluateSamples() methods, this method allows samples with
     * INPUT random parameters. The producer callback is called for each tile to allow
     * you to write the input values.
     *
     * Once the renderer computes the sample results, the tile is passed to the consumer
     * callback so you can read them.
     *
     * @param spp
     * Number of samples as a multiple of the number of pixels.
     * @param producer
     * Callback function that will be called for each tile required by the renderer.
     * @param consumer
     * Callback function that will be called for each tile produced by the renderer.
     */
    void evaluateInputSamples(SPP spp,
                              const TileProducer& producer,
                              const TileConsumer& consumer);

    /**
     * @brief Request samples with input values.
     *
     * This is an overload method that request a amount of samples that is not in SPP.
     */
    void evaluateInputSamples(int64_t numSamples,
                              const TileProducer2& producer,
                              const TileConsumer2& consumer);

    /**
     * \brief Sends the final result (rgb image)
     *
     * Calling this method is must be the last method you do before you exiting.
     */
    void sendResult();

    BenchmarkClient& operator=(const BenchmarkClient&) = delete;

    BenchmarkClient& operator=(BenchmarkClient&&) = default;

private:
    struct Imp;
    std::unique_ptr<Imp> m_imp;
};

/**@}*/

} // namespace fbksd

#endif
