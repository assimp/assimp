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

/** @file Implementation of the DXF importer class */
#include "DXFLoader.h"
#include "ParsingUtils.h"
#include "fast_atof.h"
#include "MaterialSystem.h"

// public ASSIMP headers
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "../include/IOStream.h"
#include "../include/IOSystem.h"

#include "../include/DefaultLogger.h"

// boost headers
#include <boost/scoped_ptr.hpp>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
DXFImporter::DXFImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
DXFImporter::~DXFImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool DXFImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);

	return !(extension.length() != 4 || extension[0] != '.' ||
			 extension[1] != 'd' && extension[1] != 'D' ||
			 extension[2] != 'x' && extension[2] != 'X' ||
			 extension[3] != 'f' && extension[3] != 'F');
}

// ------------------------------------------------------------------------------------------------
bool DXFImporter::GetNextLine()
{
	if(!SkipLine(&buffer))return false;
	if(!SkipSpaces(&buffer))return GetNextLine();
	return true;
}

// ------------------------------------------------------------------------------------------------
bool DXFImporter::GetNextToken()
{
	SkipSpaces(&buffer);
	groupCode = strtol10s(buffer,&buffer);
	if(!GetNextLine())return false;
	
	// copy the data line to a separate buffer
	char* m = cursor, *end = &cursor[4096];
	while (!IsLineEnd ( *buffer ) && m < end)
		*m++ = *buffer++;
	
	*m = '\0';
	GetNextLine(); 
	return true;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void DXFImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open DXF file " + pFile + "");

	// read the contents of the file in a buffer
	unsigned int m = (unsigned int)file->FileSize();
	std::vector<char> buffer2(m+1);
	buffer = &buffer2[0];
	file->Read( &buffer2[0], m,1);
	buffer2[m] = '\0';

	mDefaultLayer = NULL;

	// now get all lines of the file
	while (GetNextToken())
	{
		if (2 == groupCode)
		{
			// ENTITIES section
			if (!::strcmp(cursor,"ENTITIES"))
				if (!ParseEntities())break;

			// other sections such as BLOCK - skip them to make
			// sure there will be no name conflicts
			else
			{
				bool b = false;
				while (GetNextToken())
				{
					if (!groupCode && !::strcmp(cursor,"ENDSEC"))
						{b = true;break;}
				}
				if (!b)break;
			}
		}
		// print comment strings
		else if (999 == groupCode)
		{
			DefaultLogger::get()->info(std::string( cursor ));
		}
		else if (!groupCode && !::strcmp(cursor,"EOF"))
			break;
	}

	if (mLayers.empty())
		throw new ImportErrorException("DXF: this file contains no 3d data");

	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes = (unsigned int)mLayers.size()];
	m = 0;
	for (std::vector<LayerInfo>::const_iterator it = mLayers.begin(),end = mLayers.end();
		it != end;++it)
	{
		// generate the output mesh
		aiMesh* pMesh = pScene->mMeshes[m++] = new aiMesh();
		const std::vector<aiVector3D>& vPositions = (*it).vPositions;

		pMesh->mNumFaces = (unsigned int)vPositions.size() / 4u;
		pMesh->mFaces = new aiFace[pMesh->mNumFaces];

		aiVector3D* vpOut = pMesh->mVertices = new aiVector3D[vPositions.size()];
		const aiVector3D* vp = &vPositions[0];

		for (unsigned int i = 0; i < pMesh->mNumFaces;++i)
		{
			aiFace& face = pMesh->mFaces[i];

			// check whether we need four indices here
			if (vp[3] != vp[2])
			{
				face.mNumIndices = 4;
			}
			else face.mNumIndices = 3;
			face.mIndices = new unsigned int[face.mNumIndices];

			for (unsigned int a = 0; a < face.mNumIndices;++a)
			{
				*vpOut++ = vp[a];
				face.mIndices[a] = pMesh->mNumVertices++;
			}
			vp += 4;
		}
	}

	// generate the output scene graph
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mName.Set("<DXF_ROOT>");

	if (1 == pScene->mNumMeshes)
	{
		pScene->mRootNode->mMeshes = new unsigned int[ pScene->mRootNode->mNumMeshes = 1 ];
		pScene->mRootNode->mMeshes[0] = 0;
	}
	else
	{
		pScene->mRootNode->mChildren = new aiNode*[ pScene->mRootNode->mNumChildren = pScene->mNumMeshes ];
		for (m = 0; m < pScene->mRootNode->mNumChildren;++m)
		{
			aiNode* p = pScene->mRootNode->mChildren[m] = new aiNode();
			p->mName.length = ::strlen( mLayers[m].name );
			::strcpy(p->mName.data, mLayers[m].name);

			p->mMeshes = new unsigned int[p->mNumMeshes = 1];
			p->mMeshes[0] = m;
			p->mParent = pScene->mRootNode;
		}
	}

	// generate a default material
	MaterialHelper* pcMat = new MaterialHelper();
	aiString s;
	s.Set(AI_DEFAULT_MATERIAL_NAME);
	pcMat->AddProperty(&s, AI_MATKEY_NAME);

	aiColor4D clrDiffuse(0.6f,0.6f,0.6f,1.0f);
	pcMat->AddProperty(&clrDiffuse,1,AI_MATKEY_COLOR_DIFFUSE);

	clrDiffuse = aiColor4D(1.0f,1.0f,1.0f,1.0f);
	pcMat->AddProperty(&clrDiffuse,1,AI_MATKEY_COLOR_SPECULAR);

	clrDiffuse = aiColor4D(0.05f,0.05f,0.05f,1.0f);
	pcMat->AddProperty(&clrDiffuse,1,AI_MATKEY_COLOR_AMBIENT);

	pScene->mNumMaterials = 1;
	pScene->mMaterials = new aiMaterial*[1];
	pScene->mMaterials[0] = pcMat;

	// --- everything destructs automatically ---
}

