#include "fbksd/renderer/samples.h"
#include <cstring>


// ======================================================
//                      SampleBuffer
// ======================================================

// Static initialization
std::vector<bool> SampleBuffer::ioMask;

SampleBuffer::SampleBuffer():
    paramentersBuffer(new float[NUM_RANDOM_PARAMETERS]),
    featuresBuffer(new float[NUM_FEATURES])
{
    memset(paramentersBuffer.get(), 0, NUM_RANDOM_PARAMETERS * sizeof(float));
    memset(featuresBuffer.get(), 0, NUM_FEATURES * sizeof(float));
}

SampleBuffer::SampleBuffer(const SampleBuffer& buffer) :
    SampleBuffer()
{
    *this = buffer;
}

SampleBuffer& SampleBuffer::operator=(const SampleBuffer& buffer)
{
    memcpy(paramentersBuffer.get(), buffer.paramentersBuffer.get(), NUM_RANDOM_PARAMETERS * sizeof(float));
    memcpy(featuresBuffer.get(), buffer.featuresBuffer.get(), NUM_FEATURES * sizeof(float));
    return *this;
}

void SampleBuffer::setLayout(const SampleLayout& layout)
{
    ioMask = std::vector<bool>(NUM_RANDOM_PARAMETERS, false);
    for(const auto& par: layout.parameters)
    {
        RandomParameter p;
        if(stringToRandomParameter(par.name, &p))
            ioMask[p] = par.io;
    }
}



// ======================================================
//                      SamplesPipe
// ======================================================

// Static initialization
int64_t SamplesPipe::sampleSize = 0;
int64_t SamplesPipe::numSamples = 0;
float* SamplesPipe::samples = nullptr;
std::vector<std::pair<int, int>> SamplesPipe::inputParameterIndices;
std::vector<std::pair<int, int>> SamplesPipe::outputParameterIndices;
std::vector<std::pair<int, int>> SamplesPipe::outputFeatureIndices;

SamplesPipe::SamplesPipe():
    currentSamplePtr(samples)
{
}

void SamplesPipe::seek(size_t pos)
{
    currentSamplePtr = samples + pos;
}

void SamplesPipe::seek(int x, int y, int spp, int width)
{
    size_t index = size_t(x)*sampleSize*spp + size_t(y)*sampleSize*spp*width;
    currentSamplePtr = &samples[index];
}

size_t SamplesPipe::getPosition()
{
    return currentSamplePtr - samples;
}

SampleBuffer SamplesPipe::getBuffer()
{
    SampleBuffer buffer;
    for(const auto& pair: inputParameterIndices)
        buffer.paramentersBuffer[pair.first] = currentSamplePtr[pair.second];
    return buffer;
}

SamplesPipe& SamplesPipe::operator<<(const SampleBuffer& buffer)
{
    for(const auto& pair: outputParameterIndices)
        currentSamplePtr[pair.second] = buffer.paramentersBuffer[pair.first];
    for(const auto& pair: outputFeatureIndices)
        currentSamplePtr[pair.second] = buffer.featuresBuffer[pair.first];
    currentSamplePtr += sampleSize;
    return *this;
}

void SamplesPipe::setLayout(const SampleLayout& layout)
{
    SampleBuffer::setLayout(layout);
    sampleSize = layout.getSampleSize();

    for(size_t i = 0; i < layout.parameters.size(); ++i)
    {
        auto &par = layout.parameters[i];
        RandomParameter p;
        if(stringToRandomParameter(par.name, &p))
        {
            if(par.io == SampleLayout::INPUT)
                inputParameterIndices.push_back(std::make_pair(p, i));
            else
                outputParameterIndices.push_back(std::make_pair(p, i));
        }
        else
        {
            Feature f;
            if(stringToFeature(par.name, &f))
            {
                f = toNumbered(f, par.number);
                outputFeatureIndices.push_back(std::make_pair(f, i));
            }
        }
    }
}
