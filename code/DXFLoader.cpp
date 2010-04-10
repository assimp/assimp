/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

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

/** @file  DXFLoader.cpp
 *  @brief Implementation of the DXF importer class
 */

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_DXF_IMPORTER

#include "DXFLoader.h"
#include "ParsingUtils.h"
#include "ConvertToLHProcess.h"
#include "fast_atof.h"

using namespace Assimp;

// AutoCAD Binary DXF<CR><LF><SUB><NULL> 
#define AI_DXF_BINARY_IDENT ("AutoCAD Binary DXF\r\n\x1a\0")
#define AI_DXF_BINARY_IDENT_LEN (24)

// color indices for DXF - 16 are supported
static aiColor4D g_aclrDxfIndexColors[] =
{
	aiColor4D (0.6f, 0.6f, 0.6f, 1.0f),
	aiColor4D (1.0f, 0.0f, 0.0f, 1.0f), // red
	aiColor4D (0.0f, 1.0f, 0.0f, 1.0f), // green
	aiColor4D (0.0f, 0.0f, 1.0f, 1.0f), // blue
	aiColor4D (0.3f, 1.0f, 0.3f, 1.0f), // light green
	aiColor4D (0.3f, 0.3f, 1.0f, 1.0f), // light blue
	aiColor4D (1.0f, 0.3f, 0.3f, 1.0f), // light red
	aiColor4D (1.0f, 0.0f, 1.0f, 1.0f), // pink
	aiColor4D (1.0f, 0.6f, 0.0f, 1.0f), // orange
	aiColor4D (0.6f, 0.3f, 0.0f, 1.0f), // dark orange
	aiColor4D (1.0f, 1.0f, 0.0f, 1.0f), // yellow
	aiColor4D (0.3f, 0.3f, 0.3f, 1.0f), // dark gray
	aiColor4D (0.8f, 0.8f, 0.8f, 1.0f), // light gray
	aiColor4D (0.0f, 00.f, 0.0f, 1.0f), // black
	aiColor4D (1.0f, 1.0f, 1.0f, 1.0f), // white
	aiColor4D (0.6f, 0.0f, 1.0f, 1.0f)  // violet
};
#define AI_DXF_NUM_INDEX_COLORS (sizeof(g_aclrDxfIndexColors)/sizeof(g_aclrDxfIndexColors[0]))

// invalid/unassigned color value
aiColor4D g_clrInvalid = aiColor4D(get_qnan(),0.f,0.f,1.f);

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
DXFImporter::DXFImporter()
{}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
DXFImporter::~DXFImporter()
{}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool DXFImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
	return SimpleExtensionCheck(pFile,"dxf");
}

// ------------------------------------------------------------------------------------------------
// Get a list of all supported file extensions
void DXFImporter::GetExtensionList(std::set<std::string>& extensions)
{
	extensions.insert("dxf");
}

// ------------------------------------------------------------------------------------------------
// Get a copy of the next data line, skip strange data
bool DXFImporter::GetNextLine()
{
	if(!SkipLine(&buffer))
		return false;
	if(!SkipSpaces(&buffer))
		return GetNextLine();
	else if (*buffer == '{')	{
		// some strange meta data ...
		while (true)
		{
			if(!SkipLine(&buffer))
				return false;

			if(SkipSpaces(&buffer) && *buffer == '}')
				break;
		}
		return GetNextLine();
	}
	return true;
}

