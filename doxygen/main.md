This is the 2.0.0 API documentation for FBKSD.


## Developing a Denoising Technique

For developing denoising or samplers, see the fbksd::BenchmarkClient class. It contains the main API used to communicate with the benchmark server.

The code below shows a simple box filter implemented using the client API.

```cpp
#include "fbksd/client/BenchmarkClient.h"
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

After you compile your technique, the executable should be placed in the `<workspace>/denoisers/<technique name>` directory, alongside a `info.json` file containing information about your technique.

```json
{
    "short_name": "Box",
    "full_name": "Box filter",
    "comment": "A simple box reconstruction filter.",
    "citation": "",
}
```

> **Note**: In the `info.json` above, the system assumes that the executable for the technique has the same name as the `"short_name"` field.

If your technique has more the one variant (e.g. with different approaches for a certain step of the technique, for example), you can add the different versions like in the example below:

```json
{
    "short_name": "LBF",
    "full_name": "A Machine Learning Approach for Filtering Monte Carlo Noise",
    "comment": "Based on the original source code provided by the authors.",
    "citation": "Kalantari, N.K., Bako, S., and Sen, P. 2015. A machine learning approach for filtering Monte Carlo noise. ACM Transactions on Graphics 34, 4, 122:1-122:12.",
    "versions": [
        {
            "name": "default",
            "comment": "Based on the original source code provided by the authors",
            "executable": "LBF"
        },
        {
            "name": "mf",
            "comment": "This version uses features from the first non-specular intersection point",
            "executable": "LBF-mf"
        }
    ]
}
```

> **NOTE:** The `"name"` field in the version entry (except the `"default"` one) will be appended to the `"short_name"` field to form name of the technique when displaying it in the `fbksd` CLI or in the results visualization page.
Ex:
```
$ fbksd filters
Id   Name                Status    
---------------------------------------------------------------------------
...
11   LBF                 ready     
12   LBF-mf              ready     
...
---------------------------------------------------------------------------
```


Once the executable and the `info.json` file are installed in the workspace, the command `fbksd filters` should now list your technique. You can then add it to a config and run it.


## Porting a Renderer to FBKSD

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

After you renderer is compiled, the executable should be placed in the `<workspace>/renderers/<renderer name>` directory, alongside a `info.json` file containing information about your technique.

```json
{
    "name": "<renderer name>",
    "exec": "<renderer executable>"
}
```
