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
public class Mesh extends IMappable {

    /**
     * Defines the maximum number of UV(W) channels that are available
     * for a mesh. If a loader finds more channels in a file, some
     * will be skipped
     */
    private static final int MAX_NUMBER_OF_TEXTURECOORDS = 0x4;

    /**
     * Defines the maximum number of vertex color channels that are
     * available for a mesh. If a loader finds more channels in a file,
     * some will be skipped
     */
    private static final int MAX_NUMBER_OF_COLOR_SETS = 0x4;


    /**
     * Specifies which vertex components are existing in
     * the native implementation. If a member is null here,
     * although it is existing, it hasn't yet been mapped
     * into memory
     */
    private int m_iPresentFlags = 0;

    private static final int PF_POSITION = 0x1;
    private static final int PF_NORMAL = 0x2;
    private static final int PF_TANGENTBITANGENT = 0x4;
    private static final int PF_VERTEXCOLOR = 0x1000;
    private static final int PF_UVCOORD = 0x10000;

    private static int PF_VERTEXCOLORn(int n) {
        assert(n <= MAX_NUMBER_OF_COLOR_SETS);
        return PF_VERTEXCOLOR << n;
    }

    private static int PF_UVCOORDn(int n) {
        assert(n <= MAX_NUMBER_OF_TEXTURECOORDS);
        return PF_UVCOORD << n;
    }

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
     * Number of vertices in the mesh
     */
    private int m_iNumVertices;


    /**
     * Construction from a given parent object and array index
     *
     * @param parent Parent object
     * @param index  Valied index in the parent's list
     */
    public Mesh(Object parent, int index) throws NativeError {
        super(parent, index);

        assert (parent instanceof Scene);

        Scene sc = (Scene) parent;
        if (0xffffffff == (this.m_iPresentFlags = this._NativeGetPresenceFlags(
                sc.getImporter().getContext()))) {
            throw new NativeError("Unable to obtain a list of vertex presence flags");
        }
        if (0xffffffff == (this.m_iNumVertices = this._NativeGetNumVertices(
                sc.getImporter().getContext()))) {
            throw new NativeError("Unable to obtain the number of vertices in the mesh");
        }
        if (0xffffffff == this._NativeGetNumUVComponents(
                sc.getImporter().getContext(), this.m_aiNumUVComponents)) {
            throw new NativeError("Unable to obtain the number of UV components");
        }
    }

    /**
     * Check whether there are vertex positions in the model
     * <code>getVertex()</code> will assert this.
     *
     * @return true if vertex positions are available.
     */
    public boolean hasPositions() {
        return 0 != (this.m_iPresentFlags & PF_POSITION);
    }

    /**
     * Check whether there are normal vectors in the model
     * <code>getNormal()</code> will assert this.
     *
     * @return true if vertex normals are available.
     */
    public boolean hasNormals() {
        return 0 != (this.m_iPresentFlags & PF_NORMAL);
    }

    /**
     * Check whether there are tangents/bitangents in the model
     * <code>getTangent()</code> and <code>GetBitangent()</code> will assert this.
     *
     * @return true if vertex tangents and bitangents are available.
     */
    public boolean hasTangentsAndBitangents() {
        return 0 != (this.m_iPresentFlags & PF_TANGENTBITANGENT);
    }

    /**
     * Check whether a given UV set is existing the model
     * <code>getUV()</code> will assert this.
     *
     * @param n UV coordinate set index
     * @return true the uv coordinate set is available.
     */
    public boolean hasUVCoords(int n) {
        return 0 != (this.m_iPresentFlags & PF_UVCOORDn(n));
    }

    /**
     * Check whether a given vertex color set is existing the model
     * <code>getColor()</code> will assert this.
     *
     * @param n Vertex color set index
     * @return true the vertex color set is available.
     */
    public boolean hasVertexColors(int n) {
        return 0 != (this.m_iPresentFlags & PF_VERTEXCOLORn(n));
    }


    /**
     * Get the number of vertices in the model
     *
     * @return Number of vertices in the asset. This could be 0 in some
     *         extreme cases although loaders should filter such cases out
     */
    public int getNumVertices() {
        return m_iNumVertices;
    }

    /**
     * Get a vertex position in the mesh
     *
     * @param iIndex Zero-based index of the vertex
     * @param afOut  Output array, size must at least be 3
     *               Receives the vertex position components in x,y,z order
     */
    public void getPosition(int iIndex, float[] afOut) {
        assert(this.hasPositions());
        assert(afOut.length >= 3);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_vVertices) this.mapVertices();

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
    public void getPosition(int iIndex, float[] afOut, int iOutBase) {
        assert(this.hasPositions());
        assert(afOut.length >= 3);
        assert(iOutBase + 3 <= afOut.length);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_vVertices) this.mapVertices();

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
    public float[] getPositionArray() {
        assert(this.hasPositions());
        if (null == this.m_vVertices) this.mapVertices();
        return this.m_vVertices;
    }

    /**
     * Get a vertex normal in the mesh
     *
     * @param iIndex Zero-based index of the vertex
     * @param afOut  Output array, size must at least be 3
     *               Receives the vertex normal components in x,y,z order
     */
    public void getNormal(int iIndex, float[] afOut) {
        assert(this.hasNormals());
        assert(afOut.length >= 3);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_vNormals) this.mapNormals();

