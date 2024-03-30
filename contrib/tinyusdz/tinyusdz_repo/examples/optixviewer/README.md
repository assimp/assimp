# OptiX viewer

## Status 

Not yet working.

## Requirements

* OptiX 7.6 or later
* NVIDIA GPU
* Visual Studio 2019(Windows)

## Build on Windows

Edit path to OptiX in vcsetup-2019.bat if required.

```
> vcsetup-2019.bat
```

## Build on Linux

```
$ rm -rf build
$ mkdir build
# Set path to OptiX SDK
$ cmake -DOptiX_INSTALL_DIR=${HOME}/local/NVIDIA-OptiX-SDK-7.6.0-linux64-x86_64 -B build -S .
```

