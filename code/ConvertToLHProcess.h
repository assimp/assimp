/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/

/** @file Defines a post processing step to convert all data to a left-handed coordinate system.*/
#ifndef AI_CONVERTTOLHPROCESS_H_INC
#define AI_CONVERTTOLHPROCESS_H_INC

#include "../include/aiTypes.h"
#include "BaseProcess.h"

struct aiMesh;
struct aiNodeAnim;

namespace Assimp
{

// ---------------------------------------------------------------------------
/** The ConvertToLHProcess converts all imported data to a left-handed coordinate
 * system. This implies inverting the Z axis for all transformation matrices
 * invert the orientation of all faces, and adapting skinning and animation 
 * data in a similar way.
 */
class ASSIMP_API ConvertToLHProcess : public BaseProcess
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	ConvertToLHProcess();

	/** Destructor, private as well */
	~ConvertToLHProcess();

public:
	// -------------------------------------------------------------------
	/** Returns whether the processing step is present in the given flag field.
	 * @param pFlags The processing flags the importer was called with. A bitwise
	 *   combination of #aiPostProcessSteps.
	 * @return true if the process is present in this flag fields, false if not.
	*/
	bool IsActive( unsigned int pFlags) const;

	// -------------------------------------------------------------------
	/** Executes the post processing step on the given imported data.
	* At the moment a process is not supposed to fail.
	* @param pScene The imported data to work at.
	*/
	void Execute( aiScene* pScene);

	// -------------------------------------------------------------------
	/** Static helper function to convert a vector/matrix from DX coords to OGL coords.
	 * @param poMatrix The matrix to convert.
	 */
	static void ConvertToOGL( aiVector3D& poVector);
	static void ConvertToOGL( aiMatrix3x3& poMatrix);
	static void ConvertToOGL( aiMatrix4x4& poMatrix);

	// -------------------------------------------------------------------
	/** Static helper function to convert a vector/matrix from OGL coords back to DX coords.
	 * @param poMatrix The matrix to convert.
	 */
	static void ConvertToDX( aiVector3D& poVector);
	static void ConvertToDX( aiMatrix3x3& poMatrix);
	static void ConvertToDX( aiMatrix4x4& poMatrix);

protected:
	// -------------------------------------------------------------------
	/** Converts a single mesh to left handed coordinates. 
	 * This simply means the order of all faces is inverted.
	 * @param pMesh The mesh to convert.
	 */
	void ProcessMesh( aiMesh* pMesh);

	// -------------------------------------------------------------------
	/** Converts the given animation to LH coordinates. 
	 * The rotation and translation keys are transformed, the scale keys
	 * work in local space and can therefore be left untouched.
	 * @param pAnim The bone animation to transform
	 */
	void ProcessAnimation( aiNodeAnim* pAnim);

	//! true if the transformation matrix for the OGL-to-DX is 
	//! directly used to transform all vertices.
	mutable bool bTransformVertices;

public:
	/** The transformation matrix to convert from DirectX coordinates to OpenGL coordinates. */
	static const aiMatrix3x3 sToOGLTransform;
	/** The transformation matrix to convert from OpenGL coordinates to DirectX coordinates. */
	static const aiMatrix3x3 sToDXTransform;
};

} // end of namespace Assimp

#endif // AI_CONVERTTOLHPROCESS_H_INC