        iIndex *= 3;
        afOut[0] = this.m_vNormals[iIndex];
        afOut[1] = this.m_vNormals[iIndex + 1];
        afOut[2] = this.m_vNormals[iIndex + 2];
    }

    /**
     * Get a vertex normal in the mesh
     *
     * @param iIndex   Zero-based index of the vertex
     * @param afOut    Output array, size must at least be 3
     * @param iOutBase Start index in the output array
     *                 Receives the vertex normal components in x,y,z order
     */
    public void getNormal(int iIndex, float[] afOut, int iOutBase) {
        assert(this.hasNormals());
        assert(afOut.length >= 3);
        assert(iOutBase + 3 <= afOut.length);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_vNormals) this.mapNormals();

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
    public float[] getNormalArray() {
        assert(this.hasNormals());
        if (null == this.m_vNormals) this.mapNormals();
        return this.m_vNormals;
    }

    /**
     * Get a vertex tangent in the mesh
     *
     * @param iIndex Zero-based index of the vertex
     * @param afOut  Output array, size must at least be 3
     *               Receives the vertex tangent components in x,y,z order
     */
    public void getTangent(int iIndex, float[] afOut) {
        assert(this.hasTangentsAndBitangents());
        assert(afOut.length >= 3);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_vTangents) this.mapTangents();

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
    public void getTangent(int iIndex, float[] afOut, int iOutBase) {
        assert(this.hasTangentsAndBitangents());
        assert(afOut.length >= 3);
        assert(iOutBase + 3 <= afOut.length);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_vTangents) this.mapTangents();

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
    public float[] getTangentArray() {
        assert(this.hasTangentsAndBitangents());
        if (null == this.m_vTangents) this.mapTangents();
        return this.m_vTangents;
    }

    /**
     * Get a vertex bitangent in the mesh
     *
     * @param iIndex Zero-based index of the vertex
     * @param afOut  Output array, size must at least be 3
     *               Receives the vertex bitangent components in x,y,z order
     */
    public void getBitangent(int iIndex, float[] afOut) {
        assert(this.hasTangentsAndBitangents());
        assert(afOut.length >= 3);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_vBitangents) this.mapBitangents();

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
    public void getBitangent(int iIndex, float[] afOut, int iOutBase) {
        assert(this.hasTangentsAndBitangents());
        assert(afOut.length >= 3);
        assert(iOutBase + 3 <= afOut.length);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_vBitangents) this.mapBitangents();

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
    public float[] getBitangentArray() {
        assert(this.hasTangentsAndBitangents());
        if (null == this.m_vBitangents) this.mapBitangents();
        return this.m_vBitangents;
    }

    /**
     * Get a vertex texture coordinate in the mesh
     *
     * @param channel Texture coordinate channel
     * @param iIndex  Zero-based index of the vertex
     * @param afOut   Output array, size must at least be equal to the value
     * <code>getNumUVComponents</code> returns for <code>channel</code>
     * Receives the vertex texture coordinate, components are in u,v,w order
     */
    public void getTexCoord(int channel, int iIndex, float[] afOut) {
        assert(this.hasUVCoords(channel));
        assert(afOut.length >= this.m_aiNumUVComponents[channel]);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_avUVs[channel]) this.mapUVs(channel);

        iIndex *= this.m_aiNumUVComponents[channel];
        for (int i = 0; i < this.m_aiNumUVComponents[channel];++i) {
            afOut[i] = this.m_avUVs[channel][iIndex+i];
        }
    }

    /**
     * Get a vertex texture coordinate in the mesh
     *
     * @param channel Texture coordinate channel
     * @param iIndex  Zero-based index of the vertex
     * @param afOut   Output array, size must at least be equal to the value
     * <code>getNumUVComponents</code> returns for <code>channel</code>
     * Receives the vertex texture coordinate, components are in u,v,w order
     * @param iOutBase Start index in the output array
     */
    public void getTexCoord(int channel, int iIndex, float[] afOut, int iOutBase) {
        assert(this.hasUVCoords(channel));
        assert(afOut.length >= this.m_aiNumUVComponents[channel]);
        assert(iOutBase + this.m_aiNumUVComponents[channel] <= afOut.length);
        assert(iIndex < this.getNumVertices()); // explicitly assert here, no AIOOBE

        if (null == this.m_avUVs[channel]) this.mapUVs(channel);

        iIndex *= this.m_aiNumUVComponents[channel];
        for (int i = 0; i < this.m_aiNumUVComponents[channel];++i) {
            afOut[i+iOutBase] = this.m_avUVs[channel][iIndex+i];
        }
    }

    /**
     * Provides direct access to a texture coordinate channel of the mesh
     * This is the recommended way of accessing the data.
     *
     * @return Array of floats, size is numverts * <code>getNumUVComponents
     * (channel)</code>. Component ordering is uvw.
     */
    public float[] getTexCoordArray(int channel) {
        assert(this.hasUVCoords(channel));
        if (null == this.m_avUVs[channel]) this.mapUVs(channel);
        return this.m_avUVs[channel];
    }


    protected void OnMap() throws NativeError {

        // map all vertex component arrays into our memory

    }


    /**
     * JNI bridge function - for internal use only
     * Retrieve a bit combination which indicates which vertex
     * components are existing in the model.
     *
     * @param context Current importer context (imp.hashCode)
     * @return Combination of the PF_XXX constants
     */
    private native int _NativeGetPresenceFlags(long context);

    /**
     * JNI bridge function - for internal use only
     * Retrieve the number of vertices in the mesh
     *
     * @param context Current importer context (imp.hashCode)
     * @return Number of vertices in the mesh
     */
    private native int _NativeGetNumVertices(long context);

    /**
     * JNI bridge function - for internal use only
     * Retrieve the number of uvw components for a channel
     *
     * @param context Current importer context (imp.hashCode)
     * @param out Output array. Size must be MAX_NUMBER_OF_TEXTURECOORDS.
     * @return 0xffffffff if an error occured
     */
     private native int _NativeGetNumUVComponents(long context, int[] out);
}
