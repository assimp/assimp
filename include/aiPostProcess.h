/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/

/** @file aiPostProcess.h
 *  @brief Definitions for import post processing steps
 */
#ifndef AI_POSTPROCESS_H_INC
#define AI_POSTPROCESS_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Defines the flags for all possible post processing steps. */
enum aiPostProcessSteps
{
	/** <hr>Calculates the tangents and bitangents for the imported meshes. Does nothing
	 * if a mesh does not have normals. You might want this post processing step to be
	 * executed if you plan to use tangent space calculations such as normal mapping 
	 * applied to the meshes. There exists a configuration option,
	* #AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE that allows you to specify
	* an angle maximum for the step.
	 */
	aiProcess_CalcTangentSpace = 1,

	/** <hr>Identifies and joins identical vertex data sets within all imported meshes. 
	 * After this step is run each mesh does contain only unique vertices anymore,
	 * so a vertex is possibly used by multiple faces. You usually want
	 * to use this post processing step.*/
	aiProcess_JoinIdenticalVertices = 2,

	/** <hr>Converts all the imported data to a left-handed coordinate space such as 
	 * the DirectX coordinate system. By default the data is returned in a right-handed
	 * coordinate space which for example OpenGL prefers. In this space, +X points to the
	 * right, +Y points towards the viewer and and +Z points upwards. In the DirectX 
     * coordinate space +X points to the right, +Y points upwards and +Z points 
     * away from the viewer.
	 */
	aiProcess_ConvertToLeftHanded = 4,

	/** <hr>Triangulates all faces of all meshes. By default the imported mesh data might 
	 * contain faces with more than 3 indices. For rendering a mesh you usually need
	 * all faces to be triangles. This post processing step splits up all higher faces
	 * to triangles. This step won't modify line and point primitives. If you need
	 * only triangles, do the following:<br>
	 * 1. Specify both the aiProcess_Triangulate and the aiProcess_SortByPType
	 * step. <br>
	 * 2. Ignore all point and line meshes when you process assimp's output data.
	 */
	aiProcess_Triangulate = 8,

	/** <hr>Removes some parts of the data structure (animations, materials, 
	 *  light sources, cameras, textures, vertex components).
	 *
	 *  The  components to be removed are specified in a separate
	 *  configuration option, #AI_CONFIG_PP_RVC_FLAGS. This is quite useful
	 *  if you don't need all parts of the output structure. Especially vertex 
	 *  colors are rarely used today ... . Calling this step to exclude non-required
	 *  stuff from the pipeline as early as possible results in an increased 
	 *  performance and a better optimized output data structure.
	 *  This step is also useful if you want to force Assimp to recompute 
	 *  normals or tangents. The corresponding steps don't recompute them if 
	 *  they're already there ( loaded from the source asset). By using this 
	 *  step you can make sure they are NOT there.
	 */
	aiProcess_RemoveComponent = 0x10,

	/** <hr>Generates normals for all faces of all meshes. The normals are shared
	* between the three vertices of a face. This is ignored
	* if normals are already existing. This flag may not be specified together
	* with aiProcess_GenSmoothNormals.
	*/
	aiProcess_GenNormals = 0x20,

	/** <hr>Generates smooth normals for all vertices in the mesh. This is ignored
	* if normals are already existing. This flag may not be specified together
	* with aiProcess_GenNormals. There's a configuration option,
	* #AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE that allows you to specify
	* an angle maximum for the normal smoothing algorithm. Normals exceeding
	* this limit are not smoothed, resulting in a a 'hard' seam between two faces.
	*/
	aiProcess_GenSmoothNormals = 0x40,

	/** <hr>Splits large meshes into submeshes
	* This is quite useful for realtime rendering where the number of triangles
	* to be maximally rendered in one drawcall is usually limited by the video driver. 
	* The maximum vertex buffer size suffers from limitations, too. Both
	* requirements are met with this step: you can specify both a triangle and vertex
	* limit for a single mesh.
	*
	* The split limits can be set through the <tt>AI_CONFIG_PP_SLM_VERTEX_LIMIT</tt>
	* and <tt>AI_CONFIG_PP_SLM_TRIANGLE_LIMIT</tt> The default values are 
	* <tt>AI_SLM_DEFAULT_MAX_VERTICES</tt> and <tt>AI_SLM_DEFAULT_MAX_TRIANGLES</tt>. 
	*/
	aiProcess_SplitLargeMeshes = 0x80,

	/** <hr>Removes the node graph and pre-transforms all vertices with
	* the local transformation matrices of their nodes. The output
	* scene does still contain nodes, however, there is only a
	* root node with children, each one referencing only one mesh,
	* each mesh referencing one material. For rendering, you can
	* simply render all meshes in order, you don't need to pay
	* attention to local transformations and the node hierarchy.
	* Animations are removed during this step. 
	* This step is intended for applications that have no scenegraph.
	* The step CAN cause some problems: if e.g. a mesh of the asset
	* contains normals and another, using the same material index, does not, 
	* they will be brought together, but the first meshes's part of
	* the normal list will be zeroed.
	*/
	aiProcess_PreTransformVertices = 0x100,

