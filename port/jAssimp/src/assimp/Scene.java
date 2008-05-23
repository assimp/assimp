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

import java.util.Vector;

/**
 * Represents the asset data that has been loaded. A scene consists of
 * multiple meshes, animations, materials and embedded textures.
 * And it defines the scenegraph of the asset (the hierarchy of all
 * meshes, ...).
 * <p/>
 * An instance of this class is returned by Importer.readFile().
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Scene {

    // NOTE: use Vector's to be able to use the constructor for initialisation
    private Vector<Mesh> m_vMeshes;
    private Vector<Texture> m_vTextures;
    private Vector<Material> m_vMaterials;
    private Vector<Animation> m_vAnimations;
    private Node m_rootNode = null;
    private Importer imp = null;

    private Scene() {
    }

    protected Scene(Importer imp) {
        this.imp = imp;
    }

    public final Importer getImporter() {
        return this.imp;
    }

    /**
     * Get the mesh list
     *
     * @return mesh list
     */
    public final Vector<Mesh> getMeshes() {
        return m_vMeshes;
    }

    /**
     * Get the texture list
     *
     * @return Texture list
     */
    public final Vector<Texture> getTextures() {
        return m_vTextures;
    }

    /**
     * Get the material list
     *
     * @return Material list
     */
    public final Vector<Material> getMaterials() {
        return m_vMaterials;
    }

    /**
     * Get the animation list
     *
     * @return Animation list
     */
    public final Vector<Animation> getAnimations() {
        return m_vAnimations;
    }

    /**
     * Get the root node of the scenegraph
     *
     * @return Root node
     */
    public final Node getRootNode() {
        return m_rootNode;
    }

    /**
     * Used to initialize the class instance. Called by Importer. Will maybe
     * be replaced with a RAII solution ...
     *
     * @return true if we're successful
     */
    protected void construct() throws NativeError {

        int i;

        // Mesh, Animation, Texture, Material and Node constructors
        // throw exceptions if they fail

        // load all meshes
        int iTemp = this._NativeGetNumMeshes(imp.hashCode());
        if (0xffffffff == iTemp) throw new NativeError("Unable to obtain number of meshes in the scene");
        this.m_vMeshes.setSize(iTemp);

        for (i = 0; i < iTemp; ++i) {
            this.m_vMeshes.set(i, new Mesh(this, i));
        }

        // load all animations
        iTemp = this._NativeGetNumAnimations(imp.getContext());
        if (0xffffffff == iTemp) throw new NativeError("Unable to obtain number of animations in the scene");
        this.m_vAnimations.setSize(iTemp);

        for (i = 0; i < iTemp; ++i) {
            this.m_vAnimations.set(i, new Animation(this, i));
        }

        // load all textures
        iTemp = this._NativeGetNumTextures(imp.getContext());
        if (0xffffffff == iTemp) throw new NativeError("Unable to obtain number of textures in the scene");
        this.m_vTextures.setSize(iTemp);

        for (i = 0; i < iTemp; ++i) {
            this.m_vTextures.set(i, new Texture(this, i));
        }

        // load all materials
        iTemp = this._NativeGetNumMaterials(imp.getContext());
        if (0xffffffff == iTemp) throw new NativeError("Unable to obtain number of materials in the scene");
        this.m_vMaterials.setSize(iTemp);

        for (i = 0; i < iTemp; ++i) {
            this.m_vMaterials.set(i, new Material(this, i));
        }

        // now load all nodes
        //this.m_rootNode = new Node(this, 0xffffffff);


        return;
    }

    /**
     * JNI bridge function - for internal use only
     * Retrieve the number of meshes in a scene
     *
     * @param context Current importer context (imp.hashCode)
     * @return Number of meshes in the scene that belongs to the context
     */
    private native int _NativeGetNumMeshes(long context);

    /**
     * JNI bridge function - for internal use only
     * Retrieve the number of animations in a scene
     *
     * @param context Current importer context (imp.hashCode)
     * @return Number of animations in the scene that belongs to the context
     */
    private native int _NativeGetNumAnimations(long context);

    /**
     * JNI bridge function - for internal use only
     * Retrieve the number of textures in a scene
     *
     * @param context Current importer context (imp.hashCode)
     * @return Number of textures in the scene that belongs to the context
     */
    private native int _NativeGetNumTextures(long context);

    /**
     * JNI bridge function - for internal use only
     * Retrieve the number of materials in a scene
     *
     * @param context Current importer context (imp.hashCode)
     * @return Number of materials in the scene that belongs to the context
     */
    private native int _NativeGetNumMaterials(long context);
}
