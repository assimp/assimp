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
 * A node in the imported hierarchy.
 * <p/>
 * Each node has name, a parent node (except for the root node),
 * a transformation relative to its parent and possibly several child nodes.
 * Simple file formats don't support hierarchical structures, for these formats
 * the imported scene does consist of only a single root node with no childs.
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class Node {


    /**
     * List of all meshes of this node.
     * The array contains indices into the Scene's mesh list
     */
    private int[] meshIndices = null;


    /**
     * Local transformation matrix of the node
     * Stored in row-major order.
     */
    private float[] nodeTransform = null;


    /**
     * Name of the node
     * The name might be empty (length of zero) but all nodes which
     * need to be accessed afterwards by bones or anims are usually named.
     */
    private String name = "";


    /**
     * List of all child nodes
     * May be empty
     */
    private Vector<Node> children = null;
    private int numChildren = 0; // temporary

    /**
     * Parent scene
     */
    private Scene parentScene = null;


    /**
     * Parent node or null if we're the root node of the scene
     */
    private Node parent = null;

    /**
     * Constructs a new node and initializes it
     * @param parentScene Parent scene object
     * @param parentNode Parent node or null for root nodes
     * @param index Unique index of the node
     */
    public Node(Scene parentScene, Node parentNode, int index) {

        this.parentScene = parentScene;
        this.parent = parentNode;

        // Initialize JNI class members, including numChildren
        this._NativeInitMembers(parentScene.getImporter().getContext(),index);

        // get all children of the node
        for (int i = 0; i < numChildren;++i) {
            this.children.add(new Node(parentScene, this,  ++index));
        }
    }


    /**
     * Get a list of all meshes of this node
     *
     * @return Array containing indices into the Scene's mesh list
     */
    int[] getMeshes() {
        return meshIndices;
    }


    /**
     * Get the local transformation matrix of the node in row-major
     * order:
     * <code>
     * a1 a2 a3 a4 (the translational part of the matrix is stored
     * b1 b2 b3 b4  in (a4|b4|c4))
     * c1 c2 c3 c4
     * d1 d2 d3 d4
     * </code>
     *
     * @return Row-major transformation matrix
     */
    float[] getTransformRowMajor() {
        return nodeTransform;
    }


    /**
     * Get the local transformation matrix of the node in column-major
     * order:
     * <code>
     * a1 b1 c1 d1 (the translational part of the matrix is stored
     * a2 b2 c2 d2  in (a4|b4|c4))
     * a3 b3 c3 d3
     * a4 b4 c4 d4
     * </code>
     *
     * @return Column-major transformation matrix
     */
    float[] getTransformColumnMajor() {

        float[] transform = new float[16];
        transform[0] = nodeTransform[0];
        transform[1] = nodeTransform[4];
        transform[2] = nodeTransform[8];
        transform[3] = nodeTransform[12];
        transform[4] = nodeTransform[1];
        transform[5] = nodeTransform[5];
        transform[6] = nodeTransform[9];
        transform[7] = nodeTransform[13];
        transform[8] = nodeTransform[2];
        transform[9] = nodeTransform[6];
        transform[10] = nodeTransform[10];
        transform[11] = nodeTransform[14];
        transform[12] = nodeTransform[3];
        transform[13] = nodeTransform[7];
        transform[15] = nodeTransform[11];
        transform[16] = nodeTransform[15];
        return transform;
    }


    private native int _NativeInitMembers(long context, int nodeIndex);


    /**
     * Get the name of the node.
     * The name might be empty (length of zero) but all nodes which
     * need to be accessed afterwards by bones or anims are usually named.
     *
     * @return Node name
     */
    public String getName() {
        return name;
    }


    /**
     * Get the list of all child nodes of *this* node
     * @return List of children. May be empty.
     */
    public Vector<Node> getChildren() {
        return children;
    }

    /**
     * Get the parent node of the node
     * @return Parent node
     */
    public Node getParent() {
        return parent;
    }

    /**
     * Get the parent scene of the node
     * @return Never null
     */
    public Scene getParentScene() {
        return parentScene;
    }
}
