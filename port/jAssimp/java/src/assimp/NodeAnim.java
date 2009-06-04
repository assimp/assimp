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
 * A bone animation channel defines the animation keyframes for a single bone in
 * the mesh hierarchy.
 * 
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class NodeAnim {

	/**
	 * Describes a keyframe
	 */
	public class KeyFrame<Type> {

		/**
		 * Time line position of *this* keyframe, in "ticks"
		 */
		public double time;

		/**
		 * Current value of the property being animated
		 */
		public Type value;
	}

	/**
	 * Returns the name of the bone affected by this animation channel
	 * 
	 * @return Bone name
	 */
	public final String getName() {
		return mName;
	}

	/**
	 * Returns the number of rotation keyframes
	 * 
	 * @return This can be 0.
	 */
	public final int getNumQuatKeys() {
		return null == mQuatKeys ? 0 : mQuatKeys.length;
	}

	/**
	 * Returns the number of position keyframes
	 * 
	 * @return This can be 0.
	 */
	public final int getNumPosKeys() {
		return null == mPosKeys ? 0 : mPosKeys.length;
	}

	/**
	 * Returns the number of scaling keyframes
	 * 
	 * @return This can be 0.
	 */
	public final int getNumScalingKeys() {
		return null == mScalingKeys ? 0 : mScalingKeys.length;
	}

	/**
	 * Get a reference to the list of all rotation keyframes
	 * 
	 * @return Could be <code>null</code> if there are no rotation keys
	 */
	public final KeyFrame<Quaternion>[] getQuatKeys() {
		return mQuatKeys;
	}

	/**
	 * Get a reference to the list of all position keyframes
	 * 
	 * @return Could be <code>null</code> if there are no position keys
	 */
	public final KeyFrame<float[]>[] getPosKeys() {
		return mPosKeys;
	}

	/**
	 * Get a reference to the list of all scaling keyframes
	 * 
	 * @return Could be <code>null</code> if there are no scaling keys
	 */
	public final KeyFrame<float[]>[] getScalingKeys() {
		return mScalingKeys;
	}

	// --------------------------------------------------------------------------
	// Private stuff
	// --------------------------------------------------------------------------
	
	/**
	 * Rotation keyframes
	 */
	private KeyFrame<Quaternion>[] mQuatKeys = null;

	/**
	 * Position keyframes. Component order is x,y,z
	 */
	private KeyFrame<float[]>[] mPosKeys = null;

	/**
	 * scaling keyframes. Component order is x,y,z
	 */
	private KeyFrame<float[]>[] mScalingKeys = null;

	/**
	 * Name of the bone affected by this animation channel
	 */
	private String mName;
}
