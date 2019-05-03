# FBKSD API

## Table of Contents

- [Developing a Denoising Technique](#developing-a-denoising-technique) 
- [Developing an Image Quality Assessment Metric](#developing-an-image-quality-assessment-metric) 
- [Porting a Renderer](#porting-a-renderer)


## Developing a Denoising Technique

For developing denoisers or samplers, see the fbksd::BenchmarkClient class. It contains the main API used to communicate with the benchmark server.

The code below shows a simple box filter implemented using the client API.

```cpp
#include <fbksd/client/BenchmarkClient.h>
using namespace fbksd;

int main(int argc, char* argv[])
{
    // STEP 1: Instantiate the client object and get scene information.
    BenchmarkClient client(argc, argv);
    SceneInfo scene = client.getSceneInfo();
    const auto w = scene.get<int64_t>("width");
    const auto h = scene.get<int64_t>("height");
    const auto spp = scene.get<int64_t>("max_spp");

    // STEP 2: Set the sample layout.
    SampleLayout layout;
    layout("COLOR_R")("COLOR_G")("COLOR_B");
    client.setSampleLayout(layout);

    float* result = client.getResultBuffer();

    // STEP 3: Request samples.
    client.evaluateSamples(SPP(spp), [&](const BufferTile& tile)
    {
        for(auto y = tile.beginY(); y < tile.endY(); ++y)
        for(auto x = tile.beginX(); x < tile.endX(); ++x)
        {
            float* pixel = &result[y*w*3 + x*3];
            for(int s = 0; s < spp; ++s)
            {
                float* sample = tile(x, y, s);
                pixel[0] += sample[0];
                pixel[1] += sample[1];
                pixel[2] += sample[2];
            }
        }
    });
    
    // STEP 4: Reconstruct the image.
    const float sppInv = 1.f / (float)spp;
    for(int64_t y = 0; y < h; ++y)
    for(int64_t x = 0; x < w; ++x)
    {
        float* pixel = &result[y*w*3 + x*3];
        pixel[0] *= sppInv;
        pixel[1] *= sppInv;
        pixel[2] *= sppInv;
    }

    // STEP 5: Send result and finish.
    client.sendResult();
    return 0;
}
```
The easiest way of compiling the code is using cmake. Just use `find_package(fbksd)` and link your target with `fbksd::client`.

Ex:

```cmake
find_package(fbksd REQUIRED)
set(CMAKE_CXX_STANDARD 11)

add_executable(MyFilter main.cpp)
target_link_libraries(MyFilter fbksd::client)
```

> **Note**: C++ standard 11 or superior is required. 

After you compile your technique, the executable should be placed in the `<workspace>/denoisers/<technique name>` directory, alongside a `info.json` file containing information about your technique (see instructions in the [main repository page](https://github.com/fbksd/fbksd)).


## Developing an Image Quality Assessment Metric

FBKSD provides an API that allows implementing IQA metrics to be used by the system. The library is called `fbksd-iqa` and is composed of two classes: fbksd::IQA and fbksd::Img.

The code bellow is an example of MSE IQA metric using OpenCV.

```cpp
#include <fbksd/iqa/iqa.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;

static float computeMSE(const Mat& img1, const Mat& img2)
{
    Mat s1;
    absdiff(img1, img2, s1);
    s1 = s1.mul(s1);
    Scalar s = sum(s1);
    double sse = s.val[0] + s.val[1] + s.val[2];
    double mse  = sse / (double)(img1.channels() * img1.total());
    return mse;
}

int main(int argc, char* argv[])
{
    // STEP 1: Create an instance of fbksd::IQA, passing the required information.
    fbksd::IQA iqa(argc, argv,                             // argc, argv from main()
                   "My-MSE",                               // Metric's acronym 
                   "My Simple Mean squared error metric",  // Metric's full name
                   "",                                     // Reference (optional)
                   true,                                   // lowerIsBetter flag
                   false);                                 // hasErrorMap flag
    fbksd::Img ref;
    fbksd::Img test;
    // STEP 2: Load the input images.
    iqa.loadInputImages(ref, test);

    // STEP 3: Compute the IQA value comparing the two images.
    Mat refMat(ref.height(), ref.width(), CV_32FC3, ref.data());
    Mat testMat(ref.height(), ref.width(), CV_32FC3, test.data());
    float error = computeMSE(refMat, testMat);

    // STEP 4: Report the value (and the error map, if supported).
    iqa.report(error);
    return 0;
}
```

To compile your metric using cmake, just use `find_package(fbksd)` and link your target with `fbksd::iqa`.

Ex:

```cmake
find_package(fbksd REQUIRED)
set(CMAKE_CXX_STANDARD 11)

add_executable(MyMetric main.cpp)
target_link_libraries(MyMetric fbksd::iqa)
```

> **Note**: C++ standard 11 or superior is required. 

The compiled IQA technique should be placed inside sub-folder of `<workspace>/iqa/`, alongside a `info.json` file with the metadata about the metric (see instructions in the [main repository page](https://github.com/fbksd/fbksd)).


## Porting a Renderer

Porting renderers to be used as rendering back-ends is a bit more complex.
The main classes of interest are: fbksd::RenderingServer, fbksd::SamplesPipe and fbksd::SampleBuffer.

The code below shows an example of simple renderer that always renders blue images.
For examples of real renderers, see the renderers provided in the [fbksd-package repository](https://github.com/fbksd/fbksd-package).

```cpp
#include <fbksd/renderer/RenderingServer.h>
#include <fbksd/renderer/samples.h>
#include <iostream>
#include <thread>
#include <cmath>
using namespace fbksd;

namespace
{

// Fixed image size and SPP.
// In a real renderer, this would be loaded from the scene file.
const int64_t g_width = 1000;
const int64_t g_height = 1000;
const int64_t g_spp = 1;


// Returns a SceneInfo with information about the scene.
SceneInfo getSceneInfo()
{
    SceneInfo info;
    info.set<int64_t>("width", g_width);
    info.set<int64_t>("height", g_height);
    info.set<int64_t>("max_spp", g_spp);
    info.set<int64_t>("max_samples", g_width * g_height * g_spp);
    return info;
}

void setLayout(const SampleLayout& layout)
{
}

void evaluateSamples(int64_t spp, int64_t remainder, int tileSize)
{
    if(spp)
    {
        // break the image in tiles 16x16
        int nTilesX = std::ceil(float(g_width) / 16);
        int nTilesY = std::ceil(float(g_height) / 16);
        for(int tileY = 0; tileY < nTilesY; ++tileY)
        for(int tileX = 0; tileX < nTilesX; ++tileX)
        {
            int beginX = tileX*16;
            int beginY = tileY*16;
            int endX = std::min((tileX+1)*16, w);
            int endY = std::min((tileY+1)*16, h);
            int numPixels = (endX - beginX) * (endY - beginY);
            SamplesPipe pipe({beginX, beginY}, {endX, endY}, spp * numPixels);

            for(int y = beginY; y < endY; ++y)
            for(int x = beginX; x < endX; ++x)
            {
                pipe.seek(x, y);
                for(int64_t s = 0; s < spp; ++s)
                {
                    SampleBuffer sampleBuffer = pipe.getBuffer();
                    sampleBuffer.set(IMAGE_X, x);
                    sampleBuffer.set(IMAGE_Y, y);
                    sampleBuffer.set(COLOR_R, 0.0);
                    sampleBuffer.set(COLOR_G, 0.0);
                    sampleBuffer.set(COLOR_B, 1.0);
                    pipe << sampleBuffer;
                }
            }
        }
    }

    if(remainder)
    {
        // divide the remaining samples in tiles of maximum size.
        int nTiles = std::ceil(float(remainder) / tileSize);
        for(int tile = 0; tile < nTiles; ++tile)
        {
            int numSamples = tile == 0 ? remainder % tileSize : tileSize;
            SamplesPipe pipe({0, 0}, {w, h}, numSamples);
            for(int i = 0; i < numSamples; ++i)
            {
                SampleBuffer sampleBuffer = pipe.getBuffer();
                int x = drand48() * g_width;
                int y = drand48() * g_height;
                sampleBuffer.set(IMAGE_X, x);
                sampleBuffer.set(IMAGE_Y, y);
                sampleBuffer.set(COLOR_R, 0.0);
                sampleBuffer.set(COLOR_G, 0.0);
                sampleBuffer.set(COLOR_B, 1.0);
                pipe << sampleBuffer;
            }
        }
    }
}

void finish()
{
    std::cout << "Done." << std::endl;
}

} // namespace

int main(int argc, char* argv[])
{
    // STEP 1: Crate rendering server object.
    RenderingServer server;

    // STEP 2: Set the callback functions or methods

    // In our case, we'll break images in 16x16 tiles.
    server.onGetTileSize([](){return 16;});

    // getSceneInfo() will be called when the server need information
    // about the scene being rendered.
    server.onGetSceneInfo(&getSceneInfo);

    // onSetParameters() will be called to inform the renderer about sample layout requirements.
    server.onSetParameters(&setLayout);

    // evaluateSamples() will be called each time the server requires samples.
    // The sample data transfer between client and server is asynchronous. In this case we 
    // must run evaluteSamples() in another thread.
    std::unique_ptr<std::thread> thread;
    server.onEvaluateSamples([](int64_t spp, int64_t remainder, int tileSize){
        if(thread && thread->joinable())
            thread->join();
        thread.reset(new std::thread(evaluateSamples, spp, remainder, tileSize));
    });

    // finish() will be called when the server is done with the current scene.
    server.onFinish(&finish);

    // STEP 3: Run the server.
    // This is a blocking call that will return only when the benchmark server decides.
    server.run();
    return 0;
}
```

To compile the renderer using cmake, just use `find_package(fbksd)` and link your target with `fbksd::renderer`.

Ex:

```cmake
find_package(fbksd REQUIRED)
set(CMAKE_CXX_STANDARD 11)

add_executable(MyRenderer main.cpp)
target_link_libraries(MyRenderer fbksd::renderer)
```

> **Note**: C++ standard 11 or superior is required. 

After you renderer is compiled, the executable should be placed in the `<workspace>/renderers/<renderer name>` directory, alongside a `info.json` file containing information about your the renderer.

```json
{
    "name": "<renderer name>",
    "exec": "<renderer executable>"
}
```
Scenes for the new renderer should be placed in the `<workspace>/scenes/<renderer name>` directory, with corresponding reference images in the required formats (see instructions in the [main repository page](https://github.com/fbksd/fbksd)).
