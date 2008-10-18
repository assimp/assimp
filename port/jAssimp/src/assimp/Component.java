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

// ---------------------------------------------------------------------------
/** Enumerates components of the <code>Scene</code> and <code>Mesh</code> 
 *  classes that can be excluded from the import with the RemoveComponent step.
 *
 *  See the documentation for the postprocessing step for more details.
 *  @author Aramis (Alexander Gessler)
 *  @version 1.0
 */
public class Component
{
	/** Normal vectors are removed from all meshes
	 */
	public static final int NORMALS = 0x2;

	/** Tangents an bitangents are removed from all meshes
	 * 
	 * Tangents and bitangents below together in every case.
	 */
	public static final int TANGENTS_AND_BITANGENTS = 0x4;

	/** All vertex color sets are removed
	 * 
	 * Use <code>COLORn(N)</code> to specifiy the N'th set 
	 */
	public static final int COLORS = 0x8;

	/** All texture UV sets are removed
	 * 
	 * Use <code>TEXCOORDn(N)</code> to specifiy the N'th set 
	 */
	public static final int TEXCOORDS = 0x10;

	/** Removes all bone weights from all meshes.
	 * 
	 * The scenegraph nodes corresponding to the
	 * bones are removed
	 */
	public static final int BONEWEIGHTS = 0x20;

	/** Removes all bone animations 
	 */
	public static final int ANIMATIONS = 0x40;

	/** Removes all embedded textures 
	 */
	public static final int TEXTURES = 0x80;

	/** Removes all light sources 
	 * 
	 * The scenegraph nodes corresponding to the
	 * light sources are removed.
	 */
	public static final int LIGHTS = 0x100;

	/** Removes all light sources
	 * 
	 *  The scenegraph nodes corresponding to the
	 *  cameras are removed.
	 */
	public static final int CAMERAS = 0x200;

	/** Removes all meshes (aiScene::mMeshes). 
	 */
	public static final int MESHES = 0x400;

	/** Removes all materials. One default material will
	 *  be generated, so aiScene::mNumMaterials will be 1.
	 *  This makes no real sense without the <code>TEXTURES</code> flag.
	 * */
	public static final int MATERIALS = 0x800;


	public static final int COLORSn(int n)
	{
		return (1 << (n + 20));
	}

	public static final int TEXCOORDSn(int n)
	{
		return (1 << (n + 25));
	}
};
