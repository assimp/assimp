# virtualGizmo3D &nbsp;v2.1.3
**virtualGizmo3D** is an 3D GIZMO manipulator: like a trackball it provides a way to rotate, move and scale a model, with mouse, also with dolly and pan features
You can also define a way to rotate the model around any single axis.
It use mouse movement on screen, mouse buttons and (eventually) key modifiers, like *Shift/Ctrl/Alt/Super*, that you define

It uses **quaternions** algebra, internally, to manage rotations, but you can also only pass your model matrix and gat back a transformation matrix with rotation, translation and scale, inside.

![alt text](https://raw.githubusercontent.com/BrutPitt/virtualGizmo3D/master/screenshots/oglGizmo.gif)

**virtualGizmo3D** is an *header only* tool (`vGizmo.h`) and **is not bound to frameworks or render engines**, is written in C++ (C++11) and uses [**vgMath**](https://github.com/BrutPitt/vgMath) a compact (my *single file header only*) vectors/matrices/quaternions tool/lib that makes **virtualGizmo3D** standalone.

**No other files or external libraries are required**

In this way you can use it with any engine, like: *OpenGL, DirectX, Vulkan, Metal, etc.* and/or  with any framework, like: *GLFW, SDL, GLUT, Native O.S. calls, etc.*

You can use [**vgMath**](https://github.com/BrutPitt/vgMath) also externally, for your purposes: it contains classes to manipulate **vec**tors (with 2/3/4 components), **quat**ernions, square **mat**ricies (3x3 and 4x4), both as *simple* single precision `float` **classes** (*Default*) or, enabling **template classes** (*simply adding a* `#define`), as both `float` and `double` data types. It contains also 4 helper functions to define Model/View matrix: **perspective**, **frustum**, **lookAt**, **ortho**

If need a larger/complete library, as alternative to **virtualGizmo3D**, is also possible to interface **imGuIZMO.quat** with [**glm** mathematics library](https://github.com/g-truc/glm) (*simply adding a* `#define`)

==>&nbsp; **Please, read [**Configure virtualGizmo3D**](#Configure-virtualGizmo3D) section.*

<p>&nbsp;<br></p>


### Live WebGL2 example
You can run/test an emscripten WebGL 2 example of **virtualGismo3D** from following link:
- [virtualGizmo3D WebGL2](https://www.michelemorrone.eu/emsExamples/oglGizmo.html)

It works only on browsers with **WebGl2** and *webAssembly* support (FireFox/Opera/Chrome and Chromium based): test if your browser supports **WebGL2**, here: [WebGL2 Report](http://webglreport.com/?v=2)

### How to use virtualGizmo3D in your code

To use **virtualGizmo3D** need to include `virtualGizmo.h` file in your code and declare an object of type vfGizmo3DClass, global or as member of your class 

```cpp
#include "vGizmo.h"

// Global or member class declaration
using namespace vg;
vGizmo3D gizmo; 
vGizmo3D &getGizmo() { return gizmo; }  //optional helper
```

In your 3D engine *initialization* declare (overriding default ones) your preferred controls:

**GLFW buttons/keys initialization**

```cpp
void onInit()
{
    //If you use a different framework simply associate internal ID with your preferences

    //For main manipulator/rotation
    getGizmo().setGizmoRotControl( (vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers) 0 /* evNoModifier */ );

    //for pan and zoom/dolly... you can use also wheel to zoom
    getGizmo().setDollyControl((vgButtons) GLFW_MOUSE_BUTTON_RIGHT, (vgModifiers) GLFW_MOD_CONTROL|GLFW_MOD_SHIFT);
    getGizmo().setPanControl(  (vgButtons) GLFW_MOUSE_BUTTON_RIGHT, (vgModifiers) 0);

    // Now call viewportSize with the dimension of window/screen
    // It is need to set mouse sensitivity for rotation
    // You need to call it also in your "reshape" function: when resize the window (look below)
    getGizmo().viewportSize(GetWidth(), GetHeight());

    // more... 
    // if you need to rotate around a single axis have to select your preferences: uncomment below...
    //
    // getGizmo().setGizmoRotXControl((vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers) GLFW_MOD_SHIFT); // around X pressing SHIFT+LButton
    // getGizmo().setGizmoRotYControl((vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers) GLFW_MOD_CONTROL); // around Y pressing CTRL+LButton
    // getGizmo().setGizmoRotZControl((vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers) GLFW_MOD_ALT | GLFW_MOD_SUPER); // around Z pressing ALT|SUPER+LButton


}    
```
**SDL buttons/keys Initialization**

```cpp
void onInit()
{
    //If you use a different framework simply associate internal ID with your preferences

    //For main manipulator/rotation
    getGizmo().setGizmoRotControl( (vgButtons) SDL_BUTTON_LEFT, (vgModifiers) 0 /* evNoModifier */ );

    //for pan and zoom/dolly... you can use also wheel to zoom
    getGizmo().setDollyControl((vgButtons) SDL_BUTTON_RIGHT, (vgModifiers) 0);
    getGizmo().setPanControl(  (vgButtons) SDL_BUTTON_RIGHT, (vgModifiers) KMOD_CTRL|KMOD_SHIFT);

    // Now call viewportSize with the dimension of window/screen
    // It is need to set mouse sensitivity for rotation
    // You need to call it also in your "reshape" function: when resize the window (look below)
    getGizmo().viewportSize(GetWidth(), GetHeight());

    // more... 
    // if you need to rotate around a single axis have to select your preferences: uncomment below...
    //
    // getGizmo().setGizmoRotXControl((vgButtons) SDL_BUTTON_LEFT, (vgModifiers) KMOD_SHIFT); // around X pressing SHIFT+LButton
    // getGizmo().setGizmoRotYControl((vgButtons) SDL_BUTTON_LEFT, (vgModifiers) KMOD_CTRL); // around Y pressing CTRL+LButton
    // getGizmo().setGizmoRotZControl((vgButtons) SDL_BUTTON_LEFT, (vgModifiers) KMOD_ALT); // around Z pressing ALT+LButton

}    
```
Now you need to add some *event* funtions:

In your *Mouse-Button Event* function need to call:
```cpp
void onMouseButton(int button, int upOrDown, int x, int y)
{
    //  Call in 'mouse button event' the gizmo.mouse() func with:
    //      button:  your mouse button
    //      mod:     your modifier key -> CTRL, SHIFT, ALT, SUPER
    //      pressed: if button is pressed (TRUE) or released (FALSE)
    //      x, y:    mouse coordinates
    bool isPressed = upOrDown==GLFW_PRESS; // or upOrDown==SDL_MOUSEBUTTONDOWN for SDL
    getGizmo().mouse((vgButtons) (button), (vgModifiers) theApp->getModifier(), isPressed, x, y);

}
```

In your *Mouse-Motion Event* function need to call:
```cpp
void onMotion(int x, int y)
{
    //  Call on motion event to communicate the position
    getGizmo().motion(x, y);
}
```

And in your *Resize-Window Event* function :
```cpp
void onReshape(GLint w, GLint h)
{
    // call it on resize window to re-align mouse sensitivity
    getGizmo().viewportSize(w, h);
}
```

And finally, in your render function (or where you prefer) you can get the transformations
```cpp
void onRender() //or when you prefer
{
    mat4 model(1.0f);                          // Identity matrix

    // virtualGizmo transformations
    getGizmo().applyTransform(model);           // apply transform to matrix model

    // Now the matrix 'model' has inside all the transformations:
    // rotation, pan and dolly translations, 
    // and you can build MV and MVP matrix
}
```

<p>&nbsp;<br></p>

 
### Other useful stuff

If you need to more feeling with the mouse use:
`getGizmo().setGizmoFeeling(1.0); // 1.0 default,  > 1.0 more sensible, < 1.0 less`

Same thing for Dolly and Pan:

```cpp
 getGizmo().setDollyScale(1.0f);
 getGizmo().setPanScale(1.0f);
```
You probably will need to set center of rotation (default: origin), Dolly position (default: 1.0), and Pan position (default: vec2(0.0, 0.0)

```cpp
 getGizmo().setDollyPosition(1.0f); 
 getGizmo().setPanPosition(vec3(0.0f);
 getGizmo().setRotationCenter(vec3(0.0));
```

If you want a *continuous rotation*, that you can stop with a click, like in example, need to add the below call in your Idle function, or inside of the main render loop

```cpp
void onIdle()
{
    // call it every rendering if want an continue rotation until you do not click on screen
    // look at glApp.cpp : "mainLoop" ("newFrame") functions

    getGizmo().idle();
}
```

**Class declaration**

The include file `vGizmo.h` contains two classes:
- `virtualGizmoClass` simple rotation manipulator, used mainly for [**imGuIZMO.quat**](https://github.com/BrutPitt/imGuIZMO.quat) (a GIZMO widget developed for ImGui, Graphic User Intefrace)
- `virtualGizmo3DClass` manipulator (like above) with dolly/zoom and pan/shift
- Template classes are also available if configured. &nbsp; **(read below)*

Helper `typedef` are also defined:
```cpp
    using vGizmo    = virtualGizmoClass;
    using vGizmo3D  = virtualGizmo3DClass;
```
<p>&nbsp;<br></p>

## Configure virtualGizmo3D
**virtalGizmo3D** uses [**vgMath**](https://github.com/BrutPitt/vgMath) tool, it contains a group of vector/matrices/quaternion classes, operators, and principal functions. It uses the "glsl" convention for types and function names so is compatible with **glm** types and function calls: [**vgMath**](https://github.com/BrutPitt/vgMath) is a subset of [**glm** mathematics library](https://github.com/g-truc/glm) and so you can use first or upgrade to second via a simple `#define`. However **vgMath** does not want replicate **glm**, is only intended to make **virtalGizmo3D** standalone, and avoid **template classes** use in the cases of low resources or embedded systems.


The file `vgConfig.h` allows to configure internal math used form **virtalGizmo3D**. In particular is possible select between:
 - static **float** classes (*Default*) / template classes 
 - internal **vgMath** tool (*Default*) / **glm** mathematics library
 - **Right** (*Default*) / **Left** handed coordinate system (*lookAt, perspective, ortho, frustum - functions*)
 - **enable** (*Default*) / **disable** automatic entry of `using namespace vgm;` at end of `vgMath.h` (it influences only your external use of `vgMath.h`)
- Add additional HLSL types name convention
 
You can do this simply by commenting / uncommenting a line in `vgConfig.h` or adding related "define" to your project, as you can see below:

```cpp
// uncomment to use TEMPLATE internal vgMath classes/types
//
// This is if you need to extend the use of different math types in your code
//      or for your purposes, there are predefined alias:
//          float  ==>  vec2 / vec3 / vec4 / quat / mat3|mat3x3 / mat4|mat4x4
//      and more TEMPLATE (only!) alias:
//          double ==> dvec2 / dvec3 / dvec4 / dquat / dmat3|dmat3x3 / dmat4|dmat4x4
//          int    ==> ivec2 / ivec3 / ivec4
//          uint   ==> uvec2 / uvec3 / uvec4
// If you select TEMPLATE classes the widget too will use internally them 
//      with single precision (float)
//
// Default ==> NO template
//------------------------------------------------------------------------------
//#define VGM_USES_TEMPLATE
```
```cpp
// uncomment to use "glm" (0.9.9 or higher) library instead of vgMath
//      Need to have "glm" installed and in your INCLUDE research compiler path
//
// vgMath is a subset of "glm" and is compatible with glm types and calls
//      change only namespace from "vgm" to "glm". It's automatically set by
//      including vGizmo.h or vgMath.h or imGuIZMOquat.h
//
// Default ==> use vgMath
//      If you enable GLM use, automatically is enabled also VGM_USES_TEMPLATE
//          if you can, I recommend to use GLM
//------------------------------------------------------------------------------
//#define VGIZMO_USES_GLM
```
```cpp
// uncomment to use LeftHanded 
//
// This is used only in: lookAt / perspective / ortho / frustrum - functions
//      DX is LeftHanded, OpenGL is RightHanded
//
// Default ==> RightHanded
//------------------------------------------------------------------------------
//#define VGM_USES_LEFT_HAND_AXES
```
**From v.2.1**
```cpp
// uncomment to avoid vgMath.h add folow line code:
//      using namespace vgm | glm; // if (!VGIZMO_USES_GLM | VGIZMO_USES_GLM)
//
// Automatically "using namespace" is added to the end vgMath.h:
//      it help to maintain compatibilty between vgMath & glm declaration types,
//      but can go in confict with other pre-exist data types in your project
//
// note: this is only if you use vgMath.h in your project, for your data types:
//       it have no effect for vGizmo | imGuIZMO internal use
//
// Default ==> vgMath.h add: using namespace vgm | glm;
//------------------------------------------------------------------------------
//#define VGM_DISABLE_AUTO_NAMESPACE
```
```cpp
// uncomment to use HLSL name types (in addition!) 
//
// It add also the HLSL notation in addition to existing one:
//      alias types:
//          float  ==>  float2 / float3 / float4 / quat / float3x3 / float4x4
//      and more TEMPLATE (only!) alias:
//          double ==> double2 / double3 / double4 / dquat / double3x3 / double4x4
//          int    ==> int2 / int3 / int4
//          uint   ==> uint2 / uint3 / uint4
//
// Default ==> NO HLSL alia types defined
//------------------------------------------------------------------------------
//#define VGM_USES_HLSL_TYPES 
```
- *If your project grows you can upgrade/pass to **glm**, in any moment*
- *My [**glChAoS.P**](https://github.com/BrutPitt/glChAoS.P) project can switch from internal **vgMath** (`VGIZMO_USES_TEMPLATE`) to **glm** (`VGIZMO_USES_GLM`), and vice versa, only changing defines: you can examine it as example*


<p>&nbsp;<br></p>

## Building Example

The source code example shown in the animated gif screenshot, is provided.

In  example I use **GLFW** or **SDL2** (via `#define GLAPP_USE_SDL`) with **OpenGL**, but it is simple to change if you use Vulkan/DirectX/etc, other frameworks (like GLUT) or native OS access.

To use SDL framework instead of GLFW, uncomment `#define GLAPP_USE_SDL` in `glApp.h` file, or pass `-DGLAPP_USE_SDL` directly to compiler.
**CMake** users can pass command line `-DUSE_SDL:BOOL=TRUE` (or use relative GUI flag) to enable **SDL** framework instead of **GLFW**.

To build it you can use CMake (3.10 or higher) or the Visual Studio solution project (for VS 2017) in Windows.
You need to have [**GLFW**](https://www.glfw.org/) (or [**SDL**](https://libsdl.org/)) in your compiler search path (LIB/INCLUDE).

The CMake file is able to build also an [**EMSCRIPTEN**](https://kripken.github.io/emscripten-site/index.html) version, obviously you need to have installed EMSCRIPTEN SDK on your computer (1.38.10 or higher): look at or use the helper batch/script files, in main example folder, to pass appropriate defines/parameters to CMake command line.

To build the EMSCRIPTEN version, in Windows, with CMake, need to have **mingw32-make.exe** in your computer and search PATH (only the make utility is enough): it is a condition of EMSDK tool to build with CMake.

**For windows users that use vs2017 project solution:**
* To build **SDL** or **GLFW**, select appropriate build configuration
* If you have **GLFW** and/or **SDL** headers/library directory paths added to `INCLUDE` and `LIB` environment vars, the compiler find them.
* The current VisualStudio project solution refers to my environment variable RAMDISK (`R:`), and subsequent VS intrinsic variables to generate binary output:
`$(RAMDISK)\$(MSBuildProjectDirectoryNoRoot)\$(DefaultPlatformToolset)\$(Platform)\$(Configuration)\`, so without a RAMDISK variable, executable and binary files are outputted in base to the values of these VS variables, starting from root of current drive. &nbsp;&nbsp; *(you find built binary here... or change it)*
