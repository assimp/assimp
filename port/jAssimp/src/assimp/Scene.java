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

    private Mesh[] m_vMeshes = null;
    private Texture[] m_vTextures = null;
    private Material[] m_vMaterials = null;
    private Animation[] m_vAnimations = null;
    private Node m_rootNode = null;
    private Importer imp = null;
    private int flags = 0;

    private Scene() {
    }

    protected Scene(Importer imp) {
        this.imp = imp;
    }

    public final Importer getImporter() {
        return this.imp;
    }


    /**
     * Specifies that no full model but only an animation skeleton has been
     * imported. There are no materials in this case. There are no
     * textures in this case. But there is a node graph, animation channels
     * and propably meshes with bones.
     */
    public static final int FLAG_ANIM_SKELETON_ONLY	= 0x1;


    /**
     * Get the scene flags. This can be any combination of the
     * FLAG_XXX constants defined int this scope
     * @return Scene flags.
     */
    public final int getFlags() {
        return flags;
    }

    /**
     * Get the mesh list
     *
     * @return mesh list
     */
    public final Mesh[] getMeshes() {
        return m_vMeshes;
    }

    /**
     * Get the number of meshes in the scene
     * @return this value can be 0 if the <code>ANIMATION_SKELETON_ONLY</code>
     * flag is set.
     */
    public final int getNumMeshes() {
        return m_vMeshes.length;
    }

    /**
     * Get a mesh from the scene
     * @param i Index of the mesh
     * @return scene.mesh[i]
     */
    public final Mesh getMesh(int i) {
        assert(i < m_vMeshes.length);
        return m_vMeshes[i];
    }

    /**
     * Get the texture list
     *
     * @return Texture list
     */
    public final Texture[] getTextures() {
        return m_vTextures;
    }

     /**
     * Get the number of textures in the scene
     * @return this value is often 0 since most texture's arent' embedded
     */
    public int getNumTextures() {
        return m_vTextures.length;
    }

    /**
     * Get an embedded texture from the scene
     * @param i Index of the mesh
     * @return scene.texture[i]
     */
    public final Texture getTexture(int i) {
        assert(i < m_vTextures.length);
        return m_vTextures[i];
    }

    /**
     * Get the material list
     *
     * @return Material list
     */
    public final Material[] getMaterials() {
        return m_vMaterials;
    }

     /**
     * Get the number of animations in the scene
     * @return this value could be 0, most models have no animations
     */
    public int getNumAnimations() {
        return m_vAnimations.length;
    }

    /**
     * Get the animation list
     *
     * @return Animation list
     */
    public final Animation[] getAnimations() {
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
}
