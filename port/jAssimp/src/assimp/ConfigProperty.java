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
 *
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
     * \note The default value is <code>DEFAULT_SLM_MAX_TRIANGLES</code>
     */
    public static final String CONFIG_PP_SLM_TRIANGLE_LIMIT
            = "pp.slm.triangle_limit";


    /**
     * Set the maximum number of triangles in a mesh.
     * <p/>
     * This is used by the "SplitLargeMeshes" PostProcess-Step to determine
     * whether a mesh must be splitted or not.
     * \note The default value is <code>DEFAULT_SLM_MAX_VERTICES</code>
     */
    public static final String CONFIG_PP_SLM_VERTEX_LIMIT
            = "pp.slm.vertex_limit";


    /**
     * Set the maximum number of bones affecting a single vertex
     * <p/>
     * This is used by the aiProcess_LimitBoneWeights PostProcess-Step.
     * \note The default value is <code>DEFAULT_LBW_MAX_WEIGHTS</code>
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
     */
    public static final String CONFIG_IMPORT_3DS_IGNORE_PIVOT
            = "imp.3ds.nopivot";

    public static final String CONFIG_PP_OG_MAX_DEPTH = "pp.og.max_depth";
    public static final String CONFIG_PP_OG_MIN_TRIS_PER_NODE = "pp.og.min_tris";
    public static final String CONFIG_PP_OG_MAXIMALLY_SMALL = "pp.og.maximally_small";
}
