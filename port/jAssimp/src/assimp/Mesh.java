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

import java.awt.*;


/**
 * A mesh represents a geometry or model with a single material.
 * <p/>
 * It usually consists of a number of vertices and a series of primitives/faces
 * referencing the vertices. In addition there might be a series of bones, each
 * of them addressing a number of vertices with a certain weight. Vertex data
 * is presented in channels with each channel containing a single per-vertex
 * information such as a set of texture coords or a normal vector.
 * <p/>
 * Note that not all mesh data channels must be there. E.g. most models
 * don't contain vertex colors, so this data channel is often not filled.
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Mesh {

    /**
     * Defines the maximum number of UV(W) channels that are available
     * for a mesh. If a loader finds more channels in a file, some
     * will be skipped
     */
    public static final int MAX_NUMBER_OF_TEXTURECOORDS = 0x4;

    /**
     * Defines the maximum number of vertex color channels that are
     * available for a mesh. If a loader finds more channels in a file,
     * some will be skipped
     */
    public static final int MAX_NUMBER_OF_COLOR_SETS = 0x4;


    /**
     * Contains the vertices loaded from the model
     */
    private float[] m_vVertices = null;

    /**
     * Contains the normal vectors loaded from the model or
     * computed by postprocess steps. Needn't be existing
     */
    private float[] m_vNormals = null;

    /**
     * Contains the tangent vectors computed by Assimp
     * Needn't be existing
     */
    private float[] m_vTangents = null;

    /**
     * Contains the bitangent vectors computed by Assimp
     * Needn't be existing
     */
    private float[] m_vBitangents = null;

    /**
     * Contains the texture coordinate sets that have been loaded
     * Needn't be existing
     */
    private float[][] m_avUVs = new float[MAX_NUMBER_OF_TEXTURECOORDS][];

    /**
     * Specifies how many texture coordinate components are valid
     * in an UV channel. Normally this will be 2, but 3d texture
     * coordinates for cubic or volumetric mapping are also supported.
     */
    private int[] m_aiNumUVComponents = new int[MAX_NUMBER_OF_TEXTURECOORDS];

    /**
     * Contains the vertex color sets that have been loaded
     * Needn't be existing
     */
    private float[][] m_avColors = new float[MAX_NUMBER_OF_COLOR_SETS][];

    /**
     * Contains a list of all faces of the mesh. each face consists of
     * three indices into the vertex buffer.
     */
    private int[] m_vFaces = null;


    /**
     * Bones which are influencing the mesh
     */
    private Bone[] m_vBones = null;

    /**
     * Material index of the mesh
     */
    private int m_iMaterialIndex = 0;


    /**
     * Check whether there are vertex positions in the model
     * <code>getPosition()</code> will assert this.
     *
     * @return true if vertex positions are available. This is
     *         guaranteed to be always true, except for scenes which contain only
     *         animation skeletons.
     */
    public final boolean hasPositions() {
        return null != this.m_vVertices;
    }

    /**
     * Check whether there are normal vectors in the model
     * <code>getNormal()</code> will assert this.
     *
     * @return true if vertex normals are available.
     */
    public final boolean hasNormals() {
        return null != this.m_vNormals;
    }

    /**
     * Check whether there are bones in the model
     * <code>getBone()</code> will assert this.
     *
     * @return true if vertex normals are available.
     */
    public final boolean hasBones() {
        return null != this.m_vBones;
    }

    /**
     * Check whether there are tangents/bitangents in the model
     * <code>getTangent()</code> and <code>GetBitangent()</code> will assert this.
     *
     * @return true if vertex tangents and bitangents are available.
     */
    public final boolean hasTangentsAndBitangents() {
        return null != this.m_vBitangents && null != this.m_vTangents;
    }

    /**
     * Check whether a given UV set is existing the model
     * <code>getUV()</code> will assert this.
     *
     * @param n UV coordinate set index
     * @return true the uv coordinate set is available.
     */
    public final boolean hasUVCoords(int n) {
        return n < this.m_avUVs.length && null != this.m_avUVs[n];
    }

    /**
     * Check whether a given vertex color set is existing the model
     * <code>getColor()</code> will assert this.
     *
     * @param n Vertex color set index
     * @return true the vertex color set is available.
     */
    public final boolean hasVertexColors(int n) {
        return n < this.m_avColors.length && null != this.m_avColors[n];
    }


    /**
     * Get the number of vertices in the model
     *
     * @return Number of vertices in the model. This could be 0 in some
     *         extreme cases although loaders should filter such cases out
     */
    public final int getNumVertices() {
        return m_vVertices.length;
    }


    /**
     * Get the number of faces in the model
     *
     * @return Number of faces in the model. This could be 0 in some
     *         extreme cases although loaders should filter such cases out
     */
    public final int getNumFaces() {
        return m_vFaces.length;
    }

    /**
     * Get the number of bones in the model
     *
     * @return Number of bones in the model.
     */
    public final int getNumBones() {
        return m_vBones.length;
    }

    /**
     * Get the material index of the mesh
     *
     * @return Zero-based material index
     */
    public final int getMaterialIndex() {
        return m_iMaterialIndex;
    }

    /**
     * Get a vertex position in the mesh
     *
     * @param iIndex Zero-based index of the vertex
     * @param afOut  Output array, size must at least be 3
     *               Receives the vertex position components in x,y,z order
     */
    public final void getPosition(int iIndex, float[] afOut) {
        assert(hasPositions() &&
                afOut.length >= 3 &&
                iIndex < this.getNumVertices());

        iIndex *= 3;
        afOut[0] = this.m_vVertices[iIndex];
        afOut[1] = this.m_vVertices[iIndex + 1];
        afOut[2] = this.m_vVertices[iIndex + 2];
    }

    /**
     * Get a vertex position in the mesh
     *
     * @param iIndex   Zero-based index of the vertex
     * @param afOut    Output array, size must at least be 3
     * @param iOutBase Start index in the output array
     *                 Receives the vertex position components in x,y,z order
     */
    public final void getPosition(int iIndex, float[] afOut, int iOutBase) {
        assert(hasPositions() &&
                iOutBase + 3 <= afOut.length &&
                iIndex < this.getNumVertices());

        iIndex *= 3;
        afOut[iOutBase] = this.m_vVertices[iIndex];
        afOut[iOutBase + 1] = this.m_vVertices[iIndex + 1];
        afOut[iOutBase + 2] = this.m_vVertices[iIndex + 2];
    }

    /**
     * Provides direct access to the vertex position array of the mesh
     * This is the recommended way of accessing the data.
     *
     * @return Array of floats, size is numverts * 3. Component ordering
     *         is xyz.
     */
    public final float[] getPositionArray() {
        return this.m_vVertices;
    }

    /**
     * Get a vertex normal in the mesh
     *
     * @param iIndex Zero-based index of the vertex
     * @param afOut  Output array, size must at least be 3
     *               Receives the vertex normal components in x,y,z order
     */
    public final void getNormal(int iIndex, float[] afOut) {
        assert(hasNormals() &&
                afOut.length >= 3 &&
                iIndex < this.getNumVertices());

        iIndex *= 3;
        afOut[0] = this.m_vTangents[iIndex];
        afOut[1] = this.m_vTangents[iIndex + 1];
        afOut[2] = this.m_vTangents[iIndex + 2];
    }

    /**
     * Get a vertex normal in the mesh
     *
     * @param iIndex   Zero-based index of the vertex
     * @param afOut    Output array, size must at least be 3
     * @param iOutBase Start index in the output array
     *                 Receives the vertex normal components in x,y,z order
     */
    public final void getNormal(int iIndex, float[] afOut, int iOutBase) {
        assert(hasNormals() &&
                iOutBase + 3 <= afOut.length &&
                iIndex < this.getNumVertices());

        iIndex *= 3;
        afOut[iOutBase] = this.m_vNormals[iIndex];
        afOut[iOutBase + 1] = this.m_vNormals[iIndex + 1];
        afOut[iOutBase + 2] = this.m_vNormals[iIndex + 2];
    }

    /**
     * Provides direct access to the vertex normal array of the mesh
     * This is the recommended way of accessing the data.
     *
     * @return Array of floats, size is numverts * 3. Component ordering
     *         is xyz.
     */
    public final float[] getNormalArray() {
        return this.m_vNormals;
    }

    /**
     * Get a vertex tangent in the mesh
     *
     * @param iIndex Zero-based index of the vertex
     * @param afOut  Output array, size must at least be 3
     *               Receives the vertex tangent components in x,y,z order
     */
    public final void getTangent(int iIndex, float[] afOut) {
        assert(hasTangentsAndBitangents() &&
                afOut.length >= 3 &&
                iIndex < this.getNumVertices());

        iIndex *= 3;
        afOut[0] = this.m_vTangents[iIndex];
        afOut[1] = this.m_vTangents[iIndex + 1];
        afOut[2] = this.m_vTangents[iIndex + 2];
    }

    /**
     * Get a vertex tangent in the mesh
     *
     * @param iIndex   Zero-based index of the vertex
     * @param afOut    Output array, size must at least be 3
     * @param iOutBase Start index in the output array
     *                 Receives the vertex tangent components in x,y,z order
     */
    public final void getTangent(int iIndex, float[] afOut, int iOutBase) {
        assert(hasTangentsAndBitangents() &&
                iOutBase + 3 <= afOut.length &&
                iIndex < this.getNumVertices());

        iIndex *= 3;
        afOut[iOutBase] = this.m_vTangents[iIndex];
        afOut[iOutBase + 1] = this.m_vTangents[iIndex + 1];
        afOut[iOutBase + 2] = this.m_vTangents[iIndex + 2];
    }

    /**
     * Provides direct access to the vertex tangent array of the mesh
     * This is the recommended way of accessing the data.
     *
     * @return Array of floats, size is numverts * 3. Component ordering
     *         is xyz.
     */
    public final float[] getTangentArray() {
        return this.m_vTangents;
    }

    /**
     * Get a vertex bitangent in the mesh
     *
     * @param iIndex Zero-based index of the vertex
     * @param afOut  Output array, size must at least be 3
     *               Receives the vertex bitangent components in x,y,z order
     */
    public final void getBitangent(int iIndex, float[] afOut) {
        assert(hasTangentsAndBitangents() &&
                afOut.length >= 3 && 3 >= afOut.length &&
                iIndex < this.getNumVertices());

        iIndex *= 3;
        afOut[0] = this.m_vBitangents[iIndex];
        afOut[1] = this.m_vBitangents[iIndex + 1];
        afOut[2] = this.m_vBitangents[iIndex + 2];
    }

    /**
     * Get a vertex bitangent in the mesh
     *
     * @param iIndex   Zero-based index of the vertex
     * @param afOut    Output array, size must at least be 3
     * @param iOutBase Start index in the output array
     *                 Receives the vertex bitangent components in x,y,z order
     */
    public final void getBitangent(int iIndex, float[] afOut, int iOutBase) {
        assert(hasTangentsAndBitangents() &&
                iOutBase + 3 <= afOut.length &&
                iIndex < this.getNumVertices());

        iIndex *= 3;
        afOut[iOutBase] = this.m_vBitangents[iIndex];
        afOut[iOutBase + 1] = this.m_vBitangents[iIndex + 1];
        afOut[iOutBase + 2] = this.m_vBitangents[iIndex + 2];
    }

    /**
     * Provides direct access to the vertex bitangent array of the mesh
     * This is the recommended way of accessing the data.
     *
     * @return Array of floats, size is numverts * 3. Component ordering
     *         is xyz.
     */
    public final float[] getBitangentArray() {
        assert(this.hasTangentsAndBitangents());
        return this.m_vBitangents;
    }


    /**
     * Get a vertex texture coordinate in the mesh
     *
     * @param channel Texture coordinate channel
     * @param iIndex  Zero-based index of the vertex
     * @param afOut   Output array, size must at least be equal to the value
     *                <code>getNumUVComponents</code> returns for <code>channel</code>
     *                Receives the vertex texture coordinate, components are in u,v,w order
     */
    public final void getTexCoord(int channel, int iIndex, float[] afOut) {
        assert(this.hasUVCoords(channel) && afOut.length >= 4 &&
                4 >= afOut.length && iIndex < this.getNumVertices());

        iIndex *= this.m_aiNumUVComponents[channel];
        for (int i = 0; i < this.m_aiNumUVComponents[channel]; ++i) {
            afOut[i] = this.m_avUVs[channel][iIndex + i];
        }
    }

    /**
     * Get a vertex texture coordinate in the mesh
     *
     * @param channel  Texture coordinate channel
     * @param iIndex   Zero-based index of the vertex
     * @param afOut    Output array, size must at least be equal to the value
     *                 <code>getNumUVComponents</code> returns for <code>channel</code>
     *                 Receives the vertex texture coordinate, components are in u,v,w order
     * @param iOutBase Start index in the output array
     */
    public final void getTexCoord(int channel, int iIndex, float[] afOut, int iOutBase) {
        assert(this.hasUVCoords(channel) && afOut.length >= 4 &&
                iOutBase + 4 <= afOut.length && iIndex < this.getNumVertices());

        iIndex *= this.m_aiNumUVComponents[channel];
        for (int i = 0; i < this.m_aiNumUVComponents[channel]; ++i) {
            afOut[i + iOutBase] = this.m_avUVs[channel][iIndex + i];
        }
    }

    /**
     * Provides direct access to a texture coordinate channel of the mesh
     * This is the recommended way of accessing the data.
     *
     * @return Array of floats, size is numverts * <code>getNumUVComponents
     *         (channel)</code>. Component ordering is uvw.
     */
    public final float[] getTexCoordArray(int channel) {
        assert(channel < MAX_NUMBER_OF_TEXTURECOORDS);
        return this.m_avUVs[channel];
    }

    /**
     * Get a vertex color in the mesh
     *
     * @param channel Vertex color channel
     * @param iIndex  Zero-based index of the vertex
     * @param afOut   Output array, size must at least be 4
     *                Receives the vertex color components in r,g,b,a order
     */
    public final void getVertexColor(int channel, int iIndex, float[] afOut) {
        assert(this.hasVertexColors(channel) && afOut.length >= 4 &&
                iIndex < this.getNumVertices());

        iIndex *= 4;   // RGBA order
        afOut[0] = this.m_avColors[channel][iIndex];
        afOut[1] = this.m_avColors[channel][iIndex + 1];
        afOut[2] = this.m_avColors[channel][iIndex + 2];
        afOut[3] = this.m_avColors[channel][iIndex + 3];
    }

    /**
     * Get a vertex color as <code>java.awt.Color</code> in the mesh
     *
     * @param channel Vertex color channel
     * @param iIndex  Zero-based index of the vertex
     * @return Vertex color value packed as <code>java.awt.Color</code>
     */
    public final Color getVertexColor(int channel, int iIndex) {

        float[] afColor = new float[4];
        this.getVertexColor(channel, iIndex, afColor);
        return new Color(afColor[0], afColor[1], afColor[2], afColor[3]);
    }

    /**
     * Get a vertex color in the mesh
     *
     * @param channel  Vertex color channel
     * @param iIndex   Zero-based index of the vertex
     * @param afOut    Output array, size must at least be 4
     *                 Receives the vertex color components in r,g,b,a order
     * @param iOutBase Start index in the output array
     */
    public final void getVertexColor(int channel, int iIndex, float[] afOut, int iOutBase) {
        assert(this.hasVertexColors(channel) && afOut.length >= 4 &&
                iOutBase + 4 <= afOut.length && iIndex < this.getNumVertices());

        iIndex *= 4;   // RGBA order
        afOut[iOutBase] = this.m_avColors[channel][iIndex];
        afOut[iOutBase + 1] = this.m_avColors[channel][iIndex + 1];
        afOut[iOutBase + 2] = this.m_avColors[channel][iIndex + 2];
        afOut[iOutBase + 3] = this.m_avColors[channel][iIndex + 3];
    }

    /**
     * Provides direct access to the vertex bitangent array of the mesh
     * This is the recommended way of accessing the data.
     *
     * @return Array of floats, size is numverts * 3. Component ordering
     *         is xyz.
     */
    public final float[] getVertexColorArray(int channel) {
        assert(channel < MAX_NUMBER_OF_COLOR_SETS);
        return this.m_avColors[channel];
    }


    /**
     * Get a single face of the mesh
     *
     * @param iIndex Index of the face. Must be smaller than the value
     *               returned by <code>getNumFaces()</code>
     * @param aiOut  Output array, size must at least be 3
     */
    public final void getFace(int iIndex, int[] aiOut) {
        assert(aiOut.length >= 3);
        iIndex *= 3;
        aiOut[0] = this.m_vFaces[iIndex];
        aiOut[1] = this.m_vFaces[iIndex + 1];
        aiOut[2] = this.m_vFaces[iIndex + 2];
    }

    /**
     * Get a single face of the mesh
     *
     * @param iIndex   Index of the face. Must be smaller than the value
     *                 returned by <code>getNumFaces()</code>
     * @param aiOut    Output array, size must at least be 3
     * @param iOutBase Start index in the output array
     */
    public final void getFace(int iIndex, int[] aiOut, int iOutBase) {
        assert(aiOut.length >= 3);
        iIndex *= 3;
        aiOut[0] = this.m_vFaces[iIndex];
        aiOut[iOutBase + 1] = this.m_vFaces[iIndex + 1];
        aiOut[iOutBase + 2] = this.m_vFaces[iIndex + 2];
    }


    /**
     * Provides direct access to the face array of the mesh
     * This is the recommended way of accessing the data.
     *
     * @return Array of ints, size is numfaces * 3. Each face consists
     *         of three indices (higher level polygons are automatically
     *         triangulated by the library)
     */
    public final int[] getFaceArray() {
        return this.m_vFaces;
    }


    /**
     * Provides access to the array of all bones influencing this
     * mesh.
     *
     * @return Bone array
     */
    public final Bone[] getBonesArray() {
        assert (null != this.m_vBones);
        return this.m_vBones;
    }


    /**
     * Get a bone influencing the mesh
     *
     * @param i Index of the bone
     * @return Bone
     */
    public final Bone getBone(int i) {
        assert (null != this.m_vBones && i < this.m_vBones.length);
        return this.m_vBones[i];
    }
}