// ------------------------------------------------------------------------------------------------
bool DXFImporter::ParseEntities()
{
	while (GetNextToken())
	{
		if (!groupCode)
		{
			while (true) {
			if (!::strcmp(cursor,"3DFACE"))
				if (!Parse3DFace()) return false; else continue;
			break;
			};
			if (!::strcmp(cursor,"ENDSEC"))
				return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
bool DXFImporter::Parse3DFace()
{
	bool ret = false;
	LayerInfo* out = NULL;

	aiVector3D vip[4]; // -- vectors are initialized to zero
	while (GetNextToken())
	{
		switch (groupCode)
		{
		case 0: ret = true;break;

		// 8 specifies the layer
		case 8:
			{
				for (std::vector<LayerInfo>::iterator it = mLayers.begin(),end = mLayers.end();
					it != end;++it)
				{
					if (!::strcmp( (*it).name, cursor ))
					{
						out = &(*it);
						break;
					}
				}
				if (!out)
				{
					// we don't have this layer yet
					mLayers.push_back(LayerInfo());
					out = &mLayers.back();
					::strcpy(out->name,cursor);
				}
				break;
			}

		// x position of the first corner
		case 10: vip[0].x = fast_atof(cursor);break;

		// y position of the first corner
		case 20: vip[0].y = -fast_atof(cursor);break;

		// z position of the first corner
		case 30: vip[0].z = fast_atof(cursor);break;

		// x position of the second corner
		case 11: vip[1].x = fast_atof(cursor);break;

		// y position of the second corner
		case 21: vip[1].y = -fast_atof(cursor);break;

		// z position of the second corner
		case 31: vip[1].z = fast_atof(cursor);break;

		// x position of the third corner
		case 12: vip[2].x = fast_atof(cursor);break;

		// y position of the third corner
		case 22: vip[2].y = -fast_atof(cursor);break;

		// z position of the third corner
		case 32: vip[2].z = fast_atof(cursor);break;

		// x position of the fourth corner
		case 13: vip[3].x = fast_atof(cursor);break;

		// y position of the fourth corner
		case 23: vip[3].y = -fast_atof(cursor);break;

		// z position of the fourth corner
		case 33: vip[3].z = fast_atof(cursor);break;
		};
		if (ret)break;
	}

	// use a default layer if necessary
	if (!out)
	{
		if (!mDefaultLayer)
		{
			mLayers.push_back(LayerInfo());
			mDefaultLayer = &mLayers.back();
		}
		out = mDefaultLayer;
	}

	// add the faces to the face list for this layer
	out->vPositions.push_back(vip[0]);
	out->vPositions.push_back(vip[1]);
	out->vPositions.push_back(vip[2]);
	out->vPositions.push_back(vip[3]);
	return ret;
}