// ------------------------------------------------------------------------------------------------
// Get the next token in the file
bool DXFImporter::GetNextToken()
{
	if (bRepeat)	{
		bRepeat = false;
		return true;
	}

	SkipSpaces(&buffer);
	groupCode = strtol10s(buffer,&buffer);
	if(!GetNextLine())
		return false;
	
	// copy the data line to a separate buffer
	char* m = cursor, *end = &cursor[4096];
	while (!IsSpaceOrNewLine( *buffer ) && m < end)
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
	if( file.get() == NULL) {
		throw DeadlyImportError( "Failed to open DXF file " + pFile + "");
	}

	// read the contents of the file in a buffer
	std::vector<char> buffer2;
	TextFileToBuffer(file.get(),buffer2);
	buffer = &buffer2[0];

	bRepeat = false;
	mDefaultLayer = NULL;

	// check whether this is a binaray DXF file - we can't read binary DXF files :-(
	if (!strncmp(AI_DXF_BINARY_IDENT,buffer,AI_DXF_BINARY_IDENT_LEN))
		throw DeadlyImportError("DXF: Binary files are not supported at the moment");

	// now get all lines of the file
	while (GetNextToken())	{

		if (2 == groupCode)	{

			// ENTITIES and BLOCKS sections - skip the whole rest, no need to waste our time with them
			if (!::strcmp(cursor,"ENTITIES") || !::strcmp(cursor,"BLOCKS")) {
				if (!ParseEntities())
					break; 
				else bRepeat = true;
			}

			// other sections - skip them to make sure there will be no name conflicts
			else	{
				while ( GetNextToken())	{
					if (!::strcmp(cursor,"ENDSEC"))
						break;
				}
			}
		}
		// print comment strings
		else if (999 == groupCode)	{
			DefaultLogger::get()->info(std::string( cursor ));
		}
		else if (!groupCode && !::strcmp(cursor,"EOF"))
			break;
	}

	// find out how many valud layers we have
	for (std::vector<LayerInfo>::const_iterator it = mLayers.begin(),end = mLayers.end(); it != end;++it)	{
		if (!(*it).vPositions.empty())
			++pScene->mNumMeshes;
	}

	if (!pScene->mNumMeshes)
		throw DeadlyImportError("DXF: this file contains no 3d data");

	pScene->mMeshes = new aiMesh*[ pScene->mNumMeshes ];
	unsigned int m = 0;
	for (std::vector<LayerInfo>::const_iterator it = mLayers.begin(),end = mLayers.end();it != end;++it) {
		if ((*it).vPositions.empty()) {
			continue;
		}
		// generate the output mesh
		aiMesh* pMesh = pScene->mMeshes[m++] = new aiMesh();
		const std::vector<aiVector3D>& vPositions = (*it).vPositions;
		const std::vector<aiColor4D>& vColors = (*it).vColors;

		// check whether we need vertex colors here
		aiColor4D* clrOut = NULL;
		const aiColor4D* clr = NULL;
		for (std::vector<aiColor4D>::const_iterator it2 = (*it).vColors.begin(), end2 = (*it).vColors.end();it2 != end2; ++it2)	{
			
			if ((*it2).r == (*it2).r) /* qnan? */ {
				clrOut = pMesh->mColors[0] = new aiColor4D[vPositions.size()];
				for (unsigned int i = 0; i < vPositions.size();++i)
					clrOut[i] = aiColor4D(0.6f,0.6f,0.6f,1.0f);

				clr = &vColors[0];
				break;
			}
		}

		pMesh->mNumFaces = (unsigned int)vPositions.size() / 4u;
		pMesh->mFaces = new aiFace[pMesh->mNumFaces];

		aiVector3D* vpOut = pMesh->mVertices = new aiVector3D[vPositions.size()];
		const aiVector3D* vp = &vPositions[0];

		for (unsigned int i = 0; i < pMesh->mNumFaces;++i)	{
			aiFace& face = pMesh->mFaces[i];

			// check whether we need four, three or two indices here
			if (vp[1] == vp[2])	{
				face.mNumIndices = 2;
			}
			else if (vp[3] == vp[2])	{
				 face.mNumIndices = 3;
			}
			else face.mNumIndices = 4;
			face.mIndices = new unsigned int[face.mNumIndices];

			for (unsigned int a = 0; a < face.mNumIndices;++a)	{
				*vpOut++ = vp[a];
				if (clr)	{
					if (is_not_qnan( clr[a].r )) {
						*clrOut = clr[a];
					}
					++clrOut;
				}
				face.mIndices[a] = pMesh->mNumVertices++;
			}
			vp += 4;
		}
	}

	// generate the output scene graph
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mName.Set("<DXF_ROOT>");

	if (1 == pScene->mNumMeshes)	{
		pScene->mRootNode->mMeshes = new unsigned int[ pScene->mRootNode->mNumMeshes = 1 ];
		pScene->mRootNode->mMeshes[0] = 0;
	}
	else
	{
		pScene->mRootNode->mChildren = new aiNode*[ pScene->mRootNode->mNumChildren = pScene->mNumMeshes ];
		for (m = 0; m < pScene->mRootNode->mNumChildren;++m)	{
			aiNode* p = pScene->mRootNode->mChildren[m] = new aiNode();
			p->mName.length = ::strlen( mLayers[m].name );
			strcpy(p->mName.data, mLayers[m].name);

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

	// flip winding order to be ccw
	FlipWindingOrderProcess flipper;
	flipper.Execute(pScene);

	// --- everything destructs automatically ---
}

// ------------------------------------------------------------------------------------------------
bool DXFImporter::ParseEntities()
{
	while (GetNextToken())	{
		if (!groupCode)	{			
			if (!::strcmp(cursor,"3DFACE") || !::strcmp(cursor,"LINE") || !::strcmp(cursor,"3DLINE")){
				//http://sourceforge.net/tracker/index.php?func=detail&aid=2970566&group_id=226462&atid=1067632
				Parse3DFace();
				bRepeat = true;
			}
			if (!::strcmp(cursor,"POLYLINE") || !::strcmp(cursor,"LWPOLYLINE")){
				ParsePolyLine();
				bRepeat = true;
			}
			if (!::strcmp(cursor,"ENDSEC")) {
				return true;
			}
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
void DXFImporter::SetLayer(LayerInfo*& out)
{
	for (std::vector<LayerInfo>::iterator it = mLayers.begin(),end = mLayers.end();it != end;++it)	{
		if (!::strcmp( (*it).name, cursor ))	{
			out = &(*it);
			break;
		}
	}
	if (!out)	{
		// we don't have this layer yet
		mLayers.push_back(LayerInfo());
		out = &mLayers.back();
		::strcpy(out->name,cursor);
	}
}

// ------------------------------------------------------------------------------------------------
void DXFImporter::SetDefaultLayer(LayerInfo*& out)
{
	if (!mDefaultLayer)	{
		mLayers.push_back(LayerInfo());
		mDefaultLayer = &mLayers.back();
	}
	out = mDefaultLayer;
}

// ------------------------------------------------------------------------------------------------
bool DXFImporter::ParsePolyLine()
{
	bool ret = false;
	LayerInfo* out = NULL;

	std::vector<aiVector3D> positions;
	std::vector<aiColor4D>  colors;
	std::vector<unsigned int> indices;
	unsigned int flags = 0;

	while (GetNextToken())	{
		switch (groupCode)
		{
		case 0:
			{
				if (!::strcmp(cursor,"VERTEX"))	{
					aiVector3D v;aiColor4D clr(g_clrInvalid);
					unsigned int idx[4] = {0xffffffff,0xffffffff,0xffffffff,0xffffffff};
					ParsePolyLineVertex(v, clr, idx);
					if (0xffffffff == idx[0])	{
						positions.push_back(v);
						colors.push_back(clr);
					}
					else	{
						// check whether we have a fourth coordinate
						if (0xffffffff == idx[3])	{
							idx[3] = idx[2];
						}

						indices.reserve(indices.size()+4);
						for (unsigned int m = 0; m < 4;++m)
							indices.push_back(idx[m]);
					}
					bRepeat = true;
				}
				else if (!::strcmp(cursor,"ENDSEQ"))	{
					ret = true;
				}
				break;
			}

		// flags --- important that we know whether it is a polyface mesh
		case 70:
			{
				if (!flags)	{
					flags = strtol10(cursor);
				}
				break;
			};

		// optional number of vertices
		case 71:
			{
				positions.reserve(strtol10(cursor));
				break;
			}

		// optional number of faces
		case 72:
			{
				indices.reserve(strtol10(cursor));
				break;
			}

		// 8 specifies the layer
		case 8:
			{
				SetLayer(out);
				break;
			}
		}
	}
	if (!(flags & 64))	{
		DefaultLogger::get()->warn("DXF: Only polyface meshes are currently supported");
		return ret;
	}

	if (positions.size() < 3 || indices.size() < 3)	{
		DefaultLogger::get()->warn("DXF: Unable to parse POLYLINE element - not enough vertices");
		return ret;
	}

	// use a default layer if necessary
	if (!out) {
		SetDefaultLayer(out);
	}

	flags = (unsigned int)(out->vPositions.size()+indices.size());
	out->vPositions.reserve(flags);
	out->vColors.reserve(flags);

	// generate unique vertices
	for (std::vector<unsigned int>::const_iterator it = indices.begin(), end = indices.end();it != end; ++it)	{
		unsigned int idx = *it;
		if (idx > positions.size() || !idx)	{
			DefaultLogger::get()->error("DXF: Polyface mesh index os out of range");
			idx = (unsigned int) positions.size();
		}
		out->vPositions.push_back(positions[idx-1]); // indices are one-based.
		out->vColors.push_back(colors[idx-1]); // indices are one-based.
	}

	return ret;
}

// ------------------------------------------------------------------------------------------------
bool DXFImporter::ParsePolyLineVertex(aiVector3D& out,aiColor4D& clr, unsigned int* outIdx)
{
	bool ret = false;
	while (GetNextToken())	{
		switch (groupCode)
		{
		case 0: ret = true;
			break;

		// todo - handle the correct layer for the vertex. At the moment it is assumed that all vertices of 
		// a polyline are placed on the same global layer.

		// x position of the first corner
		case 10: out.x = fast_atof(cursor);break;

		// y position of the first corner
		case 20: out.y = -fast_atof(cursor);break;

		// z position of the first corner
		case 30: out.z = fast_atof(cursor);break;

		// POLYFACE vertex indices
		case 71: outIdx[0] = strtol10(cursor);break;
		case 72: outIdx[1] = strtol10(cursor);break;
		case 73: outIdx[2] = strtol10(cursor);break;
	//	case 74: outIdx[3] = strtol10(cursor);break;

		// color
		case 62: clr = g_aclrDxfIndexColors[strtol10(cursor) % AI_DXF_NUM_INDEX_COLORS]; break;
		};
		if (ret) {
			break;
		}
	}
	return ret;
}

// ------------------------------------------------------------------------------------------------
bool DXFImporter::Parse3DFace()
{
	bool ret = false;
	LayerInfo* out = NULL;

	aiVector3D vip[4]; // -- vectors are initialized to zero
	aiColor4D  clr(g_clrInvalid);

	// this is also used for for parsing line entities
	bool bThird = false;

	while (GetNextToken())	{
		switch (groupCode)	{
		case 0: 
			ret = true;
			break;

		// 8 specifies the layer
		case 8:	{
				SetLayer(out);
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
		case 12: vip[2].x = fast_atof(cursor);
			bThird = true;break;

		// y position of the third corner
		case 22: vip[2].y = -fast_atof(cursor);
			bThird = true;break;

		// z position of the third corner
		case 32: vip[2].z = fast_atof(cursor);
			bThird = true;break;

		// x position of the fourth corner
		case 13: vip[3].x = fast_atof(cursor);
			bThird = true;break;

		// y position of the fourth corner
		case 23: vip[3].y = -fast_atof(cursor);
			bThird = true;break;

		// z position of the fourth corner
		case 33: vip[3].z = fast_atof(cursor);
			bThird = true;break;

		// color
		case 62: clr = g_aclrDxfIndexColors[strtol10(cursor) % AI_DXF_NUM_INDEX_COLORS]; break;
		};
		if (ret)
			break;
	}

	if (!bThird)
		vip[2] = vip[1];

	// use a default layer if necessary
	if (!out) {
		SetDefaultLayer(out);
	}
	// add the faces to the face list for this layer
	out->vPositions.push_back(vip[0]);
	out->vPositions.push_back(vip[1]);
	out->vPositions.push_back(vip[2]);
	out->vPositions.push_back(vip[3]); // might be equal to the third

	for (unsigned int i = 0; i < 4;++i)
		out->vColors.push_back(clr);
	return ret;
}

#endif // !! ASSIMP_BUILD_NO_DXF_IMPORTER

