# FBKSD

## Introduction

FBKSD is a framework for developing and benchmarking sampling and denoising algorithms for Monte Carlo rendering.

This repository contains the core of the FBKSD system. It consists of:

- C++ libraries that support adding new techniques and renderers;
- the main command line interface (`fbksd`) used to manage and run benchmarks;
- other miscellaneous stuff.

The C++ libraries provide the API needed to make techniques (samplers and denoisers) compatible with FBKSD, so they can be
benchmarked and compared.
The libraries also support adapting renderers, which allow FBKSD to use them as rendering back-ends.

If you have an existing technique that you want to run with FBKSD, take a look at the library [API reference](http://fbksd.github.io).

For details about the motivation and system architecture, refer to our paper: [A Framework for Developing and Benchmarking Sampling and Denoising Algorithms for Monte Carlo Rendering](http://www.inf.ufrgs.br/~oliveira/projects/FBKSD/FBKSD_page.html)

## System Requirements

- fbksd was tested on Ubuntu 18.04, but it should work in any modern Linux distribution with a recent cmake version;
- The file system must support symbolic links: e.g. EXT4;
- FBKSD version 1.x.x needs plenty of RAM (at least 16 GB) and you'll need to increase the POSIX shared memory size limit (see below).

### Increase The POSIX Shared Memory Size Limit

By default, Linux systems limit the total amount of shared memory to 50% of the total systems' RAM. You need to set it to 100% adding this line to the `/etc/fstab` file:

```
tmpfs    /dev/shm    tmpfs    rw,nosuid,nodev,size=100%    0    0
```

After adding this line, reboot the system.

You can check if the limit is correctly set to 100% using the command `df -h | egrep 'Size|/dev/shm'`.
The printed `Size` value should read the same as the total RAM size in your system. Ex:

```
$ df -h | egrep 'Size|/dev/shm'
Filesystem      Size  Used Avail Use% Mounted on
tmpfs            32G   29M   32G   1% /dev/shm
```


## Build and Install

### Dependencies

* Qt 5
* Boost
* OpenEXR
* OpenCV

After installing the dependencies, just run your normal cmake build/install procedure:

```
$ cd fbksd
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ../
$ make install
```



## Creating a Workspace

A workspace is a folder that contain the techniques, renderers, scenes, configurations, and results.
It's also the working directory from where you'll run the `fbksd` CLI.

To get you started, we provide a package containing several techniques, renderers, and scenes in a separate repository (https://github.com/fbksd/fbksd-package).

To initialize a workspace, just create the workspace folder (let's call it `workspace`) and call `fbksd init` from it:

```
$ mkdir workspace && cd workspace
$ fbksd init 
```

Download and build the provided package, and install it in yor workspace:

```
$ git clone --recursive <package repo> && cd fbksd-package
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=workspace ../
$ cmake install
```
> It's important that you set your workspace folder as the install directory for the package, as above.

After installing the techniques, renderers, and scenes in the workspace, you can list them using the `filters`, `samplers`, and `scenes` subcommands. Ex:

```
$ fbksd filters
Id   Name                Status    
---------------------------------------------------------------------------
1    Mitchell            ready     
2    Box                 ready 
...

$ fbksd scenes
Id   Name                                              Renderer            
---------------------------------------------------------------------------
1    Conference                                        pbrt-v2             
2    Chess                                             pbrt-v2
...
```

> Remember to run `fbksd` with the workspace as current directory.

### Adding Scenes

The `<workspace>/scenes` folder contains all the scenes available for benchmarking.
The scenes are organized by renderer (e.g. `scenes/pbrt-v2`, `scenes/mitsuba`, etc.).

Each renderer subfolder in scenes contains files that describe the scenes for FBKSD.
The files can be named `fbksd-scenes.json` and `fbksd-scene.json`.
The different between them is that one contains a vector with multiple scene entries, while the other contains just one scene entry.

A scene entry in a `fbksd-scene.json` file has the following format:

```json
{
    "name": "Chess",
    "path": "chess.pbrt",
    "ref-img": "reference/ref.exr"
}
```

To add a new scene for a certain renderer, you have to provide:

- the actual scene files for the renderer, organized in a separate folder
- reference images for the scene in three formats:
  - `.exr` in full size (`<ref>.exr`)
  - `.png` in full size (`<ref>.png`)
  - `.jpg` with width 256px (`<ref>_thumb256.jpg`)
- a scene entry for the scene (in a new `fbksd-scene.json` file, or in an existing `fbksd-scenes.json` file).

> Only the `exr` image is included in the scene entry, the others are assumed to be in the same directory.

> `<ref>` can be any name.

The `Chess` scenes for `pbrt-v2`, for example, has the following layout:

```
scenes/pbrt-v2/chess/chess.pbrt
scenes/pbrt-v2/chess/fbksd-scene.pbrt
scenes/pbrt-v2/chess/reference/ref.exr
scenes/pbrt-v2/chess/reference/ref.png
scenes/pbrt-v2/chess/reference/ref_thumb256.jpg
...
```


## Configure and Run a Benchmark

A config is a set of scenes and techniques to be executed.
Create a config using the `fbksd config new` command (see `fbksd config new -h` for help).

After the config is created, execute it with `fbksd run`.
The execution may take a while, depending on the size of you config (number of scenes, techniques, etc.).

## Compute and Visualize Results

Once the run finishes. Execute a `fbksd results compute`.
This will compare the resulting images with the corresponding reference images and generate error values using different error metrics (e.g. MSE, PSNR, SSIM, rMSE).

When the computation is finished, you can run a `fbksd page serve` to serve a visualization web page for the results on localhost.



## Creating a New Filter

The simplest way is to use one of the provided filters as a template, let's say, the **Box** filter.

Copy and rename the `Filters/Box` filter folder as you like:
```
$ cp Filters/Box Filters/MyFilter
```

Adjust the `CMakeLists.txt` file (renames `Box` to `MyFilter`):
```
$ sed -ie 's/Box/MyFilter/g' Filters/MyFilter/CMakeList.txt
```
Edit the `info.json` file with the information about your filter.

To be able to compile your filter together with the rest of the system, include the appropriate entry in the `Filters/CMakeLists.txt`.
After recompiling the system, your filter's executable should be in the path `<build_dir>/Filters/MyFilter/MyFilter`.

Now it will show in the `filters` command, and can be executed as the others.

### API Reference

To implement a new filter, you'll be using the C++ `BenchmarkClient` class to communicate with the system. The API reference documentation can be generated using doxygen:
```
$ doxygen Benchmark/Doxyfile
```
Then, open the file `Benchmark/doxygen/html/index.html` in a browser.

## Adding a New Renderer (advanced)

fbksd currently supports scenes from three well known renderers: PBRT-V2, PBRT-V3, and Mitsuba. It also has a fourth experimental procedural renderer, which is not being used yet.

If you want to use scenes from another renderer, you have to adapt the renderer's source code and add it as a rendering back-end for the system.

To give you an idea about the modifications required to port a renderer to fbksd, next section explains how PBRT-V3 was ported.

### Adapting the source code

#### Build System

Renderers are kept in the `Renderers` folder, and are built by `add_subdirectory` entries in the `Renderers/CMakeLists.txt` file. This assumes the target renderer build system is CMake and it's ready to be built as a CMake subdirectory.
If the target renderer does not use CMake, you'd have create it from scratch.

PBRT-V3 already uses CMake, so it just requires a few adjustments to make it compatible as a CMake subdirectory: replace `CMAKE_BINARY_DIR` and `CMAKE_SOURCE_DIR` by `PROJECT_BINARY_DIR` and `PROJECT_SOURCE_DIR`, respectively;

```cpp
class Renderer {
public:
    Renderer(int argc, char** argv) {
        // creates scene, camera, sampler, and integrator
    }

    int render() {
        for(Pixel pixel: camera.getPixels()) {
            Sample sample = sampler.samplePixel(pixel);
            Spectrum L = integrator.L(sampler, sample);
            camera.sensor.add(L);
        }

        return 0;
    }

private:
    Scene* scene;
    Camera* camera;
    Sampler* sampler;
    Integrator* integrator;
}

int main(int argc, char** argv) {
    Renderer renderer(argc, argv);
    return renderer.render();
}
```