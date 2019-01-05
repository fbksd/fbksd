This is the API documentation for FBKSD.


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

    // STEP 3: Request samples.
    float* samples = client.getSamplesBuffer();
    client.evaluateSamples(SPP(spp));
    
    // STEP 4: Reconstruct the image.
    float* result = client.getResultBuffer();
    const float sppInv = 1.f / (float)spp;
    for(int64_t y = 0; y < h; ++y)
    for(int64_t x = 0; x < w; ++x)
    {
        float* pixel = &result[y*w*3 + x*3];
        float* pixelSamples = &samples[y*w*spp*3 + x*3*spp];
        for(int s = 0; s < spp; ++s)
        {
            float* sample = &pixelSamples[s*3];
            pixel[0] += sample[0];
            pixel[1] += sample[1];
            pixel[2] += sample[2];
        }

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

bool evaluateSamples(int64_t spp, int64_t remainingCount)
{
    SamplesPipe pipe;
    SampleBuffer sampleBuffer = pipe.getBuffer();

    for(int64_t y = 0; y < g_height; ++y)
    for(int64_t x = 0; x < g_width; ++x)
    {
        pipe.seek(x, y, spp, g_width);
        for(int64_t s = 0; s < spp; ++s)
        {
            sampleBuffer.set(IMAGE_X, x);
            sampleBuffer.set(IMAGE_Y, y);
            sampleBuffer.set(COLOR_R, 0.0);
            sampleBuffer.set(COLOR_G, 0.0);
            sampleBuffer.set(COLOR_B, 1.0);
            pipe << sampleBuffer;
        }
    }

    return true;
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
    // getSceneInfo() will be called when the server need information
    // about the scene being rendered.
    server.onGetSceneInfo(&getSceneInfo);
    // setLayout() will be called to inform the renderer about sample layout requirements.
    server.onSetParameters(&setLayout);
    // evaluateSamples() will be called each time the server requires samples.
    server.onEvaluateSamples(&evaluateSamples);
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
