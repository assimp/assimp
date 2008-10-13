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


/** @file Definition of the .MD5 importer class.
http://www.modwiki.net/wiki/MD5_(file_format)
*/
#ifndef AI_MD5LOADER_H_INCLUDED
#define AI_MD5LOADER_H_INCLUDED

#include "BaseImporter.h"
#include "MD5Parser.h"

#include "../include/aiTypes.h"

namespace Assimp	{

class IOStream;
using namespace Assimp::MD5;

// ---------------------------------------------------------------------------
/** Used to load MD5 files
*/
class MD5Importer : public BaseImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	MD5Importer();

	/** Destructor, private as well */
	~MD5Importer();

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
		append.append("*.md5mesh;*.md5anim");
	}

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	* See BaseImporter::InternReadFile() for details
	*/
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

protected:


	// -------------------------------------------------------------------
	/** Load the *.MD5MESH file.
	 * Must be called at first.
	*/
	void LoadMD5MeshFile ();

	// -------------------------------------------------------------------
	/** Load the *.MD5ANIM file.
	*/
	void LoadMD5AnimFile ();

	// -------------------------------------------------------------------
	/** Load the contents of a specific file into memory and
	 *  alocates a buffer to keep it.
	 *
	 *  mBuffer is changed to point to this buffer.
	 *  Don't forget to delete it later ...
	 *  @param pFile File stream to be read
	*/
	void LoadFileIntoMemory (IOStream* pFile);
	void UnloadFileFromMemory ();


	/** IOSystem to be used to access files */
	IOSystem* mIOHandler;

	/** Path to the file, excluding the file extension but
	    with the dot */
	std::string mFile;

	/** Buffer to hold the loaded file */
	char* mBuffer;

	/** Size of the file */
	unsigned int fileSize;

	/** Current line number. For debugging purposes */
	unsigned int iLineNumber;

	/** Scene to be filled */
	aiScene* pScene;

	/** (Custom) I/O handler implementation */
	IOSystem* pIOHandler;

	/** true if the MD5MESH file has already been parsed */
	bool bHadMD5Mesh;

	/** true if the MD5ANIM file has already been parsed */
	bool bHadMD5Anim;
};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC
