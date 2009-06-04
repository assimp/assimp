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

package assimp;

/**
 * Defines all predefined configuration properties used by Assimp. The user can
 * fine-tune these settings to optimize Assimp for a particular purpose. There
 * are both global settings and settings referring to a post processing step or
 * a particular import plugin.
 * 
 * 
 * @author Alexander Gessler (aramis_acg@users.sourceforge.net)
 * @version 1.0
 * @see Importer#setProperty
 */
public class ConfigProperty {

	private ConfigProperty() {
	}

	/**
	 * Enumerates components of the aiScene and aiMesh data structures that can
	 * be excluded from the import by using the
	 * <code>PostProcessing.RemoveComponent</code> step.
	 * 
	 * See the documentation to <code>PostProcessing.RemoveComponent</code> for
	 * more details.
	 * 
	 * @see PostProcessing#RemoveComponent
	 */
	public static final class Component {
		/**
		 * Normal vectors
		 */
		public static final int NORMALS = 0x2;

		/**
		 * Tangents and bitangents go always together ...
		 */
		public static final int TANGENTS_AND_BITANGENTS = 0x4;

		/**
		 * ALL color sets Use aiComponent_COLORn(N) to specify the N'th set
		 */
		public static final int COLORS = 0x8;

		/**
		 * ALL texture UV sets aiComponent_TEXCOORDn(N) to specify the N'th set
		 */
		public static final int TEXCOORDS = 0x10;

		/**
		 * Removes all bone weights from all meshes. The scene graph nodes
		 * corresponding to the bones are NOT removed. use the
		 * #aiProcess_OptimizeGraph step to do this
		 */
		public static final int BONEWEIGHTS = 0x20;

		/**
		 * Removes all node animations (aiScene::mAnimations). The scene graph
		 * nodes corresponding to the bones are NOT removed. use the
		 * #aiProcess_OptimizeGraph step to do this
		 */
		public static final int ANIMATIONS = 0x40;

		/**
		 * Removes all embedded textures (aiScene::mTextures)
		 */
		public static final int TEXTURES = 0x80;

		/**
		 * Removes all light sources (aiScene::mLights). The scenegraph nodes
		 * corresponding to the bones are NOT removed. use the
		 * #aiProcess_OptimizeGraph step to do this
		 */
		public static final int LIGHTS = 0x100;

		/**
		 * Removes all light sources (aiScene::mCameras). The scenegraph nodes
		 * corresponding to the bones are NOT removed. use the
		 * #aiProcess_OptimizeGraph step to do this
		 */
		public static final int CAMERAS = 0x200;

		/**
		 * Removes all meshes (aiScene::mMeshes).
		 */
		public static final int MESHES = 0x400;

		/**
		 * Removes all materials. One default material will be generated, so
		 * aiScene::mNumMaterials will be 1.
		 */
		public static final int MATERIALS = 0x800;

		// Remove a specific color channel 'n'
		public static final int COLORSn(int n) {
			return (1 << (n + 20));
		}

		// Remove a specific UV channel 'n'
		public static final int TEXCOORDSn(int n) {
			return (1 << (n + 25));
		}
	};

	/**
	 * Dummy base for all predefined configuration keys.
	 */
	public static class Any<Type> {
		public Any(String str) {
			name = str;
		}

		String name;
	}

	/**
	 * Specifies the maximum angle that may be between two vertex tangents that
	 * their tangents and bitangents are smoothed.
	 * 
	 * This applies to the CalcTangentSpace-Step. The angle is specified in
	 * degrees, so 180 is PI. The default value is 45 degrees. The maximum value
	 * is 175. Property type: float.
	 */
	public static final Any<Float> PP_CT_MAX_SMOOTHING_ANGLE = new Any<Float>(
			"PP_CT_MAX_SMOOTHING_ANGLE");

