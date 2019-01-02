/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef SAMPLESPIPE_H
#define SAMPLESPIPE_H

#include "fbksd/renderer/samples.h"
#include "fbksd/core/SampleLayout.h"


// This is need for the next classes due to the static members
#define EXPORT_LIB __attribute__((visibility("default")))


/**
 * \brief The SampleBuffer class helps the rendering system to provide the appropriate data
 * requested by the benchmark system.
 */
class EXPORT_LIB SampleBuffer
{
public:
    SampleBuffer();

    /**
     * \brief Main access method for random parameters.
     *
     * This operator sets the RandomParameter \a i to and returns \a v, if \a i is OUTPUT.
     * If \a i is INPUT, it ignores \a v and returns the value of \a i from the buffer.
     */
    float set(RandomParameter i, float v);

    float get(RandomParameter i);

    /**
     * \brief Main access method for features.
     *
     * This operator sets the Feature \a f to and returns \a v.
     */
    float set(Feature f, float v);

    /**
    * \brief Overloaded function.
    *
    * Sets the numbered Feature \a f to and returns \a v.
    */
    float set(Feature f, int number, float v);

    float get(Feature f);

private:
    friend class SamplesPipe;

    static void setLayout(const SampleLayout& layout);

    // ioMask is only for random parameters, since features are always OUTPUT.
    static std::array<bool, NUM_RANDOM_PARAMETERS> m_ioMask;
    std::array<float, NUM_RANDOM_PARAMETERS> m_paramentersBuffer;
    std::array<float, NUM_FEATURES> m_featuresBuffer;
};


/**
 * \brief The SamplesPipe class is the main way to transfer samples between client and server.
 */
class EXPORT_LIB SamplesPipe
{
public:
    SamplesPipe();

    void seek(size_t pos);
    void seek(int x, int y, int spp, int width);
    size_t getPosition();

    SampleBuffer getBuffer();

    SamplesPipe& operator<<(const SampleBuffer& buffer);

private:
    friend class RenderingServer;

    static void init(const SampleLayout& layout, float* samplesBuffer);

    static int64_t m_sampleSize;
    static float* m_samples;
    static std::vector<std::pair<int, int>> m_inputParameterIndices;
    static std::vector<std::pair<int, int>> m_outputParameterIndices;
    static std::vector<std::pair<int, int>> m_outputFeatureIndices;
    float* m_currentSamplePtr;
};

#endif // SAMPLESPIPE_H
