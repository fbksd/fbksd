#include <fbksd/renderer/RenderingServer.h>
#include <fbksd/renderer/samples.h>
#include <random>
#include <iostream>

namespace
{

const int64_t g_width = 200;
const int64_t g_height = 200;
int64_t g_spp = 8;

SampleLayout g_layout;
float* g_samples = nullptr;


float rand()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.f, 1.f);
    return dis(gen);
}

float lerp(float v, float min, float max)
{
    return (1.f - v)*min + v*max;
}


SceneInfo getSceneInfo()
{
    SceneInfo info;
    info.set<int64_t>("width", g_width);
    info.set<int64_t>("height", g_height);
    info.set<int64_t>("max_spp", g_spp);
    info.set<int64_t>("max_samples", g_width * g_height * g_spp);
    return info;
}

void setLayout(int maxSPP, const SampleLayout& layout, float* samplesBuffer)
{
    g_spp = maxSPP;
    g_layout = layout;
    g_samples = samplesBuffer;
}

int64_t evaluateSamples(bool isSPP, int64_t numSamples)
{
    auto numPixels = g_width * g_height;
    auto totalNumSamples = isSPP ? numPixels * numSamples : numSamples;

    int64_t spp = isSPP ? numSamples : totalNumSamples / (float)(numPixels);
    int64_t rest = totalNumSamples % numPixels;
//    bool hasInputPos = m_layout.hasInput("IMAGE_X") || m_layout.hasInput("IMAGE_Y");


    SamplesPipe pipe;
    SampleBuffer sampleBuffer = pipe.getBuffer();

    float inc = std::numeric_limits<float>::max() / totalNumSamples;
    if(inc > std::numeric_limits<float>::epsilon())
        std::cout << "good increment" << std::endl;

    int64_t i = 0;
    for(int64_t y = 0; y < g_height; ++y)
    for(int64_t x = 0; x < g_width; ++x)
    for(int64_t s = 0; s < spp; ++s)
    {
        pipe.seek(x, y, g_spp, g_width);
        float xs = rand() + x;
        float ys = rand() + y;
        sampleBuffer.set(IMAGE_X, inc*i);
        i++;
        sampleBuffer.set(IMAGE_Y, inc*i);
        i++;
        sampleBuffer.set(COLOR_R, inc*i);
        i++;
        sampleBuffer.set(COLOR_G, inc*i);
        i++;
        sampleBuffer.set(COLOR_B, inc*i);
        i++;
        pipe << sampleBuffer;
    }

    return totalNumSamples;
}

void finish()
{
}
}


int main()
{
    RenderingServer server;
    server.onGetSceneInfo(&getSceneInfo);
    server.onSetParameters(&setLayout);
    server.onEvaluateSamples(&evaluateSamples);
    server.onFinish(&finish);
    server.run();

    return 0;
}
