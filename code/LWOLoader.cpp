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

/** @file Implementation of the LWO importer class */

// internal headers
#include "LWOLoader.h"
#include "MaterialSystem.h"
#include "StringComparison.h"
#include "ByteSwap.h"

// public assimp headers
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "../include/DefaultLogger.h"
#include "../include/assimp.hpp"

// boost headers
#include <boost/scoped_ptr.hpp>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
LWOImporter::LWOImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
LWOImporter::~LWOImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool LWOImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)return false;
	std::string extension = pFile.substr( pos);

	if (extension.length() < 4)return false;
	if (extension[0] != '.')return false;

	if (extension[1] != 'l' && extension[1] != 'L')return false;
	if (extension[2] != 'w' && extension[2] != 'W')return false;
	if (extension[3] != 'o' && extension[3] != 'O')return false;

	return true;
}
// ------------------------------------------------------------------------------------------------
// Setup configuration properties
void LWOImporter::SetupProperties(const Importer* pImp)
{
	this->configGradientResX = pImp->GetProperty(AI_CONFIG_IMPORT_LWO_GRADIENT_RESX,512);
	this->configGradientResY = pImp->GetProperty(AI_CONFIG_IMPORT_LWO_GRADIENT_RESY,512);
}
// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void LWOImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, 
	IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, "rb"));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open LWO file " + pFile + ".");

	if((this->fileSize = (unsigned int)file->FileSize()) < 12)
		throw new ImportErrorException("LWO: The file is too small to contain the IFF header");

	// allocate storage and copy the contents of the file to a memory buffer
	std::vector< uint8_t > mBuffer(fileSize);
	file->Read( &mBuffer[0], 1, fileSize);
	this->pScene = pScene;

	// determine the type of the file
	uint32_t fileType;
	const char* sz = IFF::ReadHeader(&mBuffer[0],fileType);
	if (sz)throw new ImportErrorException(sz);

	mFileBuffer = &mBuffer[0] + 12;
	fileSize -= 12;

	// create temporary storage on the stack but store pointers to it in the class 
	// instance. Therefore everything will be destructed properly if an exception 
	// is thrown and we needn't take care of that.
	LayerList _mLayers;
	mLayers = &_mLayers;
	TagList _mTags;				
	mTags = &_mTags;
	TagMappingTable _mMapping;	
	mMapping = &_mMapping;
	SurfaceList _mSurfaces;		
	mSurfaces = &_mSurfaces;

	// allocate a default layer
	mLayers->push_back(Layer());
	mCurLayer = &mLayers->back();
	mCurLayer->mName = "<LWODefault>";

	// old lightwave file format (prior to v6)
	if (AI_LWO_FOURCC_LWOB == fileType)
	{
		mIsLWO2 = false;
		this->LoadLWOBFile();
	}

	// new lightwave format
	else if (AI_LWO_FOURCC_LWO2 == fileType)
	{
		throw new ImportErrorException("LWO2 is under development and currently disabled.");
		mIsLWO2 = true;
		this->LoadLWO2File();
	}

	// we don't know this format
	else 
	{
		char szBuff[5];
		szBuff[0] = (char)(fileType >> 24u);
		szBuff[1] = (char)(fileType >> 16u);
		szBuff[2] = (char)(fileType >> 8u);
		szBuff[3] = (char)(fileType);
		throw new ImportErrorException(std::string("Unknown LWO sub format: ") + szBuff);
	}
	ResolveTags();

	// now process all layers and build meshes and nodes
	std::vector<aiMesh*> apcMeshes;
	std::vector<aiNode*> apcNodes;
	apcNodes.reserve(mLayers->size());
	apcMeshes.reserve(mLayers->size()*std::min((mSurfaces->size()/2u), 1u));

	// the RemoveRedundantMaterials step will clean this up later
	pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials = mSurfaces->size()];
	for (unsigned int mat = 0; mat < pScene->mNumMaterials;++mat)
	{
		MaterialHelper* pcMat = new MaterialHelper();
		pScene->mMaterials[mat] = pcMat;
		ConvertMaterial((*mSurfaces)[mat],pcMat);
	}

	unsigned int iDefaultSurface = 0xffffffff; // index of the default surface
	for (LayerList::const_iterator lit = mLayers->begin(), lend = mLayers->end();
		lit != lend;++lit)
	{
		const LWO::Layer& layer = *lit;

		// I don't know whether there could be dummy layers, but it would be possible
		const unsigned int meshStart = apcMeshes.size();
		if (!layer.mFaces.empty() && !layer.mTempPoints.empty())
		{
			// now sort all faces by the surfaces assigned to them
			typedef std::vector<unsigned int> SortedRep;
			std::vector<SortedRep> pSorted(mSurfaces->size()+1);

			unsigned int i = 0;
			for (FaceList::const_iterator it = layer.mFaces.begin(), end = layer.mFaces.end();
				it != end;++it,++i)
			{
				unsigned int idx = (*it).surfaceIndex;
				if (idx >= mTags->size())
				{
					DefaultLogger::get()->warn("LWO: Invalid face surface index");
					idx = mTags->size()-1;
				}
				if(0xffffffff == (idx = _mMapping[idx]))
				{
					if (0xffffffff == iDefaultSurface)
					{
						iDefaultSurface = mSurfaces->size();
						mSurfaces->push_back(LWO::Surface());
						LWO::Surface& surf = mSurfaces->back();
						surf.mColor.r = surf.mColor.g = surf.mColor.b = 0.6f; 
					}
					idx = iDefaultSurface;
				}
				pSorted[idx].push_back(i);
			}
			if (0xffffffff == iDefaultSurface)pSorted.erase(pSorted.end()-1);

			// now generate output meshes
			for (unsigned int p = 0; p < mSurfaces->size();++p)
				if (!pSorted[p].empty())pScene->mNumMeshes++;

			if (!pScene->mNumMeshes)
				throw new ImportErrorException("LWO: There are no meshes");

			pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
			for (unsigned int p = 0,i = 0;i < mSurfaces->size();++i)
			{
				SortedRep& sorted = pSorted[i];
				if (sorted.empty())continue;

				// generate the mesh 
				aiMesh* mesh = new aiMesh();
				apcMeshes.push_back(mesh);
				mesh->mNumFaces = sorted.size();

				for (SortedRep::const_iterator it = sorted.begin(), end = sorted.end();
					it != end;++it)
				{
					mesh->mNumVertices += layer.mFaces[*it].mNumIndices;
				}

				aiVector3D* pv = mesh->mVertices = new aiVector3D[mesh->mNumVertices];
				aiFace* pf = mesh->mFaces = new aiFace[mesh->mNumFaces];
				mesh->mMaterialIndex = i;

				// now convert all faces
				unsigned int vert = 0;
				for (SortedRep::const_iterator it = sorted.begin(), end = sorted.end();
					it != end;++it)
				{
					const LWO::Face& face = layer.mFaces[*it];

					// copy all vertices
					for (unsigned int q = 0; q  < face.mNumIndices;++q)
					{
						*pv++ = layer.mTempPoints[face.mIndices[q]];
						face.mIndices[q] = vert++;
					}

					pf->mIndices = face.mIndices;
					pf->mNumIndices = face.mNumIndices;
					const_cast<unsigned int*>(face.mIndices) = NULL; // make sure it won't be deleted
					pf++;
				}
				++p;
			}
		}

		// generate nodes to render the mesh. Store the parent index
		// in the mParent member of the nodes
		aiNode* pcNode = new aiNode();
		apcNodes.push_back(pcNode);
		pcNode->mName.Set(layer.mName);
		pcNode->mParent = reinterpret_cast<aiNode*>(layer.mParent);
		pcNode->mNumMeshes = apcMeshes.size() - meshStart;
		pcNode->mMeshes = new unsigned int[pcNode->mNumMeshes];
		for (unsigned int p = 0; p < pcNode->mNumMeshes;++p)
			pcNode->mMeshes[p] = p + meshStart;
	}
	// generate the final node graph
	GenerateNodeGraph(apcNodes);

	// copy the meshes to the output structure
	if (apcMeshes.size()) // shouldn't occur, just to be sure we don't crash
	{
		pScene->mMeshes = new aiMesh*[ pScene->mNumMeshes = apcMeshes.size() ];
		::memcpy(pScene->mMeshes,&apcMeshes[0],pScene->mNumMeshes*sizeof(void*));
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::GenerateNodeGraph(std::vector<aiNode*>& apcNodes)
{
	// now generate the final nodegraph
	uint16_t curIndex = 0;
	while (curIndex < (uint16_t)apcNodes.size())
	{
		aiNode* node;
		uint16_t iCurParent = curIndex-1;
		node = curIndex ? apcNodes[iCurParent] : new aiNode("<dummy_root>");

		unsigned int numChilds = 0;
		for (unsigned int i = 0; i < apcNodes.size();++i)
		{
			if (i == iCurParent)continue;
			if ( reinterpret_cast<uint16_t>(apcNodes[i]->mParent) == iCurParent)++numChilds;
		}
		if (numChilds)
		{
			if (!pScene->mRootNode)
			{
				pScene->mRootNode = node;
			}
			node->mChildren = new aiNode* [ node->mNumChildren = numChilds ];
			for (unsigned int i = 0, p = 0; i < apcNodes.size();++i)
			{
				if (i == iCurParent)continue;
				uint16_t parent = reinterpret_cast<uint16_t>(apcNodes[i]->mParent);
				if (parent == iCurParent)
				{
					node->mChildren[p++] = apcNodes[i];
					apcNodes[i]->mParent = node;
					apcNodes[i] = NULL;
				}
			}
		}
		else if (!curIndex)delete node;
		++curIndex;
	}
	// remove a single root node
	// TODO: implement directly in the above loop, no need to deallocate here
	if (1 == pScene->mRootNode->mNumChildren)
	{
		aiNode* pc = pScene->mRootNode->mChildren[0];
		pc->mParent = pScene->mRootNode->mChildren[0] = NULL;
		delete pScene->mRootNode;
		pScene->mRootNode = pc;
	}

	// add unreferenced nodes to a dummy root
	unsigned int m = 0;
	for (std::vector<aiNode*>::iterator it = apcNodes.begin(), end = apcNodes.end();
		it != end;++it)
	{
		aiNode* p = *it;
		if (p)++m;
	}
	if (m)
	{
		aiNode* pc = new aiNode();
		pc->mName.Set("<dummy_root>");
		aiNode** cc = pc->mChildren = new aiNode*[ pc->mNumChildren = m+1 ];
		for (std::vector<aiNode*>::iterator it = apcNodes.begin(), end = apcNodes.end();
			it != end;++it)
		{
			aiNode* p = *it;
			if (p)*cc++ = p;
		}
		if (pScene->mRootNode)
		{
			*cc = pScene->mRootNode;
			pScene->mRootNode->mParent = pc;
		}
		else --pc->mNumChildren;
		pScene->mRootNode = pc;
	}
	if (!pScene->mRootNode)throw new ImportErrorException("LWO: Unable to build a valid node graph");
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::CountVertsAndFaces(unsigned int& verts, unsigned int& faces,
	LE_NCONST uint16_t*& cursor, const uint16_t* const end, unsigned int max)
{
	while (cursor < end && max--)
	{
		uint16_t numIndices = *cursor++;
		verts += numIndices;faces++;
		cursor += numIndices;
		int16_t surface = *cursor++;
		if (surface < 0)
		{
			// there are detail polygons
			numIndices = *cursor++;
			CountVertsAndFaces(verts,faces,cursor,end,numIndices);
		}
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::CopyFaceIndices(FaceList::iterator& it,
	LE_NCONST uint16_t*& cursor, 
	const uint16_t* const end,
	unsigned int max)
{
	while (cursor < end && max--)
	{
		LWO::Face& face = *it;++it;
		if(face.mNumIndices = *cursor++)
		{
			if (cursor + face.mNumIndices >= end)break;
			face.mIndices = new unsigned int[face.mNumIndices];
			for (unsigned int i = 0; i < face.mNumIndices;++i)
			{
				unsigned int & mi = face.mIndices[i] = *cursor++;
				if (mi > mCurLayer->mTempPoints.size())
				{
					DefaultLogger::get()->warn("LWO: face index is out of range");
					mi = mCurLayer->mTempPoints.size()-1;
				}
			}
		}
		else DefaultLogger::get()->warn("LWO: Face has 0 indices");
		int16_t surface = *cursor++;
		if (surface < 0)
		{
			surface = -surface;

			// there are detail polygons
			uint16_t numPolygons = *cursor++;
			if (cursor < end)CopyFaceIndices(it,cursor,end,numPolygons);
		}
		face.surfaceIndex = surface-1;
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::ResolveTags()
{
	mMapping->resize(mTags->size(),0xffffffff);
	for (unsigned int a = 0; a  < mTags->size();++a)
	{
		for (unsigned int i = 0; i < mSurfaces->size();++i)
		{
			const std::string& c = (*mTags)[a];
			const std::string& d = (*mSurfaces)[i].mName;
			if (!ASSIMP_stricmp(c,d))
			{
				(*mMapping)[a] = i;
				break;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::ParseString(std::string& out,unsigned int max)
{
	unsigned int iCursor = 0;
	const char* in = (const char*)mFileBuffer,*sz = in;
	while (*in)
	{
		if (++iCursor > max)
		{
			DefaultLogger::get()->warn("LWOB: Invalid file, string is is too long");
			break;
		}
		++in;
	}
	unsigned int len = (unsigned int) (in-sz);
	out = std::string(sz,len);
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::AdjustTexturePath(std::string& out)
{
	if (::strstr(out.c_str(), "(sequence)"))
	{
		// remove the (sequence) and append 000
		DefaultLogger::get()->info("LWO: Sequence of animated texture found. It will be ignored");
		out = out.substr(0,out.length()-10) + "000";
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOTags(unsigned int size)
{
	const char* szCur = (const char*)mFileBuffer, *szLast = szCur;
	const char* const szEnd = szLast+size;
	while (szCur < szEnd)
	{
		if (!(*szCur))
		{
			const unsigned int len = (unsigned int)(szCur-szLast);
			mTags->push_back(std::string(szLast,len));
			szCur += len & 1;
			szLast = szCur;
		}
		szCur++;
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOPoints(unsigned int length)
{
	mCurLayer->mTempPoints.resize( length / 12 );

	// perform endianess conversions
#ifndef AI_BUILD_BIG_ENDIAN
	for (unsigned int i = 0; i < length>>2;++i)
		ByteSwap::Swap4( mFileBuffer + (i << 2));
#endif
	::memcpy(&mCurLayer->mTempPoints[0],mFileBuffer,length);
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOPolygons(unsigned int length)
{
	// first find out how many faces and vertices we'll finally need
	LE_NCONST uint16_t* const end	= (LE_NCONST uint16_t*)(mFileBuffer+length);
	LE_NCONST uint16_t* cursor		= (LE_NCONST uint16_t*)mFileBuffer;

	// perform endianess conversions
#ifndef AI_BUILD_BIG_ENDIAN
	while (cursor < end)ByteSwap::Swap2(cursor++);
	cursor = (LE_NCONST uint16_t*)mFileBuffer;
#endif

	unsigned int iNumFaces = 0,iNumVertices = 0;
	CountVertsAndFaces(iNumVertices,iNumFaces,cursor,end);

	// allocate the output array and copy face indices
	if (iNumFaces)
	{
		cursor = (LE_NCONST uint16_t*)mFileBuffer;
		// this->mTempPoints->resize(iNumVertices);
		mCurLayer->mFaces.resize(iNumFaces);
		FaceList::iterator it = mCurLayer->mFaces.begin();
		CopyFaceIndices(it,cursor,end);
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOBFile()
{
	LE_NCONST uint8_t* const end = mFileBuffer + fileSize;
	while (true)
	{
		if (mFileBuffer + sizeof(IFF::ChunkHeader) > end)break;
		LE_NCONST IFF::ChunkHeader* const head = (LE_NCONST IFF::ChunkHeader*)mFileBuffer;
		AI_LSWAP4(head->length);
		AI_LSWAP4(head->type);
		mFileBuffer += sizeof(IFF::ChunkHeader);
		if (mFileBuffer + head->length > end)
		{
			throw new ImportErrorException("LWOB: Invalid file, the size attribute of "
				"a chunk points behind the end of the file");
			break;
		}
		LE_NCONST uint8_t* const next = mFileBuffer+head->length;
		switch (head->type)
		{
			// vertex list
		case AI_LWO_PNTS:
			{
				if (!mCurLayer->mTempPoints.empty())
					DefaultLogger::get()->warn("LWO: PNTS chunk encountered twice");
				else LoadLWOPoints(head->length);
				break;
			}
			// face list
		case AI_LWO_POLS:
			{
				if (!mCurLayer->mFaces.empty())
					DefaultLogger::get()->warn("LWO: POLS chunk encountered twice");
				else LoadLWOPolygons(head->length);
				break;
			}
			// list of tags
		case AI_LWO_SRFS:
			{
				if (!mTags->empty())
					DefaultLogger::get()->warn("LWO: SRFS chunk encountered twice");
				else LoadLWOTags(head->length);
				break;
			}

			// surface chunk
		case AI_LWO_SURF:
			{
				if (!mSurfaces->empty())
					DefaultLogger::get()->warn("LWO: SURF chunk encountered twice");
				else LoadLWOBSurface(head->length);
				break;
			}
		}
		mFileBuffer = next;
	}
}
// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2File()
{
	LE_NCONST uint8_t* const end = mFileBuffer + fileSize;
	while (true)
	{
		if (mFileBuffer + sizeof(IFF::ChunkHeader) > end)break;
		LE_NCONST IFF::ChunkHeader* const head = (LE_NCONST IFF::ChunkHeader*)mFileBuffer;
		AI_LSWAP4(head->length);
		AI_LSWAP4(head->type);
		mFileBuffer += sizeof(IFF::ChunkHeader);
		if (mFileBuffer + head->length > end)
		{
			throw new ImportErrorException("LWOB: Invalid file, the size attribute of "
				"a chunk points behind the end of the file");
			break;
		}
		LE_NCONST uint8_t* const next = mFileBuffer+head->length;
		unsigned int iUnnamed = 0;
		switch (head->type)
		{
			// new layer
		case AI_LWO_LAYR:
			{
				// add a new layer to the list ....
				mLayers->push_back ( LWO::Layer() );
				LWO::Layer& layer = mLayers->back();
				mCurLayer = &layer;

				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,LAYR,16);

				// and parse its properties
				mFileBuffer += 16;
				ParseString(layer.mName,head->length-16);

				// if the name is empty, generate a default name
				if (layer.mName.empty())
				{
					char buffer[128]; // should be sufficiently large
					::sprintf(buffer,"Layer_%i", iUnnamed++);
					layer.mName = buffer;
				}

				if (mFileBuffer + 2 <= next)
					layer.mParent = *((uint16_t*)mFileBuffer);

				break;
			}

			// vertex list
		case AI_LWO_PNTS:
			{
				if (!mCurLayer->mTempPoints.empty())
					DefaultLogger::get()->warn("LWO: PNTS chunk encountered twice");
				else LoadLWOPoints(head->length);
				break;
			}
			// face list
		case AI_LWO_POLS:
			{
				if (!mCurLayer->mFaces.empty())
					DefaultLogger::get()->warn("LWO: POLS chunk encountered twice");
				else LoadLWOPolygons(head->length);
				break;
			}
			// list of tags
		case AI_LWO_SRFS:
			{
				if (!mTags->empty())
					DefaultLogger::get()->warn("LWO: SRFS chunk encountered twice");
				else LoadLWOTags(head->length);
				break;
			}

			// surface chunk
		case AI_LWO_SURF:
			{
				if (!mSurfaces->empty())
					DefaultLogger::get()->warn("LWO: SURF chunk encountered twice");
				else LoadLWOBSurface(head->length);
				break;
			}
		}
		mFileBuffer = next;
	}
}