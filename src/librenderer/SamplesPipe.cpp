/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/renderer/SamplesPipe.h"
using namespace fbksd;


// ======================================================
//                      SampleBuffer
// ======================================================

// Static initialization
std::array<bool, NUM_RANDOM_PARAMETERS> SampleBuffer::m_ioMask;

SampleBuffer::SampleBuffer()
{
    m_paramentersBuffer.fill(0.f);
    m_featuresBuffer.fill(0.f);
}

float SampleBuffer::set(RandomParameter i, float v)
{
    if(m_ioMask[i] == SampleLayout::INPUT)
        return m_paramentersBuffer[i];
    else
        return m_paramentersBuffer[i] = v;
}

float SampleBuffer::get(RandomParameter i)
{
    return m_paramentersBuffer[i];
}

float SampleBuffer::set(Feature f, float v)
{
    return m_featuresBuffer[f] = v;
}

float SampleBuffer::set(Feature f, int number, float v)
{
    if(number > 1) number = 1;
    return m_featuresBuffer[toNumbered(f, number)] = v;
}

float SampleBuffer::get(Feature f)
{
    return m_featuresBuffer[f];
}

void SampleBuffer::setLayout(const SampleLayout& layout)
{
    m_ioMask.fill(false);
    RandomParameter p;
    for(const auto& par: layout.parameters)
    {
        if(stringToRandomParameter(par.name, &p))
            m_ioMask[p] = par.io;
    }
}



// ======================================================
//                      SamplesPipe
// ======================================================

// Static initialization
int64_t SamplesPipe::m_sampleSize = 0;
float* SamplesPipe::m_samples = nullptr;
std::vector<std::pair<int, int>> SamplesPipe::m_inputParameterIndices;
std::vector<std::pair<int, int>> SamplesPipe::m_outputParameterIndices;
std::vector<std::pair<int, int>> SamplesPipe::m_outputFeatureIndices;

SamplesPipe::SamplesPipe():
    m_currentSamplePtr(m_samples)
{
}

void SamplesPipe::seek(size_t pos)
{
    m_currentSamplePtr = m_samples + pos;
}

void SamplesPipe::seek(int x, int y, int spp, int width)
{
    size_t index = size_t(x)*m_sampleSize*spp + size_t(y)*m_sampleSize*spp*width;
    m_currentSamplePtr = &m_samples[index];
}

size_t SamplesPipe::getPosition()
{
    return m_currentSamplePtr - m_samples;
}

SampleBuffer SamplesPipe::getBuffer()
{
    SampleBuffer buffer;
    for(const auto& pair: m_inputParameterIndices)
        buffer.m_paramentersBuffer[pair.first] = m_currentSamplePtr[pair.second];
    return buffer;
}

SamplesPipe& SamplesPipe::operator<<(const SampleBuffer& buffer)
{
    for(const auto& pair: m_outputParameterIndices)
        m_currentSamplePtr[pair.second] = buffer.m_paramentersBuffer[pair.first];
    for(const auto& pair: m_outputFeatureIndices)
        m_currentSamplePtr[pair.second] = buffer.m_featuresBuffer[pair.first];
    m_currentSamplePtr += m_sampleSize;
    return *this;
}

void SamplesPipe::init(const SampleLayout& layout, float* samplesBuffer)
{
    SampleBuffer::setLayout(layout);
    m_sampleSize = layout.getSampleSize();
    m_samples = samplesBuffer;

    for(size_t i = 0; i < layout.parameters.size(); ++i)
    {
        auto &par = layout.parameters[i];
        RandomParameter p;
        if(stringToRandomParameter(par.name, &p))
        {
            if(par.io == SampleLayout::INPUT)
                m_inputParameterIndices.emplace_back(p, i);
            else
                m_outputParameterIndices.emplace_back(p, i);
        }
        else
        {
            Feature f;
            if(stringToFeature(par.name, &f))
            {
                f = toNumbered(f, par.number);
                m_outputFeatureIndices.emplace_back(f, i);
            }
        }
    }
}
