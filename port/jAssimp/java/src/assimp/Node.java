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

import java.io.IOException;
import java.io.PrintStream;

/**
 * A node in the imported scene hierarchy.
 * 
 * Each node has name, a single parent node (except for the root node), a
 * transformation relative to its parent and possibly several child nodes.
 * Simple file formats don't support hierarchical structures, for these formats
 * the imported scene does consist of only a single root node with no childs.
 * Multiple meshes can be assigned to a single node.
 * 
 * 
 * @author Alexander Gessler (aramis_acg@users.sourceforge.net)
 * @version 1.0
 */
public class Node {

	/**
	 * Constructs a new node and initializes it
	 * 
	 * @param parentNode
	 *            Parent node or null for root nodes
	 */
	public Node(Node parentNode) {

		this.parent = parentNode;
	}

	/**
	 * Returns the number of meshes of this node
	 * 
	 * @return Number of meshes
	 */
	public final int getNumMeshes() {
		return meshIndices.length;
	}

	/**
	 * Get a list of all meshes of this node
	 * 
	 * @return Array containing indices into the Scene's mesh list. If there are
	 *         no meshes, the array is <code>null</code>
	 */
	public final int[] getMeshes() {
		return meshIndices;
	}

	/**
	 * Get the local transformation matrix of the node in row-major order:
	 * <code>
	 * a1 a2 a3 a4 (the translational part of the matrix is stored
	 * b1 b2 b3 b4  in (a4|b4|c4))
	 * c1 c2 c3 c4
	 * d1 d2 d3 d4
     * </code>
	 * 
	 * @return Row-major transformation matrix
	 */
	public final Matrix4x4 getTransformRowMajor() {
		return nodeTransform;
	}

	/**
	 * Get the local transformation matrix of the node in column-major order:
	 * <code>
	 * a1 b1 c1 d1 (the translational part of the matrix is stored
	 * a2 b2 c2 d2  in (a4|b4|c4))
	 * a3 b3 c3 d3
	 * a4 b4 c4 d4
     * </code>
	 * 
	 * @return Column-major transformation matrix
	 */
	public final Matrix4x4 getTransformColumnMajor() {

		Matrix4x4 m = new Matrix4x4(nodeTransform);
		return m.transpose();
	}

	/**
	 * Get the name of the node. The name might be empty (length of zero) but
	 * all nodes which need to be accessed afterwards by bones or anims are
	 * usually named.
	 * 
	 * @return Node name
	 */
	public final String getName() {
		return name;
	}

	/**
	 * Get the list of all child nodes of *this* node
	 * 
	 * @return List of children. May be empty.
	 */
	public final Node[] getChildren() {
		return children;
	}

	/**
	 * Get the number of child nodes of *this* node
	 * 
	 * @return May be 0
	 */
	public final int getNumChildren() {
		return children.length;
	}

	/**
	 * Get the parent node of the node
	 * 
	 * @return Parent node
	 */
	public final Node getParent() {
		return parent;
	}

	/**
	 * Searches this node and recursively all sub nodes for a node with a
	 * specific name
	 * 
	 * @param _name
	 *            Name of the node to search for
	 * @return Either a reference to the node or <code>null</code> if no node
	 *         with this name was found.
	 */
	public final Node findNode(String _name) {

		if (_name.equals(name))
			return this;
		for (Node node : children) {
			Node out;
			if (null != (out = node.findNode(_name)))
				return out;
		}
		return null;
	}

	/**
	 * Print all nodes recursively. This is a debugging utility.
	 * @param stream
	 *            Output stream
	 * @param suffix
	 *            Suffix to all output
	 * @throws IOException
	 *             yes ... sometimes ... :-)
	 */
	public void printNodes(PrintStream stream, String suffix)
			throws IOException {
		String suffNew = suffix + "\t";
		stream.println(suffix + getName());

		// print all mesh indices
		if (0 != getNumMeshes()) {
			stream.println(suffNew + "Meshes: ");

			for (int i : getMeshes()) {
				stream.println(i + " ");
			}
			stream.println("");
		}

		// print all children
		if (0 != getNumChildren()) {
			for (Node n : getChildren()) {

				n.printNodes(stream, suffNew);
			}
		}
	}

	/**
	 * List of all meshes of this node. The array contains indices into the
	 * Scene's mesh list
	 */
	private int[] meshIndices = null;

	/**
	 * Local transformation matrix of the node Stored in row-major order.
	 */
	private Matrix4x4 nodeTransform = null;

	/**
	 * Name of the node The name might be empty (length of zero) but all nodes
	 * which need to be accessed afterwards by bones or anims are usually named.
	 */
	private String name = "";

	/**
	 * List of all child nodes May be empty
	 */
	private Node[] children = null;

	/**
	 * Parent node or null if we're the root node of the scene
	 */
	private Node parent = null;
}
