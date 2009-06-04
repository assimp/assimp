/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2009, ASSIMP Development Team

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
 * Represents a single face of a mesh. Faces a generally simple polygons and
 * store n indices into the vertex streams of the mesh they belong to.
 * 
 * @author Alexander Gessler (aramis_acg@users.sourceforge.net)
 * @version 1.0
 */
public class Face {

	/**
	 * Different types of faces.
	 * 
	 * Faces of different primitive types can occur in a single mesh. To get
	 * homogeneous meshes, try the <code>PostProcessing.SortByPType</code> flag.
	 * 
	 * @see Mesh.mPrimitiveTypes
	 * @see PostProcessing#SortByPType
	 */
	public static class Type {

		/**
		 * This is just a single vertex in the virtual world. #aiFace contains
		 * just one index for such a primitive.
		 */
		public static final int POINT = 0x1;

		/**
		 * This is a line defined by start and end position. Surprise, Face
		 * defines two indices for a line.
		 */
		public static final int LINE = 0x2;

		/**
		 * A triangle, probably the only kind of primitive you wish to handle. 3
		 * indices.
		 */
		public static final int TRIANGLE = 0x4;

		/**
		 * A simple, non-intersecting polygon defined by n points (n > 3). Can
		 * be concave or convex. Use the <code>PostProcessing.Triangulate</code>
		 * flag to have all polygons triangulated.
		 * 
		 * @see PostProcessing.Triangulate
		 */
		public static final int POLYGON = 0x8;
	}

	/**
	 * Get the indices of the face
	 * 
	 * @return Array of n indices into the vertices of the father mesh. The
	 *         return value is *never* <code>null</code>
	 */
	public int[] getIndices() {
		assert(null != indices);
		return indices;
	}

	/**
	 * Indices of the face
	 */
	private int[] indices;
}
