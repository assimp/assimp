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

/** @file Definitions for import post processing steps */
#ifndef AI_POSTPROCESS_H_INC
#define AI_POSTPROCESS_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Defines the flags for all possible post processing steps. */
enum aiPostProcessSteps
{
	/** Calculates the tangents and bitangents for the imported meshes. Does nothing
	 * if a mesh does not have normals. You might want this post processing step to be
	 * executed if you plan to use tangent space calculations such as normal mapping 
	 * applied to the meshes. There exists a configuration option,
	* #AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE that allows you to specify
	* an angle maximum for the step.
	 */
	aiProcess_CalcTangentSpace = 1,

	/** Identifies and joins identical vertex data sets within all imported meshes. 
	 * After this step is run each mesh does contain only unique vertices anymore,
	 * so a vertex is possibly used by multiple faces. You propably always want
	 * to use this post processing step.*/
	aiProcess_JoinIdenticalVertices = 2,

	/** Converts all the imported data to a left-handed coordinate space such as 
	 * the DirectX coordinate system. By default the data is returned in a right-handed
	 * coordinate space which for example OpenGL prefers. In this space, +X points to the
	 * right, +Y points towards the viewer and and +Z points upwards. In the DirectX 
     * coordinate space +X points to the right, +Y points upwards and +Z points 
     * away from the viewer.
	 */
	aiProcess_ConvertToLeftHanded = 4,

	/** Triangulates all faces of all meshes. By default the imported mesh data might 
	 * contain faces with more than 3 indices. For rendering a mesh you usually need
	 * all faces to be triangles. This post processing step splits up all higher faces
	 * to triangles. This step won't modify line and point primitives. If you need
	 * only triangles, do the following:<br>
	 * 1. Specify both the aiProcess_Triangulate and the aiProcess_SortByPType
	 * step. <br>
	 * 2. Ignore all point and line meshes when you process assimp's output data.
	 */
	aiProcess_Triangulate = 8,

	/** Removes some parts of the data structure (animations, materials, 
	 *  light sources, cameras, textures, vertex components).
	 *
	 *  The  components to be removed are specified in a separate
	 *  configuration option, #AI_CONFIG_PP_RVC_FLAGS. This is quite useful
	 *  if you don't need all parts of the output structure. Especially vertex 
	 *  colors are rarely used today ... . Calling this step to exclude unrequired
	 *  stuff from the pipeline as early as possible results in an increased 
	 *  performance and a better optimized output data structure.
	 *  This step is also useful if you want to force Assimp to recompute 
	 *  normals or tangents. The corresponding steps don't recompute them if 
	 *  they're already there ( loaded from the source asset). By using this 
	 *  step you can make sure they are NOT there.
	 */
	aiProcess_RemoveComponent = 0x10,

	/** Generates normals for all faces of all meshes. The normals are shared
	* between the three vertices of a face. This is ignored
	* if normals are already existing. This flag may not be specified together
	* with aiProcess_GenSmoothNormals
	*/
	aiProcess_GenNormals = 0x20,

	/** Generates smooth normals for all vertices in the mesh. This is ignored
	* if normals are already existing. This flag may not be specified together
	* with aiProcess_GenNormals. There exists a configuration option,
	* #AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE that allows you to specify
	* an angle maximum for the step.
	*/
	aiProcess_GenSmoothNormals = 0x40,

	/** Splits large meshes into submeshes
	* This is quite useful for realtime rendering where the number of vertices
	* is usually limited by the video driver.
	*
	* The split limits can be set through aiSetVertexSplitLimit() and
	* aiSetTriangleSplitLimit(). The default values for this are defined
	* in the internal SplitLargeMeshes.h header as AI_SLM_DEFAULT_MAX_VERTICES
	* and AI_SLM_DEFAULT_MAX_TRIANGLES. 
	*/
	aiProcess_SplitLargeMeshes = 0x80,

	/** Removes the node graph and pretransforms all vertices with
	* the local transformation matrices of their nodes. The output
	* scene does still contain nodes, however, there is only a
	* root node with childs, each one referencing only one mesh,
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

	/** Limits the number of bones simultaneously affecting a single vertex
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

	/** Validates the aiScene data structure before it is returned.
	 * This makes sure that all indices are valid, all animations and
	 * bones are linked correctly, all material are correct and so on ...
	 * This is primarily intended for our internal debugging stuff,
	 * however, it could be of interest for applications like editors
	 * where stability is more important than loading performance.
	*/
	aiProcess_ValidateDataStructure = 0x400,

	/** Reorders triangles for vertex cache locality and thus better performance.
	 * The step tries to improve the ACMR (average post-transform vertex cache
	 * miss ratio) for all meshes. The step runs in O(n) and is roughly
	 * basing on the algorithm described in this paper:
	 * http://www.cs.princeton.edu/gfx/pubs/Sander_2007_%3ETR/tipsy.pdf
	 */
	aiProcess_ImproveCacheLocality = 0x800,

