# FBKSD

## Table of Contents

- [Introduction](#introduction)
- [Build and Install](#build-and-install)
- [Creating a Workspace](#creating-a-workspace)
  - [Adding Denoisers](#adding-denoisers)
  - [Adding Samplers](#adding-samplers)
  - [Adding Scenes](#adding-scenes)
  - [Adding IQA metrics](#adding-iqa-metrics)
- [Configure and Run a Benchmark](#configure-and-run-a-benchmark)
- [API Reference](#api-reference)


## Introduction

FBKSD is a framework for developing and benchmarking sampling and denoising algorithms for Monte Carlo rendering.

This repository contains the core of the FBKSD system. It consists of:

- C++ libraries that support adding new techniques and renderers;
- Python bindings;
- the main command line interface (`fbksd`) used to manage and run benchmarks;
- other miscellaneous stuff.

The C++ libraries provide the API needed to make techniques (samplers and denoisers) compatible with FBKSD, so they can be
benchmarked and compared using Image Quality Assessment (IQA) metrics.
The libraries also support adapting renderers, which allow FBKSD to use them as rendering back-ends.

For details about the motivation and system architecture, refer to our paper: [A Framework for Developing and Benchmarking Sampling and Denoising Algorithms for Monte Carlo Rendering](http://www.inf.ufrgs.br/~oliveira/projects/FBKSD/FBKSD_page.html)


## Build and Install

### System Requirements

- fbksd was tested on Ubuntu 18.04, but it should work in any modern Linux distribution with a recent cmake version;
- The file system must support symbolic links;

### Dependencies

* Qt 5
* Boost
* OpenEXR
* OpenCV 3

For the Python bindings (optional), it also needs the packages:

* libboost-python-dev
* libboost-numpy-dev
* python3-dev
* python3-numpy

After installing the dependencies, clone the repository with the `--recursive-submodules` flag:

```text
$ git clone --recurse-submodules https://github.com/fbksd/fbksd.git
```

and run your normal cmake build/install procedure:

```text
$ cd fbksd
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ../
$ make
$ sudo make install
```

**Caution**: This repository uses git submodules. Use `--recurse-submodules` when cloning it.

The Python bindings are enabled by default.
You can disabled it by turning off the `FBKSD_PYTHON` option when running cmake:

```text
$ cmake -DCMAKE_BUILD_TYPE=Release -DFBKSD_PYTHON=OFF ../
```

## Creating a Workspace

A workspace is a folder that contain the techniques, renderers, scenes, configurations, and results.
It's also the working directory from where you'll run the `fbksd` CLI.

To initialize a workspace, just create the workspace folder (let's call it `workspace`) and call `fbksd init` from it:

```text
$ mkdir workspace && cd workspace
$ fbksd init
```

To get you started, we provide a package containing several techniques, renderers, and scenes in a [separate repository](https://github.com/fbksd/fbksd-package/tree/fbksd-2.3.0).
This package should be installed in your workspace, following the instructions on the fbksd-package repository.

After the package is installed in your workspace, you can list the filters, samplers and scenes using the `filters`, `samplers`, and `scenes` subcommands. Ex:

```text
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

### Adding Denoisers

Each denoiser should be placed in its own subdirectory inside the `<workspace>/denoisers` directory.
The directory should contain the denoiser executable and a `info.json` file containing information about the denoiser. For example, the `info.json` for the box filter is:

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

> **NOTE:** The `"name"` field in the version entry (except the `"default"` one) will be appended to the `"short_name"` field to form the name of the technique when displaying it in the `fbksd` CLI or in the results visualization page.

Once the executable and the `info.json` file are installed in the workspace, the command `fbksd filters` should now list your technique.

Ex:

```text
$ fbksd filters
Id   Name                Status
---------------------------------------------------------------------------
...
11   LBF                 ready
12   LBF-mf              ready
...
---------------------------------------------------------------------------
```

See the [API reference](#api-reference) for more details about how to implement a denoiser for FBKSD.

### Adding Samplers

The process for adding samplers is similar to adding denoisers.
The only difference is that the sampler folder should be placed in the `<workspace>/samplers` directory.


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
  - `.exr` in full size: `<ref>.exr` (where `<ref>` can be any name)
  - `.png` in full size: `<ref>.png`
  - `.jpg` with width 256px: `<ref>_thumb256.jpg`
- a scene entry for the scene in a new `fbksd-scene.json` file, or in an existing `fbksd-scenes.json` file.

> **Note:** Only the `exr` image is included in the scene entry, the others are assumed to be in the same directory.

The `Chess` scenes folder for `pbrt-v2`, for example, has the following layout:

```text
scenes/pbrt-v2/chess/chess.pbrt
scenes/pbrt-v2/chess/fbksd-scene.pbrt
scenes/pbrt-v2/chess/reference/ref.exr
scenes/pbrt-v2/chess/reference/ref.png
scenes/pbrt-v2/chess/reference/ref_thumb256.jpg
...
```

After adding new scenes, run the `fbksd scenes --update` command  to update the scenes cache file.

### Adding IQA Metrics

After executing a benchmark, FBKSD uses Image Quality Assessment (IQA) metrics to compare the images produced by the samplers and denoisers with the provided reference images for each scene.
The values produces by each IQA metric are then used to produce the result charts and rank the techniques.

To add include a new IQA metric for FBKSD, its executable should be placed inside a sub-folder of `<workspace>/iqa/`, alongside a `info.json` file describing the metric:

```json
{
    "acronym": "MSE",
    "name": "Mean squared error metric",
    "reference": "",
    "lower_is_better": true,
    "has_error_map": true,
    "executable": "mse"
}
```

After adding the metric in the workspace, it should be listed by the command `fbksd metrics` and will be used alongside all other metrics when computing results, visualizing results in the visualization page.

Ex:
```text
$ fbksd metrics
------------------------------------------------------------
Acronym   Name
------------------------------------------------------------
MSE       Mean squared error metric
...
```

See the [API reference](#api-reference) for more details about how to implement a IQA metric for FBKSD.


## Configure and Run a Benchmark

A config is a set of scenes and techniques to be executed in batch.
To create a config, use the `fbksd config new` command (see `fbksd config new -h` for help).

After the config is created, execute it with `fbksd run`.
The execution may take a while, depending on the size of you config (number of scenes, techniques, etc.).


### Compute and Visualize Results

Once the run finishes, execute the `fbksd results compute` command.
This will compare the images generated during the run with the provided reference images using the available IQA metrics.

When the computation is finished, you can run a `fbksd page serve` to serve the results visualization web page on localhost.

See the `fbksd` CLI build-in help for more info about options commands and options (`fbksd --help`).


## API Reference

For more details about how to develop technique (denoisers, samplers, and IQA metrics) for FBKSD, or add support for new renderers, see the C++ API reference: https://fbksd.github.io/fbksd/docs/latest.
We also provide Python bindings for denoising and sampling techniques: https://fbksd.github.io/fbksd/docs/python/latest.

To see the reference for a specific version, replace `latest` in the link by the version number.
Ex: https://fbksd.github.io/fbksd/docs/2.3.0
