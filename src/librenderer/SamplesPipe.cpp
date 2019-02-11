/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/renderer/SamplesPipe.h"
#include "TilePool.h"
using namespace fbksd;
#include <cassert>


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
int64_t SamplesPipe::sm_sampleSize = 0;
int64_t SamplesPipe::sm_numSamples = 0;
std::vector<std::pair<int, int>> SamplesPipe::sm_inputParameterIndices;
std::vector<std::pair<int, int>> SamplesPipe::sm_outputParameterIndices;
std::vector<std::pair<int, int>> SamplesPipe::sm_outputFeatureIndices;


SamplesPipe::SamplesPipe(const Point2l &begin, const Point2l &end, int64_t numSamples):
    m_begin(begin),
    m_end(end),
    m_width(end.x - begin.x),
    m_informedNumSamples(numSamples)
{
    m_samples = m_currentSamplePtr = TilePool::getFreeTile(begin, end, numSamples);
}

SamplesPipe::SamplesPipe(SamplesPipe &&pipe)
{
    m_samples = pipe.m_samples;
    m_currentSamplePtr = pipe.m_currentSamplePtr;
    m_begin = pipe.m_begin;
    m_end = pipe.m_end;
    m_width = pipe.m_width;
    pipe.m_samples = pipe.m_currentSamplePtr = nullptr;
}

SamplesPipe::~SamplesPipe()
{
    TilePool::releaseWorkedTile(*this);
}

void SamplesPipe::seek(int x, int y)
{
    assert(m_begin.x <= x);
    assert(x < m_end.x);
    assert(m_begin.y <= y);
    assert(y < m_end.y);
    auto index = int64_t(x - m_begin.x) * sm_sampleSize * sm_numSamples +
                 int64_t(y - m_begin.y) * sm_sampleSize * sm_numSamples * m_width;
    m_currentSamplePtr = &m_samples[index];
}

size_t SamplesPipe::getPosition() const
{
    return m_currentSamplePtr - m_samples;
}

size_t SamplesPipe::getNumSamples() const
{
    return m_numSamples;
}

SampleBuffer SamplesPipe::getBuffer()
{
    SampleBuffer buffer;
    for(const auto& pair: sm_inputParameterIndices)
        buffer.m_paramentersBuffer[pair.first] = m_currentSamplePtr[pair.second];
    return buffer;
}

SamplesPipe& SamplesPipe::operator<<(const SampleBuffer& buffer)
{
    for(const auto& pair: sm_outputParameterIndices)
        m_currentSamplePtr[pair.second] = buffer.m_paramentersBuffer[pair.first];
    for(const auto& pair: sm_outputFeatureIndices)
        m_currentSamplePtr[pair.second] = buffer.m_featuresBuffer[pair.first];
    m_currentSamplePtr += sm_sampleSize;
    ++m_numSamples;
    return *this;
}

void SamplesPipe::setLayout(const SampleLayout& layout)
{
    SampleBuffer::setLayout(layout);
    sm_sampleSize = layout.getSampleSize();

    sm_inputParameterIndices.clear();
    sm_outputParameterIndices.clear();
    sm_outputFeatureIndices.clear();
    for(size_t i = 0; i < layout.parameters.size(); ++i)
    {
        auto &par = layout.parameters[i];
        RandomParameter p;
        if(stringToRandomParameter(par.name, &p))
        {
            if(par.io == SampleLayout::INPUT)
                sm_inputParameterIndices.emplace_back(p, i);
            else
                sm_outputParameterIndices.emplace_back(p, i);
        }
        else
        {
            Feature f;
            if(stringToFeature(par.name, &f))
            {
                f = toNumbered(f, par.number);
                sm_outputFeatureIndices.emplace_back(f, i);
            }
        }
    }
}