	/**
	 * Specifies the maximum angle that may be between two face normals at the
	 * same vertex position that their are smoothed together.
	 * 
	 * Sometimes referred to as 'crease angle'. This applies to the
	 * GenSmoothNormals-Step. The angle is specified in degrees, so 180 is PI.
	 * The default value is 175 degrees (all vertex normals are smoothed). The
	 * maximum value is 175, too. Property type: float. Warning: setting this
	 * option may cause a severe loss of performance. The performance is
	 * unaffected if the #AI_CONFIG_FAVOUR_SPEED flag is set but the output
	 * quality may be reduced.
	 */
	public static final Any<Float> PP_GSN_MAX_SMOOTHING_ANGLE = new Any<Float>(
			"PP_GSN_MAX_SMOOTHING_ANGLE");

	/**
	 * Configures the #aiProcess_RemoveRedundantMaterials step to keep materials
	 * matching a name in a given list.
	 * 
	 * This is a list of 1 to n strings, ' ' serves as delimiter character.
	 * Identifiers containing whitespaces must be enclosed in *single* quotation
	 * marks. For example:<tt>
	 * "keep-me and_me_to anotherMaterialToBeKept = new Any<Float>('name with whitespace= new Any<Float>('"</tt>
	 * . If a material matches on of these names, it will not be modified or
	 * removed by the postprocessing step nor will other materials be replaced
	 * by a reference to it. <br>
	 * This option might be useful if you are using some magic material names to
	 * pass additional semantics through the content pipeline. This ensures they
	 * won't be optimized away, but a general optimization is still performed
	 * for materials not contained in the list. Property type: String. Default
	 * value: n/a
	 * 
	 * @note Linefeeds, tabs or carriage returns are treated as whitespace.
	 *       Material names are case sensitive.
	 */
	public static final Any<Float> PP_RRM_EXCLUDE_LIST = new Any<Float>(
			"PP_RRM_EXCLUDE_LIST");

	/**
	 * Configures the #aiProcess_PretransformVertices step to keep the scene
	 * hierarchy. Meshes are moved to worldspace, but no optimization is
	 * performed (means: meshes are not joined. The total number of meshes won't
	 * change).
	 * 
	 * This option could be of use for you if the scene hierarchy contains
	 * important additional information which you want to interpret. For
	 * rendering, you can still render all meshes in the scene without any
	 * transformations. Property type: integer (0: false; !0: true). Default
	 * value: false.
	 */
	public static final Any<Float> PP_PTV_KEEP_HIERARCHY = new Any<Float>(
			"PP_PTV_KEEP_HIERARCHY");

	/**
	 * Configures the #aiProcess_FindDegenerates step to remove degenerated
	 * primitives from the import - immediately.
	 * 
	 * The default behaviour converts degenerated triangles to lines and
	 * degenerated lines to points. See the documentation to the
	 * #aiProcess_FindDegenerates step for a detailed example of the various
	 * ways to get rid of these lines and points if you don't want them.
	 * Property type: integer (0: false; !0: true). Default value: false.
	 */
	public static final Any<Float> PP_FD_REMOVE = new Any<Float>("PP_FD_REMOVE");

	/**
	 * Configures the #aiProcess_OptimizeGraph step to preserve nodes matching a
	 * name in a given list.
	 * 
	 * This is a list of 1 to n strings, ' ' serves as delimiter character.
	 * Identifiers containing whitespaces must be enclosed in *single* quotation
	 * marks. For example:<tt>
	 * "keep-me and_me_to anotherNodeToBeKept = new Any<Float>('name with whitespace= new Any<Float>('"</tt>
	 * . If a node matches on of these names, it will not be modified or removed
	 * by the postprocessing step.<br>
	 * This option might be useful if you are using some magic node names to
	 * pass additional semantics through the content pipeline. This ensures they
	 * won't be optimized away, but a general optimization is still performed
	 * for nodes not contained in the list. Property type: String. Default
	 * value: n/a
	 * 
	 * @note Linefeeds, tabs or carriage returns are treated as whitespace. Node
	 *       names are case sensitive.
	 */
	public static final Any<Float> PP_OG_EXCLUDE_LIST = new Any<Float>(
			"PP_OG_EXCLUDE_LIST");

