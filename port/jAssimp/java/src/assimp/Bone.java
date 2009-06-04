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
 * A bone belongs to a mesh stores a list of vertex weights. It represents a
 * joint of the skeleton. The bone hierarchy is contained in the node graph.
 * 
 * @author Alexander Gessler (aramis_acg@users.sourceforge.net)
 * @version 1.0
 */
public class Bone {

	/**
	 * Represents a single vertex weight
	 */
	public class Weight {

		public Weight() {
			index = 0;
			weight = 1.0f;
		}

		/**
		 * Index of the vertex in the corresponding <code>Mesh</code>
		 */
		public int index;

		/**
		 * Weight of the vertex. All weights for a vertex sum up to 1.0
		 */
		public float weight;
	}

	/**
	 * Retrieves the name of the node
	 * 
	 * @return Normally bones are never unnamed
	 */
	public final String getName() {
		return name;
	}

	/**
	 * Returns a reference to the array of weights
	 * 
	 * @return <code>Weight</code> array
	 */
	public final Weight[] getWeightsArray() {
		assert (null != weights);
		return weights;
	}

	/**
	 * Returns the number of bone weights.
	 * 
	 * @return There should at least be one vertex weights (the validation step
	 *         would complain otherwise)
	 */
	public final int getNumWeights() {
		assert (null != weights);
		return weights.length;
	}
	
	// --------------------------------------------------------------------------
	// Private stuff
	// --------------------------------------------------------------------------

	/**
	 * Name of the bone
	 */
	private String name = "";

	/**
	 * List of vertex weights for the bone
	 */
	private Weight[] weights = null;
}