	/** <hr>Limits the number of bones simultaneously affecting a single vertex
	* to a maximum value. If any vertex is affected by more than that number
	* of bones, the least important vertex weights are removed and the remaining
	* vertex weights are renormalized so that the weights still sum up to 1.
	* The default bone weight limit is 4 (defined as AI_LMW_MAX_WEIGHTS in
	* LimitBoneWeightsProcess.h), but you can use the aiSetBoneWeightLimit
	* function to supply your own limit to the post processing step.
	* 
	* If you intend to perform the skinning in hardware, this post processing step
	* might be of interest for you.
	*/
	aiProcess_LimitBoneWeights = 0x200,

	/** <hr>Validates the aiScene data structure before it is returned.
	 * This makes sure that all indices are valid, all animations and
	 * bones are linked correctly, all material are correct and so on ...
	 * This is primarily intended for our internal debugging stuff,
	 * however, it could be of interest for applications like editors
	 * where stability is more important than loading performance.
	*/
	aiProcess_ValidateDataStructure = 0x400,

	/** <hr>Reorders triangles for vertex cache locality and thus better performance.
	 * The step tries to improve the ACMR (average post-transform vertex cache
	 * miss ratio) for all meshes. The step runs in O(n) and is roughly
	 * basing on the algorithm described in this paper:
	 * http://www.cs.princeton.edu/gfx/pubs/Sander_2007_%3ETR/tipsy.pdf
	 */
	aiProcess_ImproveCacheLocality = 0x800,

	/** <hr>Searches for redundant materials and removes them.
	 *
	 * This is especially useful in combination with the PretransformVertices 
	 * and OptimizeGraph steps. Both steps join small meshes, but they
	 * can't do that if two meshes have different materials.
	 */
	aiProcess_RemoveRedundantMaterials = 0x1000,

	/** <hr>This step tries to determine which meshes have normal vectors 
	 * that are facing inwards. The algorithm is simple but effective:
	 * the bounding box of all vertices + their normals is compared against
	 * the volume of the bounding box of all vertices without their normals.
	 * This works well for most objects, problems might occur with planar
	 * surfaces. However, the step tries to filter such cases.
	 * The step inverts all in-facing normals. Generally it is recommended
	 * to enable this step, although the result is not always correct.
	*/
	aiProcess_FixInfacingNormals = 0x2000,


	/** <hr>This step splits meshes with more than one primitive type in 
	 *  homogeneous submeshes. 
	 *
	 *  The step is executed after the triangulation step. After the step
	 *  returns, just one bit is set in aiMesh::mPrimitiveTypes. This is 
	 *  especially useful for real-time rendering where point and line
	 *  primitives are often ignored or rendered separately.
	 *  You can use the AI_CONFIG_PP_SBP_REMOVE option to specify which
	 *  primitive types you need. This can be used to easily exclude
	 *  lines and points, which are rarely used, from the import.
	*/
	aiProcess_SortByPType = 0x8000,

	/** <hr>This step searches all meshes for degenerated primitives and
	 *  converts them to proper lines or points.
	 *
	 * A face is degenerated if one or more of its points are identical.
	 * To have the degenerated stuff not only detected and collapsed but
	 * also removed, try one of the following procedures:
	 * <br><b>1.</b> (if you support lines&points for rendering but don't
	 *    want the degenerates)</br>
	 * <ul>
	 *   <li>Specify the #aiProcess_FindDegenerates flag.
	 *   </li>
	 *   <li>Set the <tt>AI_CONFIG_PP_FD_REMOVE</tt> option to 1. This will 
	 *       cause the step to remove degenerated triangles rom the import
	 *       as soon as they're detected. They won't pass any further
	 *       pipeline steps.
	 *   </li>
	 * </ul>
	 * <br><b>2.</b>(if you don't support lines&points at all ...)</br>
	 * <ul>
	 *   <li>Specify the #aiProcess_FindDegenerates flag.
	 *   </li>
	 *   <li>Specify the #aiProcess_SortByPType flag. This moves line and
	 *     point primitives to separate meshes.
	 *   </li>
	 *   <li>Set the <tt>AI_CONFIG_PP_SBP_REMOVE</tt> option to 
	 *       @code aiPrimitiveType_POINTS | aiPrimitiveType_LINES
	 *       @endcode to cause SortByPType to reject point
	 *       and line meshes from the scene.
	 *   </li>
	 * </ul>
	 * @note Degenerated polygons are not necessarily evil and that's why
	 * they're not removed by default. There are several file formats which 
	 * don't support lines or points ... some exporters bypass the
	 * format specification and write them as degenerated triangle instead.
	*/
	aiProcess_FindDegenerates = 0x10000,

	/** <hr>This step searches all meshes for invalid data, such as zeroed
	 *  normal vectors or invalid UV coords and removes them.
	 *
	 * This is especially useful for normals. If they are invalid, and
	 * the step recognizes this, they will be removed and can later
	 * be computed by one of the other steps.<br>
	 * The step will also remove meshes that are infinitely small.
	*/
	aiProcess_FindInvalidData = 0x20000,

