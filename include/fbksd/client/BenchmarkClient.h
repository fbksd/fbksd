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
     * \brief Returns a pointer to the buffer where samples are stored.
     *
     * Calling evaluateSamples() makes the requested samples to be
     * written to the buffer.
     *
     * To pass `INPUT` samples, the user should write them in the buffer before
     * calling evaluateSamples().
     *
     * @note BenchmarkClient owns the buffer: do not delete.
     */
    float* getSamplesBuffer();

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
     * \brief Request samples.
     *
     * The samples are written to the buffer returned by getSamplesBuffer().
     *
     * `numSamples` should not exceed the maximum allowed sample budget.
     *
     * Note that since `numSamples` can be less then the number of pixels,
     * there is no guarantee that all pixels are covered by samples.
     *
     * When the sample layout has input elements, they should be passed in the corresponding
     * positions in the samples buffer.
     *
     * @returns Number of returned samples.
     */
    int64_t evaluateSamples(int64_t numSamples);

    /**
     * @brief Request samples.
     *
     * This is an overloaded method that accepts the number of samples in SPP.
     * The spp value is a multiple of the number of pixels in the image (samples-per-pixel).
     *
     * @returns Number of evaluated samples (not in SPP).
     */
    int64_t evaluateSamples(SPP spp);

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
