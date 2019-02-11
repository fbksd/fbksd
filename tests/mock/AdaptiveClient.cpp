/*
 * Copyright (c) 2018 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include <fbksd/client/BenchmarkClient.h>
using namespace fbksd;
#include <cmath>
#include <iostream>

void genSample(int64_t w, int64_t h, float* sample)
{
    sample[0] = std::min<float>(w - 1, drand48() * w);
    sample[1] = std::min<float>(h - 1, drand48() * h);
}


int main(int argc, char* argv[])
{
    BenchmarkClient client(argc, argv);
    SceneInfo scene = client.getSceneInfo();
    const auto w = scene.get<int64_t>("width");
    const auto h = scene.get<int64_t>("height");
    const auto spp = scene.get<int64_t>("max_spp");
    const auto totalNumSamples = w*h*spp;

    SampleLayout layout;
    layout("IMAGE_X", SampleLayout::INPUT)("IMAGE_Y", SampleLayout::INPUT)("COLOR_R")("COLOR_G")("COLOR_B");
    client.setSampleLayout(layout);
    const auto sampleSize = layout.getSampleSize();

    float* result = client.getResultBuffer();

    const int numIterations = std::max<int>(spp - 1, 1);
    const int64_t samplesPerIter = std::floor(float(totalNumSamples) / numIterations);
    const int64_t rest = totalNumSamples % numIterations;
    std::cout << "num iterations = " << numIterations << std::endl;
    std::cout << "samples per iteration = " << samplesPerIter << std::endl;
    std::cout << "remainder = " << rest << std::endl;

    std::vector<int> samplesCount(w*h, 0);
    for(int it = 0; it < numIterations; ++it)
    {
        int64_t numSamples = 0;
        if(rest && it == 0)
            numSamples = rest;
        else
            numSamples = samplesPerIter;

        client.evaluateInputSamples(numSamples,
            [&](int64_t n, float* samples)
            {
                for(int64_t i = 0; i < n; ++i)
                {
                    float* sample = &samples[i*sampleSize];
                    genSample(w, h, sample);
                }
            },
            [&](int64_t n, float* samples)
            {
                for(int64_t i = 0; i < n; ++i)
                {
                    float* sample = &samples[i*sampleSize];
                    int x = sample[0];
                    int y = sample[1];
                    samplesCount[y*w + x] += 1;

                    float* pixel = &result[y*w*3 + x*3];
                    pixel[0] += sample[2];
                    pixel[1] += sample[3];
                    pixel[2] += sample[4];
                }
            }
        );
    }

    for(int y = 0; y < h; ++y)
    for(int x = 0; x < w; ++x)
    {
        int n = samplesCount[y*w + x];
        if(n)
        {
            float* pixel = &result[y*w*3 + x*3];
            pixel[0] /= float(n);
            pixel[1] /= float(n);
            pixel[2] /= float(n);
        }
    }

    client.sendResult();
    return 0;
}