	/** <hr>This step converts non-UV mappings (such as spherical or
	 *  cylindrical) to proper UV mapping channels.
	 *
	 * Most applications will support UV mapping only, so you will
	 * probably want to specify this step in every case.
	*/
	aiProcess_GenUVCoords = 0x40000,

	/** <hr>This step pre-transforms UV coordinates by the UV transformations
	 *  (such as scalings or rotations).
	 *
	 * UV transformations are specified per-texture - see the
	 * AI_MATKEY_UVTRANSFORM key for more information on this topic.
	 * This step finds all textures with transformed input UV
	 * coordinates and generates a new, transformed, UV channel for it.
	 * Most applications won't support UV transformations, so you will
	 * probably want to specify this step in every case.

	 * todo ... rewrite doc
	*/
	aiProcess_TransformUVCoords = 0x80000,


	/** <hr>This step searches for duplicate meshes and replaces duplicates
	 *  with references to the first mesh.
	 *
	 * todo ... add more doc 
	*/
	aiProcess_FindInstances = 0x100000


	// aiProcess_GenEntityMeshes = 0x100000,
	// aiProcess_OptimizeAnimations = 0x200000
	// aiProcess_OptimizeNodes      = 0x400000
};


// ---------------------------------------------------------------------------------------
/** @def aiProcessPreset_TargetRealtimeUse_Fast
 *  @brief Default postprocess configuration optimizing the data for real-time rendering.
 *  
 *  Applications would want to use this preset to load models on end-user PCs,
 *  maybe for direct use in game.
 *
 * If you're using DirectX, don't forget to combine this value with
 * the #aiProcess_ConvertToLeftHanded step. If you don't support UV transformations
 * in your application apply the #aiProcess_TransformUVCoords step, too.
 *  @note Please take the time to read the doc to the steps enabled by this preset. 
 *  Some of them offer further configurable properties, some of them might not be of
 *  use for you so it might be better to not specify them.
 */
#define aiProcessPreset_TargetRealtime_Fast \
	aiProcess_CalcTangentSpace		|  \
	aiProcess_GenNormals			|  \
	aiProcess_JoinIdenticalVertices |  \
	aiProcess_Triangulate			|  \
	aiProcess_GenUVCoords           |  \
	aiProcess_SortByPType           |  \
	0

 // ---------------------------------------------------------------------------------------
 /** @def aiProcessPreset_TargetRealtime_Quality
  *  @brief Default postprocess configuration optimizing the data for real-time rendering.
  *
  *  Unlike #aiProcessPreset_TargetRealtime_Fast, this configuration
  *  performs some extra optimizations to improve rendering speed and
  *  to minimize memory usage. It could be a good choice for a level editor
  *  environment where import speed is not so important.
  *  
  *  If you're using DirectX, don't forget to combine this value with
  *  the #aiProcess_ConvertToLeftHanded step. If you don't support UV transformations
  *  in your application apply the #aiProcess_TransformUVCoords step, too.
  *  @note Please take the time to read the doc to the steps enabled by this preset. 
  *  Some of them offer further configurable properties, some of them might not be of
  *  use for you so it might be better to not specify them.
  */
#define aiProcessPreset_TargetRealtime_Quality \
	aiProcess_CalcTangentSpace				|  \
	aiProcess_GenSmoothNormals				|  \
	aiProcess_JoinIdenticalVertices			|  \
	aiProcess_ImproveCacheLocality			|  \
	aiProcess_LimitBoneWeights				|  \
	aiProcess_RemoveRedundantMaterials      |  \
	aiProcess_SplitLargeMeshes				|  \
	aiProcess_Triangulate					|  \
	aiProcess_GenUVCoords                   |  \
	aiProcess_SortByPType                   |  \
	aiProcess_FindDegenerates               |  \
	aiProcess_FindInvalidData               |  \
	0

 // ---------------------------------------------------------------------------------------
 /** @def aiProcessPreset_TargetRealtime_MaxQuality
  *  @brief Default postprocess configuration optimizing the data for real-time rendering.
  *
  *  This preset enables almost every optimization step to achieve perfectly
  *  optimized data. It's your choice for level editor environments where import speed 
  *  doesn't care.
  *  
  *  If you're using DirectX, don't forget to combine this value with
  *  the #aiProcess_ConvertToLeftHanded step. If you don't support UV transformations
  *  in your application apply the #aiProcess_TransformUVCoords step, too.
  *  @note Please take the time to read the doc to the steps enabled by this preset. 
  *  Some of them offer further configurable properties, some of them might not be of
  *  use for you so it might be better to not specify them.
  */
#define aiProcessPreset_TargetRealtime_MaxQuality \
	aiProcessPreset_TargetRealtime_Quality   |  \
	aiProcess_FindInstances                  |  \
	aiProcess_ValidateDataStructure          |  \
	0


#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // AI_POSTPROCESS_H_INC
