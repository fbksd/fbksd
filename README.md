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

If you have an existing technique that you want to run with FBKSD, take a look at the library [API reference](http(s)://fbksd.github.io/fbksd/docs/1.0.0).

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

To initialize a workspace, just create the workspace folder (let's call it `workspace`, by it can be any name) and call `fbksd init` from it:

```
$ mkdir workspace && cd workspace
$ fbksd init 
```

To get you started, we provide a package containing several techniques, renderers, and scenes in a separate repository (https://github.com/fbksd/fbksd-package).
This package should be installed in workspace.
Follow the instructions on the [fbksd-package](https://github.com/fbksd/fbksd-package) repository.

After the package in your workspace, you can list the filters, samplers and scenes using the `filters`, `samplers`, and `scenes` subcommands. Ex:

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

> **Note:** Remember to run `fbksd` with the workspace as current directory.

### Adding Scenes

The `<workspace>/scenes` folder contains all the scenes available for benchmarking.
The scenes are organized by renderer (ex: `scenes/pbrt-v2`, `scenes/mitsuba`, etc.).

Each renderer subfolder in `scenes` contains files that describe the scenes for FBKSD.
The files can be named `fbksd-scenes.json` and `fbksd-scene.json`.
The different between them is that the first contains a vector with multiple scene entries, while the second contains just one scene entry.

A scene entry has the following format:

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

> **Note:** Only the `exr` image is included in the scene entry, the others are assumed to be in the same directory. And `<ref>` can be any name.

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


## API Reference

The API reference for release 1.0.0 can be accessed in: https://fbksd.github.io/fbksd/docs/1.0.0