	/**
	 * Set the maximum number of vertices in a mesh.
	 * 
	 * This is used by the "SplitLargeMeshes" PostProcess-Step to determine
	 * whether a mesh must be split or not.
	 * 
	 * @note The default value is AI_SLM_DEFAULT_MAX_VERTICES Property type:
	 *       integer.
	 */
	public static final Any<Float> PP_SLM_TRIANGLE_LIMIT = new Any<Float>(
			"PP_SLM_TRIANGLE_LIMIT");

	/**
	 * Set the maximum number of triangles in a mesh.
	 * 
	 * This is used by the "SplitLargeMeshes" PostProcess-Step to determine
	 * whether a mesh must be split or not.
	 * 
	 * @note The default value is AI_SLM_DEFAULT_MAX_TRIANGLES Property type:
	 *       integer.
	 */
	public static final Any<Float> PP_SLM_VERTEX_LIMIT = new Any<Float>(
			"PP_SLM_VERTEX_LIMIT");

	/**
	 * Set the maximum number of bones affecting a single vertex
	 * 
	 * This is used by the #aiProcess_LimitBoneWeights PostProcess-Step.
	 * 
	 * @note The default value is AI_LBW_MAX_WEIGHTS Property type: integer.
	 */
	public static final Any<Float> PP_LBW_MAX_WEIGHTS = new Any<Float>(
			"PP_LBW_MAX_WEIGHTS");

	/**
	 * Set the size of the post-transform vertex cache to optimize the vertices
	 * for. This configures the #aiProcess_ImproveCacheLocality step.
	 * 
	 * The size is given in vertices. Of course you can't know how the vertex
	 * format will exactly look like after the import returns, but you can still
	 * guess what your meshes will probably have.
	 * 
	 * @note The default value is #PP_ICL_PTCACHE_SIZE. That results in slight
	 *       performance improvements for most nVidia/AMD cards since 2002.
	 *       Property type: integer.
	 */
	public static final Any<Float> PP_ICL_PTCACHE_SIZE = new Any<Float>(
			"PP_ICL_PTCACHE_SIZE");

	/**
	 * Input parameter to the #aiProcess_RemoveComponent step: Specifies the
	 * parts of the data structure to be removed.
	 * 
	 * See the documentation to this step for further details. The property is
	 * expected to be an integer, a bitwise combination of the #aiComponent
	 * flags defined above in this header. The default value is 0. Important: if
	 * no valid mesh is remaining after the step has been executed (e.g you
	 * thought it was funny to specify ALL of the flags defined above) the
	 * import FAILS. Mainly because there is no data to work on anymore ...
	 */
	public static final Any<Float> PP_RVC_FLAGS = new Any<Float>("PP_RVC_FLAGS");

	/**
	 * Input parameter to the #aiProcess_SortByPType step: Specifies which
	 * primitive types are removed by the step.
	 * 
	 * This is a bitwise combination of the aiPrimitiveType flags. Specifying
	 * all of them is illegal, of course. A typical use would be to exclude all
	 * line and point meshes from the import. This is an integer property, its
	 * default value is 0.
	 */
	public static final Any<Float> PP_SBP_REMOVE = new Any<Float>(
			"PP_SBP_REMOVE");

	/**
	 * A hint to Assimp to favor speed against import quality.
	 * 
	 * Enabling this option may result in faster loading, but it needn't. It
	 * represents just a hint to loaders and post-processing steps to use faster
	 * code paths, if possible. This property is expected to be an integer, != 0
	 * stands for true. The default value is 0.
	 */
	public static final Any<Float> FAVOUR_SPEED = new Any<Float>("FAVOUR_SPEED");

	/**
	 * Set the vertex animation keyframe to be imported
	 * 
	 * ASSIMP does not support vertex keyframes (only bone animation is
	 * supported). The library reads only one frame of models with vertex
	 * animations. By default this is the first frame. = new Any<Float>(note The
	 * default value is 0. This option applies to all importers. However, it is
	 * also possible to override the global setting for a specific loader. You
	 * can use the IMPORT_XXX_KEYFRAME options (where XXX is a placeholder for
	 * the file format for which you want to override the global setting).
	 * Property type: integer.
	 */
	public static final Any<Integer> IMPORT_GLOBAL_KEYFRAME = new Any<Integer>(
			"IMPORT_GLOBAL_KEYFRAME");

