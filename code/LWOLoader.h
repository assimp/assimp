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

/** @file Declaration of the LWO importer class. */
#ifndef AI_LWOLOADER_H_INCLUDED
#define AI_LWOLOADER_H_INCLUDED

#include "../include/aiTypes.h"

#include "BaseImporter.h"
#include "LWOFileData.h"

#include <vector>

namespace Assimp	{
using namespace LWO;

// ---------------------------------------------------------------------------
/** Clas to load LWO files
*/
class LWOImporter : public BaseImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	LWOImporter();

	/** Destructor, private as well */
	~LWOImporter();

public:

	// -------------------------------------------------------------------
	/** Returns whether the class can handle the format of the given file. 
	* See BaseImporter::CanRead() for details.	*/
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler) const;

protected:

	// -------------------------------------------------------------------
	/** Called by Importer::GetExtensionList() for each loaded importer.
	 * See BaseImporter::GetExtensionList() for details
	 */
	void GetExtensionList(std::string& append)
	{
		append.append("*.lwo");
	}

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	* See BaseImporter::InternReadFile() for details
	*/
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);


	// -------------------------------------------------------------------
	/** Loads a LWO file in the older LWOB format (LW < 6)
	*/
	void LoadLWOBFile();

	// -------------------------------------------------------------------
	/** Loads a LWO file in the newer LWO2 format (LW >= 6)
	*/
	void LoadLWO2File();

	// -------------------------------------------------------------------
	/** Loads a surface chunk from an LWOB file
	*/
	void LoadLWOBSurface(unsigned int size);


	typedef std::vector<aiVector3D> PointList;
	typedef std::vector<LWO::Face> FaceList;
	typedef std::vector<LWO::Surface> SurfaceList;

private:

	// -------------------------------------------------------------------
	/** Count vertices and faces in a LWOB file
	*/
	void CountVertsAndFaces(unsigned int& verts, 
		unsigned int& faces,
		LE_NCONST uint8_t*& cursor, 
		const uint8_t* const end,
		unsigned int max = 0xffffffff);

	// -------------------------------------------------------------------
	/** Read vertices and faces in a LWOB file
	*/
	void CopyFaceIndices(FaceList::iterator& it,
		LE_NCONST uint8_t*& cursor, 
		const uint8_t* const end, 
		unsigned int max = 0xffffffff);

protected:

	/** Temporary point list from the file */
	PointList mTempPoints;

	/** Temporary face list from the file*/
	FaceList mFaces;

	/** Temporary surface list from the file */
	SurfaceList mSurfaces;

	/** file buffer */
	LE_NCONST uint8_t* mFileBuffer;

	/** Size of the file, in bytes */
	unsigned int fileSize;

	/** Output scene */
	aiScene* pScene;

};

} // end of namespace Assimp

#endif // AI_LWOIMPORTER_H_INCLUDED