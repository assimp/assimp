/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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

/** @file Defines constants for configurable properties */
#ifndef __AI_CONFIG_H_INC__
#define __AI_CONFIG_H_INC__

// ---------------------------------------------------------------------------
/** \brief Set the maximum number of vertices in a mesh.
 *
 * This is used by the "SplitLargeMeshes" PostProcess-Step to determine
 * whether a mesh must be splitted or not.
 * \note The default value is AI_SLM_DEFAULT_MAX_VERTICES, defined in
 *       the internal header file SplitLargeMeshes.h
 * Property type: integer.
 */
#define AI_CONFIG_PP_SLM_TRIANGLE_LIMIT	"pp.slm.triangle_limit"


// ---------------------------------------------------------------------------
/** \brief Set the maximum number of triangles in a mesh.
 *
 * This is used by the "SplitLargeMeshes" PostProcess-Step to determine
 * whether a mesh must be splitted or not.
 * \note The default value is AI_SLM_DEFAULT_MAX_TRIANGLES, defined in
 *       the internal header file SplitLargeMeshes.h
 * Property type: integer.
 */
#define AI_CONFIG_PP_SLM_VERTEX_LIMIT	"pp.slm.vertex_limit"


// ---------------------------------------------------------------------------
/** \brief Set the maximum number of bones affecting a single vertex
 *
 * This is used by the aiProcess_LimitBoneWeights PostProcess-Step.
 * \note The default value is AI_LBW_MAX_WEIGHTS, defined in
 *       the internal header file LimitBoneWeightsProcess.h
 * Property type: integer.
 */
#define AI_CONFIG_PP_LBW_MAX_WEIGHTS	"pp.lbw.weights_limit"


// ---------------------------------------------------------------------------
/** \brief Set the vertex animation keyframe to be imported
 *
 * ASSIMP does not support vertex keyframes (only bone animation is supported).
 * The library reads only one frame of models with vertex animations.
 * By default this is the first frame´.
 * \note The default value is 0. This option applies to all importers.
 *   However, it is also possible to override the global setting
 *   for a specific loader. You can use the AI_CONFIG_IMPORT_XXX_KEYFRAME
 *   options (where XXX is a placeholder for the file format for which you
 *   want to override the global setting).
 * Property type: integer.
 */
#define AI_CONFIG_IMPORT_GLOBAL_KEYFRAME	"imp.global.kf"

#define AI_CONFIG_IMPORT_MD3_KEYFRAME		"imp.md3.kf"
#define AI_CONFIG_IMPORT_MD2_KEYFRAME		"imp.md2.kf"
#define AI_CONFIG_IMPORT_MDL_KEYFRAME		"imp.mdl.kf"
#define AI_CONFIG_IMPORT_MDC_KEYFRAME		"imp.mdc.kf"
#define AI_CONFIG_IMPORT_MDR_KEYFRAME		"imp.mdr.kf"
#define AI_CONFIG_IMPORT_SMD_KEYFRAME		"imp.smd.kf"


// ---------------------------------------------------------------------------
/** \brief Configures the 3DS loader to ignore pivot points in the file
 *
 * There are some faulty 3DS files which look only correctly with
 * pivot points disabled.
 * Property type: integer (0: false; !0: true). Default value: false.
 */
#define AI_CONFIG_IMPORT_3DS_IGNORE_PIVOT	"imp.3ds.nopivot"


// ---------------------------------------------------------------------------
/** \brief Configures the AC loader to collect all surfaces which have the
 *    "Backface cull" flag set in separate meshes. 
 *
 * Property type: integer (0: false; !0: true). Default value: true.
 */
#define AI_CONFIG_IMPORT_AC_SEPARATE_BFCULL	"imp.ac.sepbfcull"


// ---------------------------------------------------------------------------
/** \brief Specifies the maximum angle that may be between two vertex tangents
 *         that their tangents and bitangents are smoothed.
 *
 * This applies to the CalcTangentSpace-Step. The angle is specified
 * in degrees, so 180 is PI. The default value is
 * 45 degrees. The maximum value is 175.
 * Property type: float. 
 */
#define AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE "pp.ct.max_smoothing"

// ---------------------------------------------------------------------------
/** \brief Specifies the maximum angle that may be between two face normals
 *         at the same vertex position that their are smoothed.
 *
 * This applies to the GenSmoothNormals-Step. The angle is specified
 * in degrees, so 180 is PI. The default value is
 * 175 degrees (all vertex normals are smoothed). The maximum value is 175
 * Property type: float. Warning: seting this option may cause a severe
 * loss of performance. 
 */
#define AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE "pp.gsn.max_smoothing"


// ---------------------------------------------------------------------------
/** \brief Specifies the minimum number of faces a node should have.
 *         This is an input parameter to the OptimizeGraph-Step.
 *
 * Nodes whose referenced meshes have less faces than this value
 * are propably joined with neighbors with identical local matrices.
 * However, it is just a hint to the step.
 * Property type: integer 
 */
#define AI_CONFIG_PP_OG_MIN_NUM_FACES		"pp.og.min_faces"


// ---------------------------------------------------------------------------
/** \brief Specifies whether the OptimizeGraphProcess joins nodes even if
 *         their local transformations are inequal.
 *
 * By default, nodes with different local transformations are never joined.
 * The intention is that all vertices should remain in their original
 * local coordinate space where they are correctly centered and aligned,
 * which does also allow for some significant culling improvements.
 */
#define AI_CONFIG_PP_OG_JOIN_INEQUAL_TRANSFORMS	"pp.og.allow_diffwm"


// ---------------------------------------------------------------------------
/** \brief Sets the colormap (= palette) to be used to decode embedded
 *         textures in MDL (Quake or 3DGS) files.
 *
 * This must be a valid path to a file. The file is 768 (256*3) bytes
 * large and contains RGB triplets for each of the 256 palette entries.
 * The default value is colormap.lmp. If the file is not found,
 * a default palette (from Quake 1) is used. 
 * Property type: string.
 */
#define AI_CONFIG_IMPORT_MDL_COLORMAP		"imp.mdl.color_map"


// ---------------------------------------------------------------------------
/** \brief Enumerates components of the aiScene and aiMesh data structures
 *  that can be excluded from the import with the RemoveComponent step.
 *
 *  See the documentation to #aiProcess_RemoveComment for more details.
 */
enum aiComponent
{
	//! Normal vectors
	aiComponent_NORMALS = 0x2u,

	//! Tangents and bitangents go always together ...
	aiComponent_TANGENTS_AND_BITANGENTS = 0x4u,

	//! ALL color sets
	//! Use aiComponent_COLORn(N) to specifiy the N'th set 
	aiComponent_COLORS = 0x8,

	//! ALL texture UV sets
	//! aiComponent_TEXCOORDn(N) to specifiy the N'th set 
	aiComponent_TEXCOORDS = 0x10,

	//! Removes all bone weights from all meshes.
	//! The scenegraph nodes corresponding to the
	//! bones are removed
	aiComponent_BONEWEIGHTS = 0x20,

	//! Removes all bone animations (aiScene::mAnimations)
	aiComponent_ANIMATIONS = 0x40,

	//! Removes all embedded textures (aiScene::mTextures)
	aiComponent_TEXTURES = 0x80,

	//! Removes all light sources (aiScene::mLights)
	//! The scenegraph nodes corresponding to the
	//! light sources are removed.
	aiComponent_LIGHTS = 0x100,

	//! Removes all light sources (aiScene::mCameras)
	//! The scenegraph nodes corresponding to the
	//! cameras are removed.
	aiComponent_CAMERAS = 0x200,

	//! Removes all meshes (aiScene::mMeshes). 
	aiComponent_MESHES = 0x400,

	//! Removes all materials. One default material will
	//! be generated, so aiScene::mNumMaterials will be 1.
	//! This makes no real sense without the aiComponent_TEXTURES flag.
	aiComponent_MATERIALS = 0x800
};

#define aiComponent_COLORSn(n) (1u << (n+20u))
#define aiComponent_TEXCOORDSn(n) (1u << (n+25u))


// ---------------------------------------------------------------------------
/** \brief Input parameter to the #aiProcess_RemoveComponent step:
 *  Specifies the parts of the data structure to be removed.
 *
 * See the documentation to this step for further details. The property
 * is expected to be an integer, a bitwise combination of the
 * #aiComponent flags defined above in this header. The default
 * value is 0. Important: if no valid mesh is remaining after the
 * step has been executed (e.g you thought it was funny to specify ALL
 * of the flags defined above) the import FAILS. Mainly because there is
 * no data to work on anymore ...
 */
#define AI_CONFIG_PP_RVC_FLAGS				"pp.rvc.flags"



// ---------------------------------------------------------------------------
/** \brief Causes assimp to favour speed against import quality.
 *
 * Enabling this option may result in faster loading, but it needn't.
 * It represents just a hint to loaders and post-processing steps to use
 * faster code paths, if possible. 
 * This property is expected to be an integer, != 0 stands for true.
 * The default value is 0.
 */
#define AI_CONFIG_FAVOUR_SPEED				"imp.speed_flag"

#endif // !! AI_CONFIG_H_INC
