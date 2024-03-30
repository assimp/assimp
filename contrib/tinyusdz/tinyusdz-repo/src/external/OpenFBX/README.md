[![Discord Chat](https://img.shields.io/discord/480318777943392266.svg)](https://discord.gg/RgFybs6) 
[![License](http://img.shields.io/:license-mit-blue.svg)](http://doge.mit-license.org)

# OpenFBX

Lightweight open source FBX importer. Used in [Lumix Engine](https://github.com/nem0/lumixengine) and [Flax Engine](https://flaxengine.com/). It's an *almost* full-featured importer. It can load geometry (with uvs, normals, tangents, colors), skeletons, animations, blend shapes, materials and textures.

[UFBX](https://github.com/bqqbarbhg/ufbx) is similar project in C.

## Use the library in your own project

Note: It's recommended to be familiar with fbx format to use this library, you can read about it more [here](http://help.autodesk.com/view/FBX/2017/ENU/?guid=__files_GUID_F194000D_5AD4_49C1_86CC_5DAC2CE64E97_htm).

1. add files from src to your project
2. use

See [demo](https://github.com/nem0/OpenFBX/blob/master/demo/main.cpp#L203) as an example how to use the library.
See [Lumix Engine](https://github.com/nem0/LumixEngine/blob/master/src/renderer/editor/fbx_importer.cpp) as more advanced use case.

Alternatively, CMake support is provided by community but it's not supported by me - @nem0.

## Compile demo project

1. download source code
2. execute [projects/genie_vs19.bat](https://github.com/nem0/OpenFBX/blob/master/projects/genie_vs19.bat)
3. open projects/tmp/vs2019/OpenFBX.sln in Visual Studio 2019
4. compile and run

Demo is windows only. Library is multiplatform.

![ofbx](https://user-images.githubusercontent.com/153526/27876079-eea3c872-61b5-11e7-9fce-3a7c558fb0d2.png)