	/** Searches for redundant materials and removes them.
	 *
	 * This is especially useful in combination with the PretransformVertices 
	 * and OptimizeGraph steps. Both steps join small meshes, but they
	 * can't do that if two meshes have different materials.
	 */
	aiProcess_RemoveRedundantMaterials = 0x1000,

	/** This step tries to determine which meshes have normal vectors 
	 * that are facing inwards. The algorithm is simple but effective:
	 * the bounding box of all vertices + their normals is compared against
	 * the volume of the bounding box of all vertices without their normals.
	 * This works well for most objects, problems might occur with planar
	 * surfaces. However, the step tries to filter such cases.
	 * The step inverts all infacing normals. Generally it is recommended
	 * to enable this step, although the result is not always correct.
	*/
	aiProcess_FixInfacingNormals = 0x2000,

	/** This step performs some optimizations on the node graph.
	 * 
	 * It is incompatible to the PreTransformVertices-Step. Some configuration
	 * options exist, see aiConfig.h for more details. 
	 * Generally, two actions are available:<br>
	 *   1. Remove animation nodes and data from the scene. This allows other
	 *      steps for further optimizations.<br>
	 *   2. Combine very small meshes to larger ones. Only if the meshes
	 *      are used by the same node or by nodes on the same hierarchy (with
	 *      equal local transformations). Unlike PreTransformVertices, the
	 *      OptimizeGraph-step doesn't transform vertices from one space 
	 *      another (at least with the default configuration).<br>
	 *
	 *  It is recommended to have this step run with the default configuration.
	 */
	aiProcess_OptimizeGraph = 0x4000,

	/** This step splits meshes with more than one primitive type in 
	 *  homogenous submeshes. 
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

	/** This step searches all meshes for degenerated primitives and
	 *  converts them to proper lines or points.
	 *
	 * A face is degenerated if one or more of its faces are identical.
	*/
	aiProcess_FindDegenerates = 0x10000,

	/** This step searches all meshes for invalid data, such as zeroed
	 *  normal vectors or invalid UV coords and removes them.
	 *
	 * This is especially useful for normals. If they are invalid, and
	 * the step recognizes this, they will be removed and can later
	 * be computed by one of the other steps.<br>
	 * The step will also remove meshes that are infinitely small.
	*/
	aiProcess_FindInvalidData = 0x20000,

	/** This step converts non-UV mappings (such as spherical or
	 *  cylindrical) to proper UV mapping channels.
	 *
	 * Most applications will support UV mapping only, so you will
	 * propably want to specify this step in every case.
	*/
	aiProcess_GenUVCoords = 0x40000,

	/** This step pretransforms UV coordinates by the UV transformations
	 *  (such as scalings or rotations).
	 *
	 * UV transformations are specified per-texture - see the
	 * AI_MATKEY_UVTRANSFORM key for more information on this topic.
	 * This step finds all textures with transformed input UV
	 * coordinates and generates a new, transformed, UV channel for it.
	 * Most applications won't support UV transformations, so you will
	 * propably want to specify this step in every case.
	*/
	aiProcess_TransformUVCoords = 0x80000,
};


/** @def AI_POSTPROCESS_DEFAULT_REALTIME_FASTEST
 *  @brief Default postprocess configuration targeted at realtime applications
 *    which need to load models as fast as possible.
 *  
 *  If you're using DirectX, don't forget to combine this value with
 * the #aiProcess_ConvertToLeftHanded step.
 */
#define AI_POSTPROCESS_DEFAULT_REALTIME_FASTEST \
	aiProcess_CalcTangentSpace		|  \
	aiProcess_GenNormals			|  \
	aiProcess_JoinIdenticalVertices |  \
	aiProcess_Triangulate			|  \
	aiProcess_GenUVCoords


 /** @def AI_POSTPROCESS_DEFAULT_REALTIME
 *   @brief Default postprocess configuration targeted at realtime applications.
 *    Unlike AI_POSTPROCESS_DEFAULT_REALTIME_FASTEST, this configuration
 *    performs some extra optimizations.
 *  
 *  If you're using DirectX, don't forget to combine this value with
 * the #aiProcess_ConvertToLeftHanded step.
 */
#define AI_POSTPROCESS_DEFAULT_REALTIME \
	aiProcess_CalcTangentSpace				|  \
	aiProcess_GenSmoothNormals				|  \
	aiProcess_JoinIdenticalVertices			|  \
	aiProcess_ImproveCacheLocality			|  \
	aiProcess_LimitBoneWeights				|  \
	aiProcess_RemoveRedundantMaterials      |  \
	aiProcess_SplitLargeMeshes				|  \
	aiProcess_OptimizeGraph					|  \
	aiProcess_Triangulate					|  \
	aiProcess_GenUVCoords


#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // AI_POSTPROCESS_H_INC
