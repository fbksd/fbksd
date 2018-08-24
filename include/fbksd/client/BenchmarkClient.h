#ifndef BENCHMARKCLIENT_H
#define BENCHMARKCLIENT_H

#include "fbksd/core/Definitions.h"
#include "fbksd/core/SharedMemory.h"
#include <vector>
#include <string>
#include <map>
#include <memory>

namespace rpc{ class client; };


/**
 * \file
 * \brief File used by the user to communicate with the benchmark server.
 */

/**
 * \defgroup BenchmarkClient Benchmark Client Library
 * @{
 */


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
     * \brief Used to specify the samples counting unit when calling
     * evaluateSamples().
     *
     * Constant                            | Value  | Description
     * ----------------------------------- | ------ | --------------------------------------------------------------
     * BenchmarkClient::SAMPLES            | false  | The number of samples should be specified as a plain amount
     * BenchmarkClient::SAMPLES_PER_PIXEL  | true   | The number of samples should be specified as samples per pixel
     */
    enum SamplesCountUnit : bool
    {
        SAMPLES = false,
        SAMPLES_PER_PIXEL = true,
    };

    /**
     * \brief Connects with the benchmark server.
     *
     * The BenchmarkClient object should be instantiated at the beginning of the main function.
     */
    BenchmarkClient();

    BenchmarkClient(const BenchmarkClient&) = delete;

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
    int evaluateSamples(SamplesCountUnit unit, int numSamples);

    /**
     * \brief Sends the final result (rgb image)
     */
    void sendResult();

    BenchmarkClient& operator=(const BenchmarkClient&) = delete;

private:
    void fetchSceneInfo();

    std::unique_ptr<rpc::client> m_client;
    SharedMemory m_samplesMemory;
    SharedMemory m_resultMemory;
    SceneInfo m_sceneInfo;
    int m_maxNumSamples = 0;
};


/**@}*/

#endif
