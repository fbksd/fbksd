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
    "versions": [
        {
            "name": "default",
            "comment": "Default version using random sampler.",
            "executable": "Box"
        },
    ]
}
```

Once the executable and the `info.json` file are installed in the workspace, the command `fbksd filters` should now list your technique. You can then add it to a config and run it.


## Porting a Renderer to FBKSD

For porting renderers to be used as rendering back-ends, see the fbksd::RenderingServer class. It contains the main API used to provide sample data to the benchmark server.
