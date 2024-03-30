# Building NFD #

Most of the building instructions are included in [README.md](/README.md). This file just contains apocrypha.

## Running Premake5 Directly ##

*You shouldn't have to run Premake5 directly to build Native File Dialog.  This is for package maintainers or people with exotic demands only!*

1. [Clone premake-core](https://github.com/premake/premake-core)
2. [Follow instructions on how to build premake](https://github.com/premake/premake-core/wiki/Building-Premake)
3. `cd` to `build`
4. Type `premake5 <type>`, where <type> is the build you want to create.

### Package Maintainer Only ###

I support a custom Premake action: `premake5 dist`, which generates all of the checked in project types in subdirectories.  It is useful to run this command if you are submitting a pull request to test all of the supported premake configurations.  Do not check in the built projects; I will do so before accepting your pull request.

## SCons build (deprecated) ##

NFD used to use [SCons](http://www.scons.org) for cross-platform builds.  For the time being, the SCons scripts are still available.

After installing SCons, build it with:

    cd src
    scons debug=[0,1]

## Compiling with Mingw ##

Use the Makefile in `build/gmake_windows` to build Native File Dialog with mingw.  This is tested against the [msys2 distribution](https://msys2.github.io/).
