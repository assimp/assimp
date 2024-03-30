//------------------------------------------------------------------------------
//  Copyright (c) 2018-2020 Michele Morrone
//  All rights reserved.
//
//  https://michelemorrone.eu - https://BrutPitt.com
//
//  twitter: https://twitter.com/BrutPitt - github: https://github.com/BrutPitt
//
//  mailto:brutpitt@gmail.com - mailto:me@michelemorrone.eu
//  
//  This software is distributed under the terms of the BSD 2-Clause license
//------------------------------------------------------------------------------
#pragma once

////////////////////////////////////////////////////////////////////////////////
//  v g M a t h   C O N F I G   start

//------------------------------------------------------------------------------
// EXPERIMENTAL ==> NOT FULL TESTED (YET)
//
// uncomment to use DOUBLE precision
//      It automatically enable also VGM_USES_TEMPLATE (read below)
// Default ==> SINGLE precision: float
//------------------------------------------------------------------------------
//#define VGM_USES_DOUBLE_PRECISION

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// uncomment to use "glm" (0.9.9 or higher) library instead of vgMath
//      Need to have "glm" installed and in your INCLUDE research compiler path
//
// vgMath is a subset of "glm" and is compatible with glm types and calls
//      change only namespace from "vgm" to "glm". It's automatically set by
//      including vGizmo.h or vgMath.h or imGuIZMOquat.h
//
// note: affects only virtualGizmo3D / imGuIZMO.quat on which library to use
//      internally: vgMath | glm
//
// Default ==> use vgMath
//      If you enable GLM use, automatically is enabled also VGM_USES_TEMPLATE
//          if you can, I recommend to use GLM
//------------------------------------------------------------------------------
//#define VGIZMO_USES_GLM

//------------------------------------------------------------------------------
// uncomment to use LeftHanded 
//
// This is used only in: lookAt / perspective / ortho / frustrum - functions
//      DX is LeftHanded, OpenGL is RightHanded
//
// Default ==> RightHanded
//------------------------------------------------------------------------------
//#define VGM_USES_LEFT_HAND_AXES

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// imGuiZmo.quat - v3.0 and later - (used only inside it)
//
//      Used to remove Pan & Dolly feature to imGuIZMO.quat widget and to use
//          only rotation feature (like v2.2 and above)
//
//          Pan/Dolly use virtualGizmo3DClass just a little bit complex of
//          virtualGizmoClass that uses only "quat" rotations
//          uncomment for very low resources ==> Pan & Dolly will be disabled
//
// Default ==> Pan & Dolly enabled 
//------------------------------------------------------------------------------
//#define IMGUIZMO_USE_ONLY_ROT

//------------------------------------------------------------------------------
// imGuiZmo.quat - v3.0 and later - (used only inside it)
//
//      used to specify where ImGui include files should be searched
//          #define IMGUIZMO_IMGUI_FOLDER  
//              is equivalent to use:
//                  #include <imgui.h>
//                  #include <imgui_internal.h>
//          #define IMGUIZMO_IMGUI_FOLDER myLibs/ImGui/
//              (final slash is REQUIRED) is equivalent to use: 
//                  #include <myLib/ImGui/imgui.h>
//                  #include <myLib/ImGui/imgui_internal.h>
//          Default: IMGUIZMO_IMGUI_FOLDER commented/undefined
//              is equivalent to use:
//                  #include <imgui/imgui.h>
//                  #include <imgui/imgui_internal.h>
//
// N.B. Final slash to end of path is REQUIRED!
//------------------------------------------------------------------------------
// #define IMGUIZMO_IMGUI_FOLDER ImGui/

//  v g M a t h   C O N F I G   end
////////////////////////////////////////////////////////////////////////////////
