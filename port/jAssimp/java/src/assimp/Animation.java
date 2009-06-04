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
 * An animation consists of keyframe data for a number of bones. For each bone
 * affected by the animation a separate series of data is given. There can be
 * multiple animations in a single scene.
 * 
 * @author Alexander Gessler (aramis_acg@users.sourceforge.net)
 * @version 1.0
 */
public class Animation {

	/**
	 * Returns the name of the animation channel
	 * 
	 * @return If the modeling package this data was exported from does support
	 *         only a single animation channel, this name is usually
	 *         <code>""</code>
	 */
	public final String getName() {
		return name;
	}

	/**
	 * Returns the total duration of the animation, in ticks
	 * 
	 * @return Total duration
	 */
	public final double getDuration() {
		return mDuration;
	}

	/**
	 * Returns the ticks per second count.
	 * 
	 * @return 0 if not specified in the imported file
	 */
	public final double getTicksPerSecond() {
		return mTicksPerSecond;
	}

	/**
	 * Returns the number of bone animation channels
	 * 
	 * @return This value is never 0
	 */
	public final int getNumNodeAnimChannels() {
		assert (null != boneAnims);
		return boneAnims.length;
	}

	/**
	 * Returns the list of all bone animation channels
	 * 
	 * @return This value is never <code>null</code>
	 */
	public final NodeAnim[] getNodeAnimChannels() {
		assert (null != boneAnims);
		return boneAnims;
	}

	// --------------------------------------------------------------------------
	// Private stuff
	// --------------------------------------------------------------------------

	/**
	 * The name of the animation.
	 */
	private String name = "";

	/**
	 * Duration of the animation in ticks.
	 */
	private double mDuration = 0.0;

	/**
	 * Ticks per second. 0 if not specified in the imported file
	 */
	private double mTicksPerSecond = 0.0;

	/**
	 * Bone animation channels
	 */
	private NodeAnim[] boneAnims = null;
}
