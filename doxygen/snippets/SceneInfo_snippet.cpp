#include "../src/BenchmarkClient/BenchmarkClient.h"


int use_SceneDescription()
{

    //![0]
    BenchmarkClient client(argc, argv);
    SceneInfo scene = client.getSceneInfo();

    const auto w = scene.get<int64_t>("width");
    const auto h = scene.get<int64_t>("height");
    const auto spp = scene.get<int64_t>("max_spp");
    //![0]

    return 0;
}
