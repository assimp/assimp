/** Defines the collada loader class */

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

#ifndef AI_COLLADALOADER_H_INC
#define AI_COLLADALOADER_H_INC

#include "BaseImporter.h"
#include "ColladaParser.h"

namespace Assimp
{

/** Loader class to read Collada scenes. Collada is over-engineered to death, with every new iteration bringing
 * more useless stuff, so I limited the data to what I think is useful for games. 
*/
class ColladaLoader : public BaseImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	ColladaLoader();

	/** Destructor, private as well */
	~ColladaLoader();

public:
	/** Returns whether the class can handle the format of the given file. 
	 * See BaseImporter::CanRead() for details.	*/
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler) const;

protected:
	/** Called by Importer::GetExtensionList() for each loaded importer.
	 * See BaseImporter::GetExtensionList() for details
	 */
	void GetExtensionList( std::string& append)
	{
		append.append("*.dae");
	}

	/** Imports the given file into the given scene structure. 
	 * See BaseImporter::InternReadFile() for details
	 */
	void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler);

	/** Recursively constructs a scene node for the given parser node and returns it. */
	aiNode* BuildHierarchy( const ColladaParser& pParser, const Collada::Node* pNode);

	/** Builds meshes for the given node and references them */
	void BuildMeshesForNode( const ColladaParser& pParser, const Collada::Node* pNode, aiNode* pTarget);

	/** Stores all meshes in the given scene */
	void StoreSceneMeshes( aiScene* pScene);

protected:
	/** Filename, for a verbose error message */
	std::string mFileName;

	/** Which mesh-material compound was stored under which mesh ID */
	std::map<std::string, size_t> mMeshIndexbyID;

	/** Accumulated meshes for the target scene */
	std::vector<aiMesh*> mMeshes;
};

} // end of namespace Assimp

#endif // AI_COLLADALOADER_H_INC
