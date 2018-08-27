#include <fbksd/renderer/RenderingServer.h>
#include <fbksd/renderer/samples.h>
#include <random>
#include <iostream>
#include <QCommandLineParser>
#include <QTimer>

namespace
{

int64_t g_width = 200;
int64_t g_height = 200;
int64_t g_spp = 4;

SampleLayout g_layout;
float* g_samples = nullptr;


float rand()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.f, 1.f);
    return dis(gen);
}

float getValue(int64_t x, int64_t y, int64_t s, int64_t c)
{
    // FIXME: This function produces subnormal floats,
    // that may not be very good (lower performance).
    constexpr int64_t totalSampleSize = 41;
    int64_t i = y * g_width * g_spp * totalSampleSize;
    i += x * totalSampleSize * g_spp;
    i+= s * totalSampleSize + c;
    int32_t k = i % std::numeric_limits<int32_t>::max();
    return *reinterpret_cast<float*>(&k);
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

    SamplesPipe pipe;
    SampleBuffer sampleBuffer = pipe.getBuffer();

    for(int64_t y = 0; y < g_height; ++y)
    for(int64_t x = 0; x < g_width; ++x)
    {
        pipe.seek(x, y, g_spp, g_width);
        for(int64_t s = 0; s < spp; ++s)
        {
            int i = 0;
            sampleBuffer.set(IMAGE_X, getValue(x, y, s, i++));
            sampleBuffer.set(IMAGE_Y, getValue(x, y, s, i++));
            sampleBuffer.set(LENS_U, getValue(x, y, s, i++));
            sampleBuffer.set(LENS_V, getValue(x, y, s, i++));
            sampleBuffer.set(TIME, getValue(x, y, s, i++));
            sampleBuffer.set(LIGHT_X, getValue(x, y, s, i++));
            sampleBuffer.set(LIGHT_Y, getValue(x, y, s, i++));
            sampleBuffer.set(COLOR_R, getValue(x, y, s, i++));
            sampleBuffer.set(COLOR_G, getValue(x, y, s, i++));
            sampleBuffer.set(COLOR_B, getValue(x, y, s, i++));
            sampleBuffer.set(DEPTH, getValue(x, y, s, i++));
            sampleBuffer.set(DIRECT_LIGHT_R, getValue(x, y, s, i++));
            sampleBuffer.set(DIRECT_LIGHT_G, getValue(x, y, s, i++));
            sampleBuffer.set(DIRECT_LIGHT_B, getValue(x, y, s, i++));
            sampleBuffer.set(WORLD_X, getValue(x, y, s, i++));
            sampleBuffer.set(WORLD_Y, getValue(x, y, s, i++));
            sampleBuffer.set(WORLD_Z, getValue(x, y, s, i++));
            sampleBuffer.set(NORMAL_X, getValue(x, y, s, i++));
            sampleBuffer.set(NORMAL_Y, getValue(x, y, s, i++));
            sampleBuffer.set(NORMAL_Z, getValue(x, y, s, i++));
            sampleBuffer.set(TEXTURE_COLOR_R, getValue(x, y, s, i++));
            sampleBuffer.set(TEXTURE_COLOR_G, getValue(x, y, s, i++));
            sampleBuffer.set(TEXTURE_COLOR_B, getValue(x, y, s, i++));
            sampleBuffer.set(WORLD_X_1, getValue(x, y, s, i++));
            sampleBuffer.set(WORLD_Y_1, getValue(x, y, s, i++));
            sampleBuffer.set(WORLD_Z_1, getValue(x, y, s, i++));
            sampleBuffer.set(NORMAL_X_1, getValue(x, y, s, i++));
            sampleBuffer.set(NORMAL_Y_1, getValue(x, y, s, i++));
            sampleBuffer.set(NORMAL_Z_1, getValue(x, y, s, i++));
            sampleBuffer.set(TEXTURE_COLOR_R_1, getValue(x, y, s, i++));
            sampleBuffer.set(TEXTURE_COLOR_G_1, getValue(x, y, s, i++));
            sampleBuffer.set(TEXTURE_COLOR_B_1, getValue(x, y, s, i++));
            sampleBuffer.set(WORLD_X_NS, getValue(x, y, s, i++));
            sampleBuffer.set(WORLD_Y_NS, getValue(x, y, s, i++));
            sampleBuffer.set(WORLD_Z_NS, getValue(x, y, s, i++));
            sampleBuffer.set(NORMAL_X_NS, getValue(x, y, s, i++));
            sampleBuffer.set(NORMAL_Y_NS, getValue(x, y, s, i++));
            sampleBuffer.set(NORMAL_Z_NS, getValue(x, y, s, i++));
            sampleBuffer.set(TEXTURE_COLOR_R_NS, getValue(x, y, s, i++));
            sampleBuffer.set(TEXTURE_COLOR_G_NS, getValue(x, y, s, i++));
            sampleBuffer.set(TEXTURE_COLOR_B_NS, getValue(x, y, s, i++));
            pipe << sampleBuffer;
        }
    }

    return totalNumSamples;
}

void finish()
{
}
}


int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;

    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption sizeOpt("img-size", "Image size: <width>x<height>", "size");
    sizeOpt.setDefaultValue("200x200");
    parser.addOption(sizeOpt);
    QCommandLineOption sppOpt("spp", "Number of samples per pixel.", "spp");
    sppOpt.setDefaultValue("4");
    parser.addOption(sppOpt);
    parser.process(app);

    if(parser.isSet(sizeOpt))
    {
        QString size = parser.value(sizeOpt);
        auto values = size.split("x");
        g_width = values[0].toInt();
        g_height = values[1].toInt();
    }
    if(parser.isSet(sppOpt))
        g_spp = parser.value(sppOpt).toInt();

    std::cout << "img size = " << g_width << " x " << g_height << std::endl;
    std::cout << "spp = " << g_spp << std::endl;

    RenderingServer server;
    server.onGetSceneInfo(&getSceneInfo);
    server.onSetParameters(&setLayout);
    server.onEvaluateSamples(&evaluateSamples);
    server.onFinish(&finish);
    server.run();

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &app, &QCoreApplication::quit);
    timer.start();
    return app.exec();
}
