/*
 * Copyright (c) 2018 Jonas Deyson
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
    explicit SPP(int64_t value):
        m_value(value)
    {}

    int64_t operator=(int64_t v)
    { m_value = v; }

    void setValue(int64_t v)
    { m_value = v; }

    int64_t getValue() const
    { return m_value; }

private:
    int64_t m_value = 0;
};


/**
 * \brief The BenchmarkClient class is used to communicate with the benchmark server.
 *
 * This class provides query methods that permit to get information about the scene, get samples, end send the final result.
 * Only one instance of this class should be created in your program.
 *
 * A simple use case for a non-adaptive reconstruction technique is the following:
 * \snippet BenchmarkClient_snippet.cpp 4
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
     *     --fbksd-renderer "<renderer_exec> <renderer_args> ..."
     *         Starts the renderer process with the given arguments.
     *     --fbksd-spp <value>
     *         Sets the sample budget available to client.
     * This allows you to run your client program directly (for debugging purposes, for example).
     */
    BenchmarkClient(int argc = 0, char* argv[] = nullptr);

    BenchmarkClient(const BenchmarkClient&) = delete;

    BenchmarkClient(BenchmarkClient&&) = default;

    ~BenchmarkClient();

    /**
     * \brief Get information about the scene being rendered.
     *
     * Gets information about the scene being rendered,
     * allowing you to configure your technique accordingly.
     *
     * \return SceneInfo
     */
    SceneInfo getSceneInfo();

    /**
     * \brief Sets the sample layout.
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
     * BenchmarkClient owns the buffer: the user should not delete it manually.
     */
    float* getSamplesBuffer();

    /**
     * \brief Returns a pointer to the buffer where the result image is stored.
     *
     * The user should write the final reconstructed image to this buffer before calling
     * sendResult();
     *
     * BenchmarkClient owns the buffer: the user should not delete it manually.
     */
    float* getResultBuffer();

    /**
     * \brief Evaluate samples.
     *
     * The samples are written to the buffer returned by getSamplesBuffer().
     *
     * `numSamples` should not exceed the maximum allowed sample budget.
     *
     * The unit changes the distribution of the generated sample positions. When `unit == SAMPLES`,
     * there is no guarantee that all pixels are covered by samples.
     *
     * When the sample layout has input elements, they should be passed in the corresponding positions in samples buffer.
     *
     * \param[in] unit          The unit used to count samples
     * \param[in] numSamples    The number of samples (according to unit)
     */
    int64_t evaluateSamples(int64_t numSamples);

    /**
     * @brief Overloaded method.
     *
     * Evaluates an amount of samples given as a multiple of the number of pixels in the image (samples-per-pixel).
     *
     * @returns Number of evaluated samples (not in SPP).
     */
    int64_t evaluateSamples(SPP spp);

    /**
     * \brief Sends the final result (rgb image)
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
