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

#include "AssimpPCH.h"


// internal headers
#include "LWOLoader.h"
#include "MaterialSystem.h"
#include "StringComparison.h"
#include "SGSpatialSort.h"
#include "ByteSwap.h"
#include "ProcessHelper.h"

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

	return ! (extension[1] != 'l' && extension[1] != 'L' ||
			  extension[2] != 'w' && extension[2] != 'W' &&
			  extension[2] != 'x' && extension[2] != 'X' ||
			  extension[3] != 'o' && extension[3] != 'O');
}

// ------------------------------------------------------------------------------------------------
// Setup configuration properties
void LWOImporter::SetupProperties(const Importer* pImp)
{
	configSpeedFlag  = ( 0 != pImp->GetPropertyInteger(AI_CONFIG_FAVOUR_SPEED,0) ? true : false);
	configLayerIndex = pImp->GetPropertyInteger (AI_CONFIG_IMPORT_LWO_ONE_LAYER_ONLY,0xffffffff); 
	configLayerName  = pImp->GetPropertyString  (AI_CONFIG_IMPORT_LWO_ONE_LAYER_ONLY,"");
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
	hasNamedLayer = false;

	// create temporary storage on the stack but store pointers to it in the class 
	// instance. Therefore everything will be destructed properly if an exception 
	// is thrown and we needn't take care of that.
	LayerList		_mLayers;
	SurfaceList		_mSurfaces;		
	TagList			_mTags;		
	TagMappingTable _mMapping;	

	mLayers			= &_mLayers;
	mTags			= &_mTags;
	mMapping		= &_mMapping;
	mSurfaces		= &_mSurfaces;

	// Allocate a default layer (layer indices are 1-based from now)
	mLayers->push_back(Layer());
	mCurLayer = &mLayers->back();
	mCurLayer->mName = "<LWODefault>";

	// old lightwave file format (prior to v6)
	if (AI_LWO_FOURCC_LWOB == fileType)
	{
		DefaultLogger::get()->info("LWO file format: LWOB (<= LightWave 5.5)");

		mIsLWO2 = false;
		LoadLWOBFile();
	}

	// New lightwave format
	else if (AI_LWO_FOURCC_LWO2 == fileType)
	{
		DefaultLogger::get()->info("LWO file format: LWO2 (>= LightWave 6)");
	}
	// MODO file format
	else if (AI_LWO_FOURCC_LXOB == fileType)
	{
		DefaultLogger::get()->info("LWO file format: LXOB (Modo)");
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

	if (AI_LWO_FOURCC_LWOB != fileType)
	{
		mIsLWO2 = true;
		LoadLWO2File();

		// The newer lightwave format allows the user to configure the
		// loader that just one layer is used. If this is the case
		// we need to check now whether the requested layer has been found.
		if (0xffffffff != configLayerIndex && configLayerIndex > mLayers->size())
			throw new ImportErrorException("LWO2: The requested layer was not found");

		if (configLayerName.length() && !hasNamedLayer)
		{
			throw new ImportErrorException("LWO2: Unable to find the requested layer: " 
				+ configLayerName);
		}
	}

	// now, as we have loaded all data, we can resolve cross-referenced tags and clips
	ResolveTags();
	ResolveClips();

	// now process all layers and build meshes and nodes
	std::vector<aiMesh*> apcMeshes;
	std::vector<aiNode*> apcNodes;
	apcNodes. reserve(mLayers->size());
	apcMeshes.reserve(mLayers->size()*std::min(((unsigned int)mSurfaces->size()/2u), 1u));

	unsigned int iDefaultSurface = 0xffffffff; // index of the default surface
	for (LayerList::iterator lit = mLayers->begin(), lend = mLayers->end();
		lit != lend;++lit)
	{
		LWO::Layer& layer = *lit;
		if (layer.skip)continue;

		// I don't know whether there could be dummy layers, but it would be possible
		const unsigned int meshStart = (unsigned int)apcMeshes.size();
		if (!layer.mFaces.empty() && !layer.mTempPoints.empty())
		{
			// now sort all faces by the surfaces assigned to them
			typedef std::vector<unsigned int> SortedRep;
			std::vector<SortedRep> pSorted(mSurfaces->size()+1);

			unsigned int i = 0;
			for (FaceList::iterator it = layer.mFaces.begin(), end = layer.mFaces.end();
				it != end;++it,++i)
			{
				unsigned int idx = (*it).surfaceIndex;
				if (idx >= mTags->size())
				{
					DefaultLogger::get()->warn("LWO: Invalid face surface index");
					idx = 0xffffffff;
				}
				if(0xffffffff == idx || 0xffffffff == (idx = _mMapping[idx]))
				{
					if (0xffffffff == iDefaultSurface)
					{
						iDefaultSurface = (unsigned int)mSurfaces->size();
						mSurfaces->push_back(LWO::Surface());
						LWO::Surface& surf = mSurfaces->back();
						surf.mColor.r = surf.mColor.g = surf.mColor.b = 0.6f; 
					}
					idx = iDefaultSurface;
				}
				pSorted[idx].push_back(i);
			}
			if (0xffffffff == iDefaultSurface)pSorted.erase(pSorted.end()-1);
			for (unsigned int p = 0,i = 0;i < mSurfaces->size();++i)
			{
				SortedRep& sorted = pSorted[i];
				if (sorted.empty())continue;

				// generate the mesh 
				aiMesh* mesh = new aiMesh();
				apcMeshes.push_back(mesh);
				mesh->mNumFaces = (unsigned int)sorted.size();

				// count the number of vertices
				SortedRep::const_iterator it = sorted.begin(), end = sorted.end();
				for (;it != end;++it)
				{
					mesh->mNumVertices += layer.mFaces[*it].mNumIndices;
				}

				aiVector3D *nrm = NULL, * pv = mesh->mVertices = new aiVector3D[mesh->mNumVertices];
				aiFace* pf = mesh->mFaces = new aiFace[mesh->mNumFaces];
				mesh->mMaterialIndex = i;

				// find out which vertex color channels and which texture coordinate
				// channels are really required by the material attached to this mesh
				unsigned int vUVChannelIndices[AI_MAX_NUMBER_OF_TEXTURECOORDS];
				unsigned int vVColorIndices[AI_MAX_NUMBER_OF_COLOR_SETS];

#if _DEBUG
				for (unsigned int mui = 0; mui < AI_MAX_NUMBER_OF_TEXTURECOORDS;++mui )
					vUVChannelIndices[mui] = 0xffffffff;
				for (unsigned int mui = 0; mui < AI_MAX_NUMBER_OF_COLOR_SETS;++mui )
					vVColorIndices[mui] = 0xffffffff;
#endif

				FindUVChannels(_mSurfaces[i],layer,vUVChannelIndices);
				FindVCChannels(_mSurfaces[i],layer,vVColorIndices);

				// allocate storage for UV and CV channels
				aiVector3D* pvUV[AI_MAX_NUMBER_OF_TEXTURECOORDS];
				for (unsigned int mui = 0; mui < AI_MAX_NUMBER_OF_TEXTURECOORDS;++mui )
				{
					if (0xffffffff == vUVChannelIndices[mui])break;
					pvUV[mui] = mesh->mTextureCoords[mui] = new aiVector3D[mesh->mNumVertices];

					// LightWave doesn't support more than 2 UV components
					// so we can directly setup this value
					mesh->mNumUVComponents[0] = 2;
				}

				if (layer.mNormals.name.length())
					nrm = mesh->mNormals = new aiVector3D[mesh->mNumVertices];
		
				aiColor4D* pvVC[AI_MAX_NUMBER_OF_COLOR_SETS];
				for (unsigned int mui = 0; mui < AI_MAX_NUMBER_OF_COLOR_SETS;++mui)	
				{
					if (0xffffffff == vVColorIndices[mui])break;
					pvVC[mui] = mesh->mColors[mui] = new aiColor4D[mesh->mNumVertices];
				}

				// we would not need this extra array, but the code is much cleaner if we use it
				// FIX: we can use the referrer ID array here. invalidate its contents
				// before we resize it to avoid a unnecessary memcpy 
				std::vector<unsigned int>& smoothingGroups = layer.mPointReferrers;
				smoothingGroups.erase (smoothingGroups.begin(),smoothingGroups.end());
				smoothingGroups.resize(mesh->mNumFaces,0);

				// now convert all faces
				unsigned int vert = 0;
				std::vector<unsigned int>::iterator outIt = smoothingGroups.begin();
				for (it = sorted.begin(); it != end;++it,++outIt)
				{
					const LWO::Face& face = layer.mFaces[*it];
					*outIt = face.smoothGroup;

					// copy all vertices
					for (unsigned int q = 0; q  < face.mNumIndices;++q)
					{
						register unsigned int idx = face.mIndices[q];
						*pv = layer.mTempPoints[idx] + layer.mPivot;
						pv->z *= -1.0f; // DX to OGL
						pv++;

						// process UV coordinates
						for (unsigned int w = 0; w < AI_MAX_NUMBER_OF_TEXTURECOORDS;++w)	
						{
							if (0xffffffff == vUVChannelIndices[w])break;
							aiVector3D*& pp = pvUV[w];
							const aiVector2D& src = ((aiVector2D*)&layer.mUVChannels[vUVChannelIndices[w]].rawData[0])[idx];
							pp->x = src.x;
							pp->y = src.y; 
							pp++;
						}

						// process normals (MODO extension)
						if (nrm)
						{
							*nrm++ = ((aiVector3D*)&layer.mNormals.rawData[0])[idx];
						}

						// process vertex colors
						for (unsigned int w = 0; w < AI_MAX_NUMBER_OF_COLOR_SETS;++w)	
						{
							if (0xffffffff == vVColorIndices[w])break;
							*pvVC[w] = ((aiColor4D*)&layer.mVColorChannels[vVColorIndices[w]].rawData[0])[idx];

							// If a RGB color map is explicitly requested delete the
							// alpha channel - it could theoretically be != 1.
							if(_mSurfaces[i].mVCMapType == AI_LWO_RGB)
								pvVC[w]->a = 1.f;

							pvVC[w]++;
						}

#if 0
						// process vertex weights - not yet supported
						for (unsigned int w = 0; w < layer.mWeightChannels.size();++w)
						{
						}
#endif
						face.mIndices[q] = vert + (face.mNumIndices-q-1);
					}
					vert += face.mNumIndices;

					pf->mIndices = face.mIndices;
					pf->mNumIndices = face.mNumIndices;
					unsigned int** p = (unsigned int**)&face.mIndices;*p = NULL; // make sure it won't be deleted
					pf++;
				}

				if (!mesh->mNormals)
				{
					// Compute normal vectors for the mesh - we can't use our GenSmoothNormal-
					// Step here since it wouldn't handle smoothing groups correctly for LWO.
					// So we use a separate implementation.
					ComputeNormals(mesh,smoothingGroups,_mSurfaces[i]);
				}
				else DefaultLogger::get()->debug("LWO2: No need to compute normals, they're already there");
				++p;
			}
		}

		// Generate nodes to render the mesh. Store the parent index
		// in the mParent member of the nodes
		aiNode* pcNode = new aiNode();
		apcNodes.push_back(pcNode);
		pcNode->mName.Set(layer.mName);
		pcNode->mParent = (aiNode*)(uintptr_t)(layer.mParent);
		pcNode->mNumMeshes = (unsigned int)apcMeshes.size() - meshStart;
		pcNode->mMeshes = new unsigned int[pcNode->mNumMeshes];
		for (unsigned int p = 0; p < pcNode->mNumMeshes;++p)
			pcNode->mMeshes[p] = p + meshStart;
	}

	if (apcNodes.empty() || apcMeshes.empty())
		throw new ImportErrorException("LWO: No meshes loaded");

	// The RemoveRedundantMaterials step will clean this up later
	pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials = (unsigned int)mSurfaces->size()];
	for (unsigned int mat = 0; mat < pScene->mNumMaterials;++mat)
	{
		MaterialHelper* pcMat = new MaterialHelper();
		pScene->mMaterials[mat] = pcMat;
		ConvertMaterial((*mSurfaces)[mat],pcMat);
	}

	// copy the meshes to the output structure
	if (apcMeshes.size()) // shouldn't occur, just to be sure we don't crash
	{
		pScene->mMeshes = new aiMesh*[ pScene->mNumMeshes = (unsigned int)apcMeshes.size() ];
		::memcpy(pScene->mMeshes,&apcMeshes[0],pScene->mNumMeshes*sizeof(void*));
	}

	// generate the final node graph
	GenerateNodeGraph(apcNodes);
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::ComputeNormals(aiMesh* mesh, const std::vector<unsigned int>& smoothingGroups,
	const LWO::Surface& surface)
{
	// allocate output storage
	mesh->mNormals = new aiVector3D[mesh->mNumVertices];

	// First generate per-face normals
	aiVector3D* out;
	std::vector<aiVector3D> faceNormals;

	if (!surface.mMaximumSmoothAngle)
		out = mesh->mNormals;
	else
	{
		faceNormals.resize(mesh->mNumVertices);
		out = &faceNormals[0];
	}

	aiFace* begin = mesh->mFaces, *const end = mesh->mFaces+mesh->mNumFaces;
	for (; begin != end; ++begin)
	{
		aiFace& face = *begin;

		// LWO doc: "the normal is defined as the cross product of the first and last edges"
		aiVector3D* pV1 = mesh->mVertices + face.mIndices[0];
		aiVector3D* pV2 = mesh->mVertices + face.mIndices[1];
		aiVector3D* pV3 = mesh->mVertices + face.mIndices[face.mNumIndices-1];

		aiVector3D vNor = ((*pV2 - *pV1) ^ (*pV3 - *pV1)).Normalize();
		for (unsigned int i = 0; i < face.mNumIndices;++i)
			out[face.mIndices[i]] = vNor;
	}
	if (!surface.mMaximumSmoothAngle)return;
	const float posEpsilon = ComputePositionEpsilon(mesh);
	
	// now generate the spatial sort tree
	SGSpatialSort sSort;
	std::vector<unsigned int>::const_iterator it = smoothingGroups.begin();
	for( begin =  mesh->mFaces; begin != end; ++begin, ++it)
	{
		aiFace& face = *begin;
		for (unsigned int i = 0; i < face.mNumIndices;++i)
		{
			register unsigned int tt = face.mIndices[i];
			sSort.Add(mesh->mVertices[tt],tt,*it);
		}
	}
	// sort everything - this takes O(nlogn) time
	sSort.Prepare();
	std::vector<unsigned int> poResult;
	poResult.reserve(20);

	// generate vertex normals. We have O(logn) for the binary lookup, which we need
	// for n elements, thus the EXPECTED complexity is O(nlogn)
	if (surface.mMaximumSmoothAngle < 3.f && !configSpeedFlag)
	{
		const float fLimit = cos(surface.mMaximumSmoothAngle);

		for( begin =  mesh->mFaces, it = smoothingGroups.begin(); begin != end; ++begin, ++it)
		{
			register unsigned int sg = *it;

			aiFace& face = *begin;
			unsigned int* beginIdx = face.mIndices, *const endIdx = face.mIndices+face.mNumIndices;
			for (; beginIdx != endIdx; ++beginIdx)
			{
				register unsigned int idx = *beginIdx;
				sSort.FindPositions(mesh->mVertices[idx],sg,posEpsilon,poResult,true);

				std::vector<unsigned int>::const_iterator a, end = poResult.end();

				aiVector3D vNormals;
				for (a =  poResult.begin();a != end;++a)
				{
					const aiVector3D& v = faceNormals[*a];
					if (v * faceNormals[idx] < fLimit)continue;
					vNormals += v;
				}
				vNormals.Normalize();
				mesh->mNormals[idx] = vNormals;
			}
		}
	}
	else // faster code path in case there is no smooth angle
	{
		std::vector<bool> vertexDone(mesh->mNumVertices,false);

		for( begin =  mesh->mFaces, it = smoothingGroups.begin(); begin != end; ++begin, ++it)
		{
			register unsigned int sg = *it;

			aiFace& face = *begin;
			unsigned int* beginIdx = face.mIndices, *const endIdx = face.mIndices+face.mNumIndices;
			for (; beginIdx != endIdx; ++beginIdx)
			{
				register unsigned int idx = *beginIdx;

				if (vertexDone[idx])continue;
				sSort.FindPositions(mesh->mVertices[idx],sg,posEpsilon,poResult,true);

				std::vector<unsigned int>::const_iterator a, end = poResult.end();

				aiVector3D vNormals;
				for (a =  poResult.begin();a != end;++a)
				{
					const aiVector3D& v = faceNormals[*a];
					vNormals += v;
				}
				vNormals.Normalize();
				for (a =  poResult.begin();a != end;++a)
				{
					mesh->mNormals[*a] = vNormals;
					vertexDone[*a] = true;
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::AddChildren(aiNode* node, uintptr_t parent, std::vector<aiNode*>& apcNodes)
{
	for (uintptr_t i  = 0; i < (uintptr_t)apcNodes.size();++i)
	{
		if (i == parent)continue;
		if (apcNodes[i] && (uintptr_t)apcNodes[i]->mParent == parent)++node->mNumChildren;
	}

	if (node->mNumChildren)
	{
		node->mChildren = new aiNode* [ node->mNumChildren ];
		for (uintptr_t i = 0, p = 0; i < (uintptr_t)apcNodes.size();++i)
		{
			if (i == parent)continue;

			if (apcNodes[i] && parent == (uintptr_t)(apcNodes[i]->mParent))
			{
				node->mChildren[p++] = apcNodes[i];
				apcNodes[i]->mParent = node;

				// recursively add more children
				AddChildren(apcNodes[i],i,apcNodes);
				apcNodes[i] = NULL;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::GenerateNodeGraph(std::vector<aiNode*>& apcNodes)
{
	// now generate the final nodegraph - generate a root node
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mName.Set("<LWORoot>");
	AddChildren(pScene->mRootNode,0,apcNodes);

	unsigned int extra = 0;
	for (unsigned int i = 0; i < apcNodes.size();++i)
		if (apcNodes[i] && apcNodes[i]->mNumMeshes)++extra;

	if (extra)
	{
		// we need to add extra nodes to the root
		const unsigned int newSize = extra + pScene->mRootNode->mNumChildren;
		aiNode** const apcNewNodes = new aiNode*[newSize];
		if((extra = pScene->mRootNode->mNumChildren))
			::memcpy(apcNewNodes,pScene->mRootNode->mChildren,extra*sizeof(void*));

		aiNode** cc = apcNewNodes+extra;
		for (unsigned int i = 0; i < apcNodes.size();++i)
		{
			if (apcNodes[i] && apcNodes[i]->mNumMeshes)
			{
				*cc++ = apcNodes[i];
				apcNodes[i]->mParent = pScene->mRootNode;

				// recursively add more children
				AddChildren(apcNodes[i],i,apcNodes);
				apcNodes[i] = NULL;
			}
		}
		delete[] pScene->mRootNode->mChildren;
		pScene->mRootNode->mChildren	= apcNewNodes;
		pScene->mRootNode->mNumChildren	= newSize;
	}
	if (!pScene->mRootNode->mNumChildren)throw new ImportErrorException("LWO: Unable to build a valid node graph");

	// remove a single root node
	// TODO: implement directly in the above loop, no need to deallocate here
	if (1 == pScene->mRootNode->mNumChildren)
	{
		aiNode* pc = pScene->mRootNode->mChildren[0];
		pc->mParent = pScene->mRootNode->mChildren[0] = NULL;
		delete pScene->mRootNode;
		pScene->mRootNode = pc;
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::ResolveTags()
{
	// --- this function is used for both LWO2 and LWOB
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
void LWOImporter::ResolveClips()
{
	for( unsigned int i = 0; i < mClips.size();++i)
	{
		Clip& clip = mClips[i];
		if (Clip::REF == clip.type)
		{
			if (clip.clipRef >= mClips.size())
			{
				DefaultLogger::get()->error("LWO2: Clip referrer index is out of range");
				clip.clipRef = 0;
			}
			Clip& dest = mClips[clip.clipRef];
			if (Clip::REF == dest.type)
			{
				DefaultLogger::get()->error("LWO2: Clip references another clip reference");
				clip.type = Clip::UNSUPPORTED;
			}
			else
			{
				clip.path = dest.path;
				clip.type = dest.type;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::AdjustTexturePath(std::string& out)
{
	// --- this function is used for both LWO2 and LWOB
	if (!mIsLWO2 && ::strstr(out.c_str(), "(sequence)"))
	{
		// remove the (sequence) and append 000
		DefaultLogger::get()->info("LWOB: Sequence of animated texture found. It will be ignored");
		out = out.substr(0,out.length()-10) + "000";
	}

	// format: drive:path/file - we need to insert a slash after the drive
	std::string::size_type n = out.find_first_of(':');
	if (std::string::npos != n)
	{
		out.insert(n+1,"/");
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWOTags(unsigned int size)
{
	// --- this function is used for both LWO2 and LWOB

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
	// --- this function is used for both LWO2 and LWOB but for
	// LWO2 we need to allocate 25% more storage - it could be we'll 
	// need to duplicate some points later.
	register unsigned int regularSize = (unsigned int)mCurLayer->mTempPoints.size() + length / 12;
	if (mIsLWO2)
	{
		mCurLayer->mTempPoints.reserve	( regularSize + (regularSize>>2u) );
		mCurLayer->mTempPoints.resize	( regularSize );

		// initialize all point referrers with the default values
		mCurLayer->mPointReferrers.reserve	( regularSize + (regularSize>>2u) );
		mCurLayer->mPointReferrers.resize	( regularSize, 0xffffffff );
	}
	else mCurLayer->mTempPoints.resize( regularSize );

	// perform endianess conversions
#ifndef AI_BUILD_BIG_ENDIAN
	for (unsigned int i = 0; i < length>>2;++i)
		ByteSwap::Swap4( mFileBuffer + (i << 2));
#endif
	::memcpy(&mCurLayer->mTempPoints[0],mFileBuffer,length);
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2Polygons(unsigned int length)
{
	LE_NCONST uint16_t* const end	= (LE_NCONST uint16_t*)(mFileBuffer+length);
	uint32_t type = GetU4();

	// Determine the type of the polygons
	switch (type)
	{
	case  AI_LWO_PTCH:
	case  AI_LWO_FACE:

		break;
	default:
		DefaultLogger::get()->warn("LWO2: Unsupported polygon type (PTCH and FACE are supported)");
	}

	// first find out how many faces and vertices we'll finally need
	uint16_t* cursor		= (uint16_t*)mFileBuffer;

	unsigned int iNumFaces = 0,iNumVertices = 0;
	CountVertsAndFacesLWO2(iNumVertices,iNumFaces,cursor,end);

	// allocate the output array and copy face indices
	if (iNumFaces)
	{
		cursor = (uint16_t*)mFileBuffer;

		mCurLayer->mFaces.resize(iNumFaces);
		FaceList::iterator it = mCurLayer->mFaces.begin();
		CopyFaceIndicesLWO2(it,cursor,end);
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::CountVertsAndFacesLWO2(unsigned int& verts, unsigned int& faces,
	uint16_t*& cursor, const uint16_t* const end, unsigned int max)
{
	while (cursor < end && max--)
	{
		AI_LSWAP2P(cursor);
		uint16_t numIndices = *cursor++;
		numIndices &= 0x03FF;
		verts += numIndices;++faces;

		for(uint16_t i = 0; i < numIndices; i++)
			ReadVSizedIntLWO2((uint8_t*&)cursor);
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::CopyFaceIndicesLWO2(FaceList::iterator& it,
	uint16_t*& cursor, 
	const uint16_t* const end)
{
	while (cursor < end)
	{
		LWO::Face& face = *it;++it;
		if((face.mNumIndices = (*cursor++) & 0x03FF)) // swapping has already been done
		{
			face.mIndices = new unsigned int[face.mNumIndices];
			for(unsigned int i = 0; i < face.mNumIndices; i++)
			{
				face.mIndices[i] = ReadVSizedIntLWO2((uint8_t*&)cursor) + mCurLayer->mPointIDXOfs;
				if(face.mIndices[i] > mCurLayer->mTempPoints.size())
				{
					DefaultLogger::get()->warn("LWO2: face index is out of range");
					face.mIndices[i] = (unsigned int)mCurLayer->mTempPoints.size()-1;
				}
			}
		}
		else DefaultLogger::get()->warn("LWO2: face has 0 indices");
	}
}


// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2PolygonTags(unsigned int length)
{
	LE_NCONST uint8_t* const end = mFileBuffer+length;

	AI_LWO_VALIDATE_CHUNK_LENGTH(length,PTAG,4);
	uint32_t type = GetU4();

	if (type != AI_LWO_SURF && type != AI_LWO_SMGP)
		return;

	while (mFileBuffer < end)
	{
		unsigned int i = ReadVSizedIntLWO2(mFileBuffer) + mCurLayer->mFaceIDXOfs;
		unsigned int j = GetU2();

		if (i >= mCurLayer->mFaces.size())
		{
			DefaultLogger::get()->warn("LWO2: face index in PTAG is out of range");
			continue;
		}

		switch (type)
		{
		case AI_LWO_SURF:
			mCurLayer->mFaces[i].surfaceIndex = j;
			break;
		case AI_LWO_SMGP:
			mCurLayer->mFaces[i].smoothGroup = j;
			break;
		};
	}
}

// ------------------------------------------------------------------------------------------------
template <class T>
VMapEntry* FindEntry(std::vector< T >& list,const std::string& name, bool perPoly)
{
	for (typename std::vector< T >::iterator it = list.begin(), end = list.end();
		 it != end; ++it)
	{
		if ((*it).name == name)
		{
			if (!perPoly)
			{
				DefaultLogger::get()->warn("LWO2: Found two VMAP sections with equal names");
			}
			return &(*it);
		}
	}
	list.push_back( T() );
	VMapEntry* p = &list.back();
	p->name = name;
	return p;
}

// ------------------------------------------------------------------------------------------------
template <class T>
inline void CreateNewEntry(T& chan, unsigned int srcIdx)
{
	if (!chan.name.length())return;

	chan.abAssigned[srcIdx] = true;
	chan.abAssigned.resize(chan.abAssigned.size()+1,false);

	for (unsigned int a = 0; a < chan.dims;++a)
		chan.rawData.push_back(chan.rawData[srcIdx*chan.dims+a]);
}

// ------------------------------------------------------------------------------------------------
template <class T>
inline void CreateNewEntry(std::vector< T >& list, unsigned int srcIdx)
{
	for (typename std::vector< T >::iterator 
		it =  list.begin(), end = list.end();
		it != end;++it)
	{
		CreateNewEntry( *it, srcIdx );
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::DoRecursiveVMAPAssignment(VMapEntry* base, unsigned int numRead, 
	unsigned int idx, float* data)
{
	ai_assert(NULL != data);
	LWO::ReferrerList& refList	= mCurLayer->mPointReferrers;
	unsigned int i;

	base->abAssigned[idx] = true;
	for (i = 0; i < numRead;++i) 
		base->rawData[idx*base->dims+i]= data[i];

	if (0xffffffff != (i = refList[idx]))
		DoRecursiveVMAPAssignment(base,numRead,i,data);
}

// ------------------------------------------------------------------------------------------------
void AddToSingleLinkedList(ReferrerList& refList, unsigned int srcIdx, unsigned int destIdx)
{
	if(0xffffffff == refList[srcIdx])
	{
		refList[srcIdx] = destIdx;
		return;
	}
	AddToSingleLinkedList(refList,refList[srcIdx],destIdx);
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2VertexMap(unsigned int length, bool perPoly)
{
	LE_NCONST uint8_t* const end = mFileBuffer+length;

	AI_LWO_VALIDATE_CHUNK_LENGTH(length,VMAP,6);
	unsigned int type = GetU4();
	unsigned int dims = GetU2();

	VMapEntry* base;

	// read the name of the vertex map 
	std::string name;
	GetS0(name,length);

	switch (type)
	{
	case AI_LWO_TXUV:
		if (dims != 2)
		{
			DefaultLogger::get()->warn("LWO2: Found UV channel with != 2 components"); 
		}
		base = FindEntry(mCurLayer->mUVChannels,name,perPoly);
		break;
	case AI_LWO_WGHT:
		if (dims != 1)
		{
			DefaultLogger::get()->warn("LWO2: found vertex weight map with != 1 components"); 
		}
		base = FindEntry(mCurLayer->mWeightChannels,name,perPoly);
		break;
	case AI_LWO_RGB:
	case AI_LWO_RGBA:
		if (dims != 3 && dims != 4)
		{
			DefaultLogger::get()->warn("LWO2: found vertex color map with != 3&4 components"); 
		}
		base = FindEntry(mCurLayer->mVColorChannels,name,perPoly);
		break;

	case AI_LWO_MODO_NORM:

		/*  This is a non-standard extension chunk used by Luxology's MODO.
		 *  It stores per-vertex normals. This VMAP exists just once, has
		 *  3 dimensions and is btw extremely beautiful.
		 */
		if (name != "vert_normals" || dims != 3 || mCurLayer->mNormals.name.length())
			return;

		DefaultLogger::get()->info("Non-standard extension: MODO VMAP.NORM.vert_normals");
		
		mCurLayer->mNormals.name = name;
		base = & mCurLayer->mNormals;
		break;

	default: 
		return;
	};
	base->Allocate((unsigned int)mCurLayer->mTempPoints.size());

	// now read all entries in the map
	type = std::min(dims,base->dims); 
	const unsigned int diff = (dims - type)<<2;

	LWO::FaceList& list	= mCurLayer->mFaces;
	LWO::PointList& pointList = mCurLayer->mTempPoints;
	LWO::ReferrerList& refList = mCurLayer->mPointReferrers;

	float temp[4];

	const unsigned int numPoints = (unsigned int)pointList.size();
	const unsigned int numFaces  = (unsigned int)list.size();

	while (mFileBuffer < end)
	{
		unsigned int idx = ReadVSizedIntLWO2(mFileBuffer) + mCurLayer->mPointIDXOfs;
		if (idx >= numPoints)
		{
			DefaultLogger::get()->warn("LWO2: vertex index in vmap/vmad is out of range");
			mFileBuffer += base->dims*4;continue;
		}
		if (perPoly)
		{
			unsigned int polyIdx = ReadVSizedIntLWO2(mFileBuffer) + mCurLayer->mFaceIDXOfs;
			if (base->abAssigned[idx])
			{
				// we have already a VMAP entry for this vertex - thus
				// we need to duplicate the corresponding polygon.
				if (polyIdx >= numFaces)
				{
					DefaultLogger::get()->warn("LWO2: VMAD polygon index is out of range");
					mFileBuffer += base->dims*4;
					continue;
				}

				LWO::Face& src = list[polyIdx];

				// generate a new unique vertex for the corresponding index - but only
				// if we can find the index in the face
				for (unsigned int i = 0; i < src.mNumIndices;++i)
				{
					register unsigned int srcIdx = src.mIndices[i];
					if (idx != srcIdx)continue;

					refList.resize(refList.size()+1, 0xffffffff);
						
					idx = (unsigned int)pointList.size();
					src.mIndices[i] = (unsigned int)pointList.size();

					// store the index of the new vertex in the old vertex
					// so we get a single linked list we can traverse in
					// only one direction
					AddToSingleLinkedList(refList,srcIdx,src.mIndices[i]);
					pointList.push_back(pointList[srcIdx]);

					CreateNewEntry(mCurLayer->mVColorChannels,	srcIdx );
					CreateNewEntry(mCurLayer->mUVChannels,		srcIdx );
					CreateNewEntry(mCurLayer->mWeightChannels,	srcIdx );
					CreateNewEntry(mCurLayer->mNormals,	srcIdx );
				}
			}
		}
		for (unsigned int l = 0; l < type;++l)
			temp[l] = GetF4();

		DoRecursiveVMAPAssignment(base,type,idx, temp);
		mFileBuffer += diff;
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2Clip(unsigned int length)
{
	AI_LWO_VALIDATE_CHUNK_LENGTH(length,CLIP,10);

	mClips.push_back(LWO::Clip());
	LWO::Clip& clip = mClips.back();

	// first - get the index of the clip
	clip.idx = GetU4();

	IFF::SubChunkHeader* const head = IFF::LoadSubChunk(mFileBuffer);
	switch (head->type)
	{
	case AI_LWO_STIL:

		// "Normal" texture
		GetS0(clip.path,head->length);
		clip.type = Clip::STILL;
		break;

	case AI_LWO_ISEQ:

		// Image sequence. We'll later take the first.
		{
			uint8_t digits = GetU1();  mFileBuffer++;
			int16_t offset = GetU2();  mFileBuffer+=4;
			int16_t start  = GetU2();  mFileBuffer+=4;

			std::string s;std::stringstream ss;
			GetS0(s,head->length);

			head->length -= (unsigned int)s.length()+1;
			ss << s;
			ss << std::setw(digits) << offset + start;
			GetS0(s,head->length);
			ss << s;
			clip.path = ss.str();
			clip.type = Clip::SEQ;
		}
		break;

	case AI_LWO_STCC:
		DefaultLogger::get()->warn("LWO2: Color shifted images are not supported");
		break;

	case AI_LWO_ANIM:
		DefaultLogger::get()->warn("LWO2: Animated textures are not supported");
		break;

	case AI_LWO_XREF:

		// Just a cross-reference to another CLIp
		clip.type = Clip::REF;
		clip.clipRef = GetU4();
		break;

	default:
		DefaultLogger::get()->warn("LWO2: Encountered unknown CLIP subchunk");
	}
}

// ------------------------------------------------------------------------------------------------
void LWOImporter::LoadLWO2File()
{
	bool skip = false;

	LE_NCONST uint8_t* const end = mFileBuffer + fileSize;
	while (true)
	{
		if (mFileBuffer + sizeof(IFF::ChunkHeader) > end)break;
		IFF::ChunkHeader* const head = IFF::LoadChunk(mFileBuffer);

		if (mFileBuffer + head->length > end)
		{
			throw new ImportErrorException("LWO2: Chunk length points behind the file");
			break;
		}
		uint8_t* const next = mFileBuffer+head->length;
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

				// load this layer or ignore it? Check the layer index property
				// NOTE: The first layer is the default layer, so the layer
				// index is one-based now
				if (0xffffffff != configLayerIndex && configLayerIndex != mLayers->size())
				{
					skip = true;
				}
				else skip = false;

				AI_LWO_VALIDATE_CHUNK_LENGTH(head->length,LAYR,16);

				// and parse its properties, e.g. the pivot point
				mFileBuffer += 2;
				mCurLayer->mPivot.x = GetF4();
				mCurLayer->mPivot.y = GetF4();
				mCurLayer->mPivot.z = GetF4();
				mFileBuffer += 2;
				GetS0(layer.mName,head->length-16);

				// if the name is empty, generate a default name
				if (layer.mName.empty())
				{
					char buffer[128]; // should be sufficiently large
					::sprintf(buffer,"Layer_%i", iUnnamed++);
					layer.mName = buffer;
				}

				// load this layer or ignore it? Check the layer name property
				if (configLayerName.length() && configLayerName != layer.mName)
				{
					skip = true;
				}
				else hasNamedLayer = true;

				if (mFileBuffer + 2 <= next)
					layer.mParent = GetU2();

				break;
			}

			// vertex list
		case AI_LWO_PNTS:
			{
				if (skip)break;

				unsigned int old = (unsigned int)mCurLayer->mTempPoints.size();
				LoadLWOPoints(head->length);
				mCurLayer->mPointIDXOfs = old;
				break;
			}
			// vertex tags
		case AI_LWO_VMAD:
			if (mCurLayer->mFaces.empty())
			{
				DefaultLogger::get()->warn("LWO2: Unexpected VMAD chunk");
				break;
			}
			// --- intentionally no break here
		case AI_LWO_VMAP:
			{
				if (skip)break;

				if (mCurLayer->mTempPoints.empty())
					DefaultLogger::get()->warn("LWO2: Unexpected VMAP chunk");
				else LoadLWO2VertexMap(head->length,head->type == AI_LWO_VMAD);
				break;
			}
			// face list
		case AI_LWO_POLS:
			{
				if (skip)break;

				unsigned int old = (unsigned int)mCurLayer->mFaces.size();
				LoadLWO2Polygons(head->length);
				mCurLayer->mFaceIDXOfs = old;
				break;
			}
			// polygon tags 
		case AI_LWO_PTAG:
			{
				if (skip)break;

				if (mCurLayer->mFaces.empty())
					DefaultLogger::get()->warn("LWO2: Unexpected PTAG");
				else LoadLWO2PolygonTags(head->length);
				break;
			}
			// list of tags
		case AI_LWO_TAGS:
			{
				if (!mTags->empty())
					DefaultLogger::get()->warn("LWO2: SRFS chunk encountered twice");
				else LoadLWOTags(head->length);
				break;
			}

			// surface chunk
		case AI_LWO_SURF:
			{
				LoadLWO2Surface(head->length);
				break;
			}

			// clip chunk
		case AI_LWO_CLIP:
			{
				LoadLWO2Clip(head->length);
				break;
			}
		}
		mFileBuffer = next;
	}
}