	public static final Any<Integer> IMPORT_MD3_KEYFRAME = new Any<Integer>(
			"IMPORT_MD3_KEYFRAME");
	public static final Any<Integer> IMPORT_MD2_KEYFRAME = new Any<Integer>(
			"IMPORT_MD2_KEYFRAME");
	public static final Any<Integer> IMPORT_MDL_KEYFRAME = new Any<Integer>(
			"IMPORT_MDL_KEYFRAME");
	public static final Any<Integer> IMPORT_MDC_KEYFRAME = new Any<Integer>(
			"IMPORT_MDC_KEYFRAME");
	public static final Any<Integer> IMPORT_SMD_KEYFRAME = new Any<Integer>(
			"IMPORT_SMD_KEYFRAME");
	public static final Any<Integer> IMPORT_UNREAL_KEYFRAME = new Any<Integer>(
			"IMPORT_UNREAL_KEYFRAME");

	/**
	 * Configures the AC loader to collect all surfaces which have the
	 * "Backface cull" flag set in separate meshes.
	 * 
	 * Property type: integer (0: false; !0: true). Default value: true.
	 */
	public static final Any<Float> IMPORT_AC_SEPARATE_BFCULL = new Any<Float>(
			"IMPORT_AC_SEPARATE_BFCULL");

	/**
	 * Configures the UNREAL 3D loader to separate faces with different surface
	 * flags (e.g. two-sided vs. single-sided).
	 * 
	 * Property type: integer (0: false; !0: true). Default value: true.
	 */
	public static final Any<Float> IMPORT_UNREAL_HANDLE_FLAGS = new Any<Float>(
			"UNREAL_HANDLE_FLAGS");

	/**
	 * Configures the terragen import plugin to compute uv's for terrains, if
	 * not given. Furthermore a default texture is assigned.
	 * 
	 * UV coordinates for terrains are so simple to compute that you'll usually
	 * want to compute them on your own, if you need them. This option is
	 * intended for model viewers which want to offer an easy way to apply
	 * textures to terrains. Property type: integer (0: false; !0: true).
	 * Default value: false.
	 */
	public static final Any<Float> IMPORT_TER_MAKE_UVS = new Any<Float>(
			"IMPORT_TER_MAKE_UVS");

	/**
	 * Configures the ASE loader to always reconstruct normal vectors basing on
	 * the smoothing groups loaded from the file.
	 * 
	 * Many ASE files have invalid normals (they're not orthonormal). Property
	 * type: integer (0: false; !0: true). Default value: true.
	 */
	public static final Any<Float> IMPORT_ASE_RECONSTRUCT_NORMALS = new Any<Float>(
			"IMPORT_ASE_RECONSTRUCT_NORMALS");

	/**
	 * Configures the M3D loader to process multi-part player models.
	 * 
	 * These models usually consist of 3 files, lower.md3, upper.md3 and
	 * head.md3. If this property is set to true, Assimp will try to load and
	 * combine all three files if one of them is loaded. Property type: integer
	 * (0: false; !0: true). Default value: true.
	 */
	public static final Any<Float> IMPORT_MD3_HANDLE_MULTIPART = new Any<Float>(
			"IMPORT_MD3_HANDLE_MULTIPART");

	/**
	 * Tells the MD3 loader which skin files to load.
	 * 
	 * When loading MD3 files, Assimp checks whether a file
	 * <md3_file_name>_<skin_name>.skin is existing. These files are used by
	 * Quake III to be able to assign different skins (e.g. red and blue team)
	 * to models. 'default', 'red', 'blue' are typical skin names. Property
	 * type: String. Default value: "default".
	 */
	public static final Any<Float> IMPORT_MD3_SKIN_NAME = new Any<Float>(
			"IMPORT_MD3_SKIN_NAME");

