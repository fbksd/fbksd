#include "Benchmark/BenchmarkClient/BenchmarkClient.h"


int use_samplelayout()
{
    //![0]
    SampleLayout layout;
    layout  ("IMAGE_X")
            ("IMAGE_Y")
            ("COLOR_R")
            ("COLOR_G")
            ("COLOR_B");

    //![0]

    //![1]
    layout  ("IMAGE_X", SampleLayout::INPUT)
            ("IMAGE_Y", SampleLayout::INPUT);

    //![1]

    //![2]
    layout.setElementIO("IMAGE_X", SampleLayout::INPUT);
    layout.setElementIO("IMAGE_Y", SampleLayout::INPUT);
    //![2]

    //![3]
    layout  ("WORLD_Z")[0]
            ("WORLD_Z")[1];

    //![3]

    return 0;
}
