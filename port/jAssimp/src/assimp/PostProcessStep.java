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
 * Enumeration class that defines postprocess steps that can be executed on a model
 * after it has been loaded. All PPSteps are implemented in C++, so their performance
 * is awesome. Most steps are O(n * log(n)).
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class PostProcessStep {

    /**
     * Default vertex split limit for the SplitLargeMeshes process
     */
    public static final int DEFAULT_VERTEX_SPLIT_LIMIT = 1000000;

    /**
     * Default triangle split limit for the SplitLargeMeshes process
     */
    public static final int DEFAULT_TRIANGLE_SPLIT_LIMIT = 1000000;

    private static int s_iVertexSplitLimit = DEFAULT_VERTEX_SPLIT_LIMIT;
    private static int s_iTriangleSplitLimit = DEFAULT_TRIANGLE_SPLIT_LIMIT;

    /**
     * Identifies and joins identical vertex data sets within all imported
     * meshes. After this step is run each mesh does contain only unique
     * vertices anymore, so a vertex is possibly used by multiple faces.
     * You propably always want to use this post processing step.
     */
    public static final PostProcessStep JoinIdenticalVertices =
            new PostProcessStep("JoinIdenticalVertices");


    /**
     * Splits large meshes into submeshes
     * This is quite useful for realtime rendering where the number of
     * vertices is usually limited by the video driver.
     * <p/>
     * The split limits can be set through SetVertexSplitLimit() and
     * SetTriangleSplitLimit(). The default values are
     * <code>DEFAULT_VERTEX_SPLIT_LIMIT</code> and
     * <code>DEFAULT_TRIANGLE_SPLIT_LIMIT</code>
     */
    public static final PostProcessStep SplitLargeMeshes =
            new PostProcessStep("SplitLargeMeshes");


    /**
     * Omits all normals found in the file. This can be used together
     * with either the <code>GenSmoothNormals</code> or the
     * <code>GenFaceNormal</code> step to force the recomputation of the
     * normals.  If none of the two flags is specified, the output mesh
     * won't have normals
     */
    public static final PostProcessStep KillNormals =
            new PostProcessStep("KillNormals");


    /**
     * Generates smooth normals for all vertices in the mesh. This is
     * ignored if normals are already existing. This step may not be used
     * together with <code>GenFaceNormals</code>
     */
    public static final PostProcessStep GenSmoothNormals =
            new PostProcessStep("GenSmoothNormals");


    /**
     * Generates normals for all faces of all meshes. The normals are
     * shared between the three vertices of a face. This is ignored
     * if normals are already existing. This step may not be used together
     * with <code>GenSmoothNormals</code>
     */
    public static final PostProcessStep GenFaceNormals =
            new PostProcessStep("GenFaceNormals");


    /**
     * Calculates the tangents and bitangents for the imported meshes. Does
     * nothing if a mesh does not have normals. You might want this post
     * processing step to be executed if you plan to use tangent space
     * calculations such as normal mapping applied to the meshes.
     */
    public static final PostProcessStep CalcTangentSpace =
            new PostProcessStep("CalcTangentSpace");


    /**
     * Converts all the imported data to a left-handed coordinate space
     * such as the DirectX coordinate system. By default the data is
     * returned in a right-handed coordinate space which for example
     * OpenGL prefers. In this space, +X points to the right, +Y points towards
     * the viewer and and +Z points upwards. In the DirectX coordinate space
     * +X points to the right, +Y points upwards and +Z points away from
     * the viewer.
     */
    public static final PostProcessStep ConvertToLeftHanded =
            new PostProcessStep("ConvertToLeftHanded ");


    /**
     * Removes the node graph and pretransforms all vertices with the local
     * transformation matrices of their nodes. The output scene does still
     * contain nodes, however, there is only a root node with childs, each
     * one referencing only one mesh, each mesh referencing one material.
     * For rendering, you can simply render all meshes in order, you don't
     * need to pay attention to local transformations and the node hierarchy.
     * Animations are removed during this step.
     */
    public static final PostProcessStep PreTransformVertices =
            new PostProcessStep("PreTransformVertices");


    /**
     * Set the vertex split limit for the "SplitLargeMeshes" process
     * If a mesh exceeds this limit it will be splitted
     *
     * @param limit New vertex split limit. Pass 0xffffffff to disable
     *              a vertex split limit. However, splitting by triangles is still active
     *              then.
     * @return Old vertex split limit
     */
    public static synchronized int setVertexSplitLimit(int limit) {
        if (s_iVertexSplitLimit != limit) {
            // send to the JNI bridge ...
            s_iVertexSplitLimit = limit;
            _NativeSetVertexSplitLimit(limit);
        }
        return limit;
    }

    /**
     * Set the triangle split limit for the "SplitLargeMeshes" process
     * If a mesh exceeds this limit it will be splitted
     *
     * @param limit new triangle split limit. Pass 0xffffffff to disable
     *              a triangle split limit. However, splitting by vertices is still active
     *              then.
     * @return Old triangle split limit
     */
    public static synchronized int setTriangleSplitLimit(int limit) {
        if (s_iTriangleSplitLimit != limit) {
            // send to the JNI bridge ...
            s_iTriangleSplitLimit = limit;
            _NativeSetTriangleSplitLimit(limit);
        }
        return limit;
    }

    /**
     * JNI bridge call. For internal use only
     *
     * @param limit New vertex split limit
     */
    private native static void _NativeSetVertexSplitLimit(int limit);

    /**
     * JNI bridge call. For internal use only
     *
     * @param limit New triangle split limit
     */
    private native static void _NativeSetTriangleSplitLimit(int limit);


    private final String myName; // for debug only

    private PostProcessStep(String name) {
        myName = name;
    }

    public String toString() {
        return myName;
    }
}