	/**
	 * Specify the Quake 3 shader file to be used for a particular MD3 file.
	 * This can also be a search path.
	 * 
	 * By default Assimp's behaviour is as follows: If a MD3 file
	 * <tt><any_path>/models/<any_q3_subdir>/<model_name>/<file_name>.md3</tt>
	 * is loaded, the library tries to locate the corresponding shader file in
	 * <tt><any_path>/scripts/<model_name>.shader</tt>. This property overrides
	 * this behaviour. It can either specify a full path to the shader to be
	 * loaded or alternatively the path (relative or absolute) to the directory
	 * where the shaders for all MD3s to be loaded reside. Assimp attempts to
	 * open <tt><dir>/<model_name>.shader</tt> first,
	 * <tt><dir>/<file_name>.shader</tt> is the fallback file. Note that <dir>
	 * should have a terminal (back)slash. Property type: String. Default value:
	 * n/a.
	 */
	public static final Any<Float> IMPORT_MD3_SHADER_SRC = new Any<Float>(
			"IMPORT_MD3_SHADER_SRC");

	/**
	 * Configures the LWO loader to load just a single layer from the file.
	 * 
	 * LWO files consist of layers which are completely separate from each
	 * other. Each layer has a zero-based index. If this property is set, only
	 * layers with that particular index are loaded.
	 */
	public static final Any<Integer> IMPORT_LWO_LAYER_INDEX = new Any<Integer>(
			"IMPORT_LWO_ONE_LAYER_ONLY");

	/**
	 * Configures the MD5 loader to not load the MD5ANIM file for a MD5MESH file
	 * automatically.
	 * 
	 * The default strategy is to look for a file with the same name but the
	 * MD5ANIM extension in the same directory. If it is found, it is loaded and
	 * combined with the MD5MESH file. This configuration option can be used to
	 * disable this behaviour. This is a boolean property, != 0 is
	 * <code>true</code>.
	 */
	public static final Any<Float> IMPORT_MD5_NO_ANIM_AUTOLOAD = new Any<Float>(
			"IMPORT_MD5_NO_ANIM_AUTOLOAD");

	/**
	 * Defines the begin/end of the animation time range for the LWS loader.
	 * 
	 * Assimp provides full conversion of LightWave's envelope system, including
	 * proper handling of LightWave's pre and post conditions. The loader
	 * computes linearly subsampled animation chanels with the frame rate given
	 * in the LWS file. This property defines the start time. Note: animation
	 * channels are only generated if a node has at least one envelope with more
	 * tan one key assigned. This property is given in frames, '0' is the first
	 * frame. By default the importer takes the animation start from the input
	 * LWS file ('FirstFrame' line)<br>
	 * 
	 * @see IMPORT_LWS_ANIM_END - end of the imported time range
	 */
	public static final Any<Float> IMPORT_LWS_ANIM_START = new Any<Float>(
			"IMPORT_LWS_ANIM_START");
	public static final Any<Float> IMPORT_LWS_ANIM_END = new Any<Float>(
			"IMPORT_LWS_ANIM_END");

	/**
	 * Defines the output frame rate of the IRR loader.
	 * 
	 * IRR animations are difficult to convert for Assimp and there will always
	 * be a loss of quality. This setting defines how many keys per second are
	 * returned by the converter.
	 */
	public static final Any<Integer> IMPORT_IRR_ANIM_FPS = new Any<Integer>(
			"IMPORT_IRR_ANIM_FPS");

	/**
	 * Sets the colormap (= palette) to be used to decode embedded textures in
	 * MDL (Quake or 3DGS) files.
	 * 
	 * This must be a valid path to a file. The file is 768 (256*3) bytes large
	 * and contains RGB triplets for each of the 256 palette entries. The
	 * default value is 'colormap.lmp'. If the file is not found, a default
	 * palette (suitable for Quake 1) is used.
	 */
	public static final Any<String> IMPORT_MDL_COLORMAP = new Any<String>(
			"IMPORT_MDL_COLORMAP");
};
