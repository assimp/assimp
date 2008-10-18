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
 * Defines configuration properties.
 * <p/>
 * Static helper class, can't be instanced. It defines configuration
 * property keys to be used with <code> Importer.setPropertyInt</code>
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class ConfigProperty {

    private ConfigProperty() {
    }


    /**
     * Default value for the <code>CONFIG_PP_SLM_TRIANGLE_LIMIT</code>
     * configuration property.
     */
    public static final int DEFAULT_SLM_MAX_TRIANGLES = 1000000;


    /**
     * Default value for the <code>CONFIG_PP_SLM_VERTEX_LIMIT</code>
     * configuration property.
     */
    public static final int DEFAULT_SLM_MAX_VERTICES = 1000000;


    /**
     * Default value for the <code>CONFIG_PP_LBW_MAX_WEIGHTS</code>
     * configuration property.
     */
    public static final int DEFAULT_LBW_MAX_WEIGHTS = 4;


    /**
     * Set the maximum number of vertices in a mesh.
     * <p/>
     * This is used by the "SplitLargeMeshes" PostProcess-Step to determine
     * whether a mesh must be splitted or not.
     * note: The default value is <code>DEFAULT_SLM_MAX_TRIANGLES</code>.
     * The type of the property is int.
     */
    public static final String CONFIG_PP_SLM_TRIANGLE_LIMIT
            = "pp.slm.triangle_limit";


    /**
     * Set the maximum number of triangles in a mesh.
     * <p/>
     * This is used by the "SplitLargeMeshes" PostProcess-Step to determine
     * whether a mesh must be splitted or not.
     * note: The default value is <code>DEFAULT_SLM_MAX_VERTICES</code>.
     * The type of the property is int.
     */
    public static final String CONFIG_PP_SLM_VERTEX_LIMIT
            = "pp.slm.vertex_limit";


    /**
     * Set the maximum number of bones affecting a single vertex
     * <p/>
     * This is used by the aiProcess_LimitBoneWeights PostProcess-Step.
     * note :The default value is <code>DEFAULT_LBW_MAX_WEIGHTS</code>.
     * The type of the property is int.
     */
    public static final String CONFIG_PP_LBW_MAX_WEIGHTS
            = "pp.lbw.weights_limit";


    /**
     * Set the vertex animation keyframe to be imported
     * <p/>
     * ASSIMP does not support vertex keyframes (only bone animation is
     * supported). The library reads only one frame of models with vertex
     * animations. By default this is the first frame.
     * \note The default value is 0. This option applies to all importers.
     * However, it is also possible to override the global setting
     * for a specific loader. You can use the
     * <code>CONFIG_IMPORT_XXX_KEYFRAME</code> options (where XXX is a
     * placeholder for the file format for which you want to override the
     * global setting).
     * The type of the property is int.
     */
    public static final String CONFIG_IMPORT_GLOBAL_KEYFRAME
            = "imp.global.kf";

    public static final String CONFIG_IMPORT_MD3_KEYFRAME = "imp.md3.kf";
    public static final String CONFIG_IMPORT_MD2_KEYFRAME = "imp.md2.kf";
    public static final String CONFIG_IMPORT_MDL_KEYFRAME = "imp.mdl.kf";
    public static final String CONFIG_IMPORT_MDC_KEYFRAME = "imp.mdc.kf";
    public static final String CONFIG_IMPORT_MDR_KEYFRAME = "imp.mdr.kf";
    public static final String CONFIG_IMPORT_SMD_KEYFRAME = "imp.smd.kf";


    /**
     * Causes the 3DS loader to ignore pivot points in the file
     * <p/>
     * There are some faulty 3DS files on the internet which look
     * only correctly with pivot points disabled. By default,
     * this option is disabled.
     * note: This is a boolean property stored as an integer, 0 is false
     */
    public static final String CONFIG_IMPORT_3DS_IGNORE_PIVOT
            = "imp.3ds.nopivot";


    /**
     * Specifies the maximum angle that may be between two vertex tangents
     * that their tangents and bitangents are smoothed.
     * <p/>
     * This applies to the CalcTangentSpace-Step. The angle is specified
     * in degrees, so 180 is PI. The default value is
     * 45 degrees. The maximum value is 180.f
     * The type of the property is float.
     */
    public static final String AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE
            = "pp.ct.max_smoothing";


    /**
     * Specifies the maximum angle that may be between two face normals
     * at the same vertex position that their are smoothed.
     * <p/>
     * This applies to the GenSmoothNormals-Step. The angle is specified
     * in degrees * 1000, so 180.f is PI. The default value is
     * 180 degrees (all vertex normals are smoothed). The maximum value is 180.f
     * The type of the property is float.
     */
    public static final String AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE
            = "pp.gsn.max_smoothing";

 
	/** Input parameter to the #aiProcess_RemoveComponent step:
	 *  Specifies the parts of the data structure to be removed.
	 *
	 * See the documentation to this step for further details. The property
	 * is expected to be an integer, a bitwise combination of the
	 * flags defined in the <code>Component</code> class. The default
	 * value is 0. Important: if no valid mesh is remaining after the
	 * step has been executed (e.g you thought it was funny to specify ALL
	 * of the flags defined above) the import FAILS. Mainly because there is
	 * no data to work on anymore ...
	 */
	public static final String AI_CONFIG_PP_RVC_FLAGS 
		= "pp.rvc.flags";


	/** Causes assimp to favour speed against import quality.
	*
	 * Enabling this option may result in faster loading, but it needn't.
	 * It represents just a hint to loaders and post-processing steps to use
	 * faster code paths, if possible. 
	 * This property is expected to be an integer, != 0 stands for true.
	 * The default value is 0.
	*/
	public static final String AI_CONFIG_FAVOUR_SPEED 
		=	"imp.speed_flag";
};
