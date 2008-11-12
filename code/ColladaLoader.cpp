/** Implementation of the Collada loader */
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

#include "AssimpPCH.h"
#include "../include/aiAnim.h"
#include "ColladaLoader.h"
#include "ColladaParser.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ColladaLoader::ColladaLoader()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ColladaLoader::~ColladaLoader()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool ColladaLoader::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// check file extension 
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);
	for( std::string::iterator it = extension.begin(); it != extension.end(); ++it)
		*it = tolower( *it);

	if( extension == ".dae")
		return true;

	return false;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void ColladaLoader::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	mFileName = pFile;

	// parse the input file
	ColladaParser parser( pFile);

	// build the node hierarchy from it
	pScene->mRootNode = BuildHierarchy( parser, parser.mRootNode);

	pScene->mFlags = AI_SCENE_FLAGS_INCOMPLETE;
}

// ------------------------------------------------------------------------------------------------
// Recursively constructs a scene node for the given parser node and returns it.
aiNode* ColladaLoader::BuildHierarchy( const ColladaParser& pParser, const ColladaParser::Node* pNode)
{
	// create a node for it
	aiNode* node = new aiNode( pNode->mName);
	
	// calculate the transformation matrix for it
	node->mTransformation = pParser.CalculateResultTransform( pNode->mTransforms);

	// add children
	node->mNumChildren = pNode->mChildren.size();
	node->mChildren = new aiNode*[node->mNumChildren];
	for( unsigned int a = 0; a < pNode->mChildren.size(); a++)
	{
		node->mChildren[a] = BuildHierarchy( pParser, pNode->mChildren[a]);
		node->mChildren[a]->mParent = node;
	}

	return node;
}
