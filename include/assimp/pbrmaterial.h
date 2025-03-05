/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file pbrmaterial.h
 *  @brief Deprecated GLTF_PBR macros
 */
#pragma once
#ifndef AI_PBRMATERIAL_H_INC
#define AI_PBRMATERIAL_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#   warning pbrmaterial.h is deprecated. Please update to PBR materials in materials.h and glTF-specific items in GltfMaterial.h
#else if defined(_MSC_VER)
#   pragma message("pbrmaterial.h is deprecated. Please update to PBR materials in materials.h and glTF-specific items in GltfMaterial.h")
#endif

#include <assimp/material.h>
#include <assimp/GltfMaterial.h>

#define AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR AI_MATKEY_BASE_COLOR
#define AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE AI_MATKEY_BASE_COLOR_TEXTURE
#define AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR AI_MATKEY_METALLIC_FACTOR
#define AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR AI_MATKEY_ROUGHNESS_FACTOR

#define AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS AI_MATKEY_GLOSSINESS_FACTOR
#define AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS_GLOSSINESS_FACTOR AI_MATKEY_GLOSSINESS_FACTOR

// Use AI_MATKEY_SHADING_MODEL == aiShadingMode_Unlit instead
#define AI_MATKEY_GLTF_UNLIT "$mat.gltf.unlit", 0, 0

//AI_MATKEY_GLTF_MATERIAL_SHEEN
#define AI_MATKEY_GLTF_MATERIAL_SHEEN_COLOR_FACTOR AI_MATKEY_SHEEN_COLOR_FACTOR
#define AI_MATKEY_GLTF_MATERIAL_SHEEN_ROUGHNESS_FACTOR AI_MATKEY_SHEEN_ROUGHNESS_FACTOR
#define AI_MATKEY_GLTF_MATERIAL_SHEEN_COLOR_TEXTURE AI_MATKEY_SHEEN_COLOR_TEXTURE
#define AI_MATKEY_GLTF_MATERIAL_SHEEN_ROUGHNESS_TEXTURE AI_MATKEY_SHEEN_ROUGHNESS_TEXTURE

//AI_MATKEY_GLTF_MATERIAL_CLEARCOAT
#define AI_MATKEY_GLTF_MATERIAL_CLEARCOAT_FACTOR AI_MATKEY_CLEARCOAT_FACTOR
#define AI_MATKEY_GLTF_MATERIAL_CLEARCOAT_ROUGHNESS_FACTOR AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR
#define AI_MATKEY_GLTF_MATERIAL_CLEARCOAT_TEXTURE AI_MATKEY_CLEARCOAT_TEXTURE
#define AI_MATKEY_GLTF_MATERIAL_CLEARCOAT_ROUGHNESS_TEXTURE AI_MATKEY_CLEARCOAT_ROUGHNESS_TEXTURE
#define AI_MATKEY_GLTF_MATERIAL_CLEARCOAT_NORMAL_TEXTURE AI_MATKEY_CLEARCOAT_NORMAL_TEXTURE

//AI_MATKEY_GLTF_MATERIAL_TRANSMISSION
#define AI_MATKEY_GLTF_MATERIAL_TRANSMISSION_FACTOR AI_MATKEY_TRANSMISSION_FACTOR
#define AI_MATKEY_GLTF_MATERIAL_TRANSMISSION_TEXTURE AI_MATKEY_TRANSMISSION_TEXTURE

#define AI_MATKEY_GLTF_TEXTURE_TEXCOORD(type, N) AI_MATKEY_UVWSRC(type, N)

#endif //!!AI_PBRMATERIAL_H_INC
