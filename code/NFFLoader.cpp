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

/** @file Implementation of the STL importer class */

// internal headers
#include "NFFLoader.h"
#include "MaterialSystem.h"
#include "ParsingUtils.h"
#include "StandardShapes.h"
#include "fast_atof.h"
#include "qnan.h"

// public assimp headers
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "../include/DefaultLogger.h"

// boost headers
#include <boost/scoped_ptr.hpp>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
NFFImporter::NFFImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
NFFImporter::~NFFImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool NFFImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)return false;
	std::string extension = pFile.substr( pos);

	return !(extension.length() != 4 || extension[0] != '.' ||
			 extension[1] != 'n' && extension[1] != 'N' ||
			 extension[2] != 'f' && extension[2] != 'F' ||
			 extension[3] != 'f' && extension[3] != 'F');
}
// ------------------------------------------------------------------------------------------------
bool GetNextLine(const char*& buffer, char out[4096])
{
	if ('\0' == *buffer)return false;

	char* _out = out;
	char* const end = _out+4096;
	while (!IsLineEnd( *buffer ) && _out < end)
		*_out++ = *buffer++;
	*_out = '\0';

	if ('\0' != *buffer)while (IsLineEnd( *buffer ))++buffer;
	return true;
}

// ------------------------------------------------------------------------------------------------
#define AI_NFF_PARSE_FLOAT(f) \
	SkipSpaces(&sz); \
	if (!::IsLineEnd(*sz))sz = fast_atof_move(sz, (float&)f); 

// ------------------------------------------------------------------------------------------------
#define AI_NFF_PARSE_TRIPLE(v) \
	AI_NFF_PARSE_FLOAT(v.x) \
	AI_NFF_PARSE_FLOAT(v.y) \
	AI_NFF_PARSE_FLOAT(v.z) 

// ------------------------------------------------------------------------------------------------
#define AI_NFF_PARSE_SHAPE_INFORMATION() \
	aiVector3D center, radius(1.0f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN()); \
	AI_NFF_PARSE_TRIPLE(center); \
	AI_NFF_PARSE_TRIPLE(radius); \
	if (is_qnan(radius.z))radius.z = radius.x; \
	if (is_qnan(radius.y))radius.y = radius.x; \
	currentMesh.radius = radius; \
	currentMesh.center = center;

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void NFFImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, "rb"));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open NFF file " + pFile + ".");

	unsigned int m = (unsigned int)file->FileSize();

	// allocate storage and copy the contents of the file to a memory buffer
	// (terminate it with zero)
	std::vector<char> mBuffer2(m+1);
	file->Read(&mBuffer2[0],m,1);
	const char* buffer = &mBuffer2[0];
	mBuffer2[m] = '\0';

	// mesh arrays - separate here to make the handling of
	// the pointers below easier.
	std::vector<MeshInfo> meshes;
	std::vector<MeshInfo> meshesWithNormals;
	std::vector<MeshInfo> meshesLocked;
	MeshInfo* currentMeshWithNormals = NULL;
	MeshInfo* currentMesh = NULL;

	ShadingInfo s; // current material info

	// degree of tesselation
	unsigned int iTesselation = 4;

	char line[4096];
	const char* sz;
	unsigned int sphere = 0,cylinder = 0,cone = 0,numNamed = 0,
		dodecahedron = 0,octahedron = 0,tetrahedron = 0, hexahedron = 0;

	while (GetNextLine(buffer,line))
	{
		if ('p' == line[0])
		{
			MeshInfo* out = NULL;
			// 'pp' - polygon patch primitive
			if ('p' == line[1])
			{
				if (meshesWithNormals.empty())
				{
					meshesWithNormals.push_back(MeshInfo(true));
					currentMeshWithNormals = &meshesWithNormals.back();
				}
		
				sz = &line[2];out = currentMeshWithNormals;
			}
			// 'p' - polygon primitive
			else
			{
				if (meshes.empty())
				{
					meshes.push_back(MeshInfo(false));
					currentMesh = &meshes.back();
				}
				sz = &line[1];out = currentMesh;
			}
			SkipSpaces(sz,&sz);
			m = strtol10(sz);

			// ---- flip the face order
			out->vertices.resize(out->vertices.size()+m);
			if (out == currentMeshWithNormals)
			{
				out->normals.resize(out->vertices.size());
			}
			for (unsigned int n = 0; n < m;++n)
			{
				if(!GetNextLine(buffer,line))
				{
					DefaultLogger::get()->error("NFF: Unexpected EOF was encountered");
					continue;
				}

				aiVector3D v; sz = &line[0];
				AI_NFF_PARSE_TRIPLE(v);
				out->vertices[out->vertices.size()-n-1] = v;

				if (out == currentMeshWithNormals)
				{
					AI_NFF_PARSE_TRIPLE(v);
					out->normals[out->vertices.size()-n-1] = v;
				}
			}
			out->faces.push_back(m);
		}
		// 'f' - shading information block
		else if ('f' == line[0] && IsSpace(line[1]))
		{
			SkipSpaces(&line[1],&sz);

			// read just the RGB colors, the rest is ignored for the moment
			sz = fast_atof_move(sz, (float&)s.color.r);
			SkipSpaces(&sz);
			sz = fast_atof_move(sz, (float&)s.color.g);
			SkipSpaces(&sz);
			sz = fast_atof_move(sz, (float&)s.color.b);

			// check whether we have this material already -
			// although we have the RRM-Step, this is necessary here.
			// otherwise we would generate hundreds of small meshes
			// with just a few faces - this is surely never wanted.
			currentMesh = currentMeshWithNormals = NULL;
			for (std::vector<MeshInfo>::iterator it = meshes.begin(), end = meshes.end();
				it != end;++it)
			{
				if ((*it).bLocked)continue;
				if ((*it).shader == s)
				{
					if ((*it).bHasNormals)currentMeshWithNormals = &(*it);
					else currentMesh = &(*it);
				}
			}

			if (!currentMesh)
			{
				meshes.push_back(MeshInfo(false));
				currentMesh = &meshes.back();
				currentMesh->shader = s;
			}

			if (!currentMeshWithNormals)
			{
				meshesWithNormals.push_back(MeshInfo(true));
				currentMeshWithNormals = &meshesWithNormals.back();
				currentMeshWithNormals->shader = s;
			}
		}
		// 's' - sphere
		else if ('s' == line[0] && IsSpace(line[1]))
		{
			meshesLocked.push_back(MeshInfo(false,true));
			MeshInfo& currentMesh = meshesLocked.back();
			currentMesh.shader = s;

			sz = &line[1]; 
			AI_NFF_PARSE_SHAPE_INFORMATION();

			// we don't need scaling or translation here - we do it in the node's transform
			StandardShapes::MakeSphere(iTesselation, currentMesh.vertices);
			currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

			// generate a name for the mesh
			::sprintf(currentMesh.name,"sphere_%i",sphere++);
		}
		// 'dod' - dodecahedron
		else if (!strncmp(line,"dod",3) && IsSpace(line[3]))
		{
			meshesLocked.push_back(MeshInfo(false,true));
			MeshInfo& currentMesh = meshesLocked.back();
			currentMesh.shader = s;

			sz = &line[4]; 
			AI_NFF_PARSE_SHAPE_INFORMATION();

			// we don't need scaling or translation here - we do it in the node's transform
			StandardShapes::MakeDodecahedron(currentMesh.vertices);
			currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

			// generate a name for the mesh
			::sprintf(currentMesh.name,"dodecahedron_%i",dodecahedron++);
		}

		// 'oct' - octahedron
		else if (!strncmp(line,"oct",3) && IsSpace(line[3]))
		{
			meshesLocked.push_back(MeshInfo(false,true));
			MeshInfo& currentMesh = meshesLocked.back();
			currentMesh.shader = s;

			sz = &line[4]; 
			AI_NFF_PARSE_SHAPE_INFORMATION();

			// we don't need scaling or translation here - we do it in the node's transform
			StandardShapes::MakeOctahedron(currentMesh.vertices);
			currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

			// generate a name for the mesh
			::sprintf(currentMesh.name,"octahedron_%i",octahedron++);
		}

		// 'tet' - tetrahedron
		else if (!strncmp(line,"tet",3) && IsSpace(line[3]))
		{
			meshesLocked.push_back(MeshInfo(false,true));
			MeshInfo& currentMesh = meshesLocked.back();
			currentMesh.shader = s;

			sz = &line[4]; 
			AI_NFF_PARSE_SHAPE_INFORMATION();

			// we don't need scaling or translation here - we do it in the node's transform
			StandardShapes::MakeTetrahedron(currentMesh.vertices);
			currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

			// generate a name for the mesh
			::sprintf(currentMesh.name,"tetrahedron_%i",tetrahedron++);
		}

		// 'hex' - hexahedron
		else if (!strncmp(line,"hex",3) && IsSpace(line[3]))
		{
			meshesLocked.push_back(MeshInfo(false,true));
			MeshInfo& currentMesh = meshesLocked.back();
			currentMesh.shader = s;

			sz = &line[4]; 
			AI_NFF_PARSE_SHAPE_INFORMATION();

			// we don't need scaling or translation here - we do it in the node's transform
			StandardShapes::MakeHexahedron(currentMesh.vertices);
			currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

			// generate a name for the mesh
			::sprintf(currentMesh.name,"hexahedron_%i",hexahedron++);
		}

		// 'tess' - tesselation
		else if (!strncmp(line,"tess",4) && IsSpace(line[4]))
		{
			sz = &line[5];SkipSpaces(&sz);
			iTesselation = strtol10(sz);
		}
		// 'c' - cone
		else if ('c' == line[0] && IsSpace(line[1]))
		{
			meshesLocked.push_back(MeshInfo(false,true));
			MeshInfo& currentMesh = meshes.back();
			currentMesh.shader = s;

			sz = &line[1];
			aiVector3D center1, center2; float radius1, radius2;
			AI_NFF_PARSE_TRIPLE(center1);
			AI_NFF_PARSE_FLOAT(radius1);
			AI_NFF_PARSE_TRIPLE(center2);
			AI_NFF_PARSE_FLOAT(radius2);

			// compute the center point of the cone/cylinder
			center2 = (center2-center1)/2.f;
			currentMesh.center = center1+center2;
			center1 = -center2;

			// generate the cone - it consists of simple triangles
			StandardShapes::MakeCone(center1, radius1, center2, radius2, iTesselation, currentMesh.vertices);
			currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

			// generate a name for the mesh
			if (radius1 != radius2)
				::sprintf(currentMesh.name,"cone_%i",cone++);
			else ::sprintf(currentMesh.name,"cylinder_%i",cylinder++);
		}
		// '#' - comment
		else if ('#' == line[0])
		{
			const char* sz;SkipSpaces(&line[1],&sz);
			if (!IsLineEnd(*sz))DefaultLogger::get()->info(sz);
		}
	}

	// copy all arrays into one large
	meshes.reserve(meshes.size()+meshesLocked.size()+meshesWithNormals.size());
	meshes.insert(meshes.end(),meshesLocked.begin(),meshesLocked.end());
	meshes.insert(meshes.end(),meshesWithNormals.begin(),meshesWithNormals.end());

	// now generate output meshes. first find out how many meshes we'll need
	std::vector<MeshInfo>::const_iterator it = meshes.begin(), end = meshes.end();
	for (;it != end;++it)
	{
		if (!(*it).faces.empty())
		{
			++pScene->mNumMeshes;
			if ((*it).name[0])++numNamed;
		}
	}

	// generate a dummy root node - assign all unnamed elements such
	// as polygons and polygon patches to the root node and generate
	// sub nodes for named objects such as spheres and cones.
	aiNode* const root = new aiNode();
	root->mName.Set("<NFF_Root>");
	root->mNumChildren = numNamed;
	root->mNumMeshes = pScene->mNumMeshes-numNamed;

	aiNode** ppcChildren;
	unsigned int* pMeshes;
	if (root->mNumMeshes)
		pMeshes = root->mMeshes = new unsigned int[root->mNumMeshes];
	if (root->mNumChildren)
		ppcChildren = root->mChildren = new aiNode*[root->mNumChildren];


	if (!pScene->mNumMeshes)throw new ImportErrorException("NFF: No meshes loaded");
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
	pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials = pScene->mNumMeshes];
	for (it = meshes.begin(), m = 0; it != end;++it)
	{
		if ((*it).faces.empty())continue;

		const MeshInfo& src = *it;
		aiMesh* const mesh = pScene->mMeshes[m] = new aiMesh();
		mesh->mNumVertices = (unsigned int)src.vertices.size();
		mesh->mNumFaces = (unsigned int)src.faces.size();

		// generate sub nodes for named meshes
		if (src.name[0])
		{
			aiNode* const node = *ppcChildren = new aiNode();
			node->mParent = root;
			node->mNumMeshes = 1;
			node->mMeshes = new unsigned int[1];
			node->mMeshes[0] = m;
			node->mName.Set(src.name);

			// setup the transformation matrix of the node
			node->mTransformation.a4 = src.center.x;
			node->mTransformation.b4 = src.center.y;
			node->mTransformation.c4 = src.center.z;

			node->mTransformation.a1 = src.radius.x;
			node->mTransformation.b2 = src.radius.y;
			node->mTransformation.c3 = src.radius.z;

			++ppcChildren;
		}
		else *pMeshes++ = m;

		// copy vertex positions
		mesh->mVertices = new aiVector3D[mesh->mNumVertices];
		::memcpy(mesh->mVertices,&src.vertices[0],sizeof(aiVector3D)*mesh->mNumVertices);
		if (src.bHasNormals)
		{
			ai_assert(src.normals.size() == src.vertices.size());

			// copy normal vectors
			mesh->mNormals = new aiVector3D[mesh->mNumVertices];
			::memcpy(mesh->mNormals,&src.normals[0],sizeof(aiVector3D)*mesh->mNumVertices);
		}

		// generate faces
		unsigned int p = 0;
		aiFace* pFace = mesh->mFaces = new aiFace[mesh->mNumFaces];
		for (std::vector<unsigned int>::const_iterator it2 = src.faces.begin(),
			end2 = src.faces.end();
			it2 != end2;++it2,++pFace)
		{
			pFace->mIndices = new unsigned int [ pFace->mNumIndices = *it2 ];
			for (unsigned int o = 0; o < pFace->mNumIndices;++o)
				pFace->mIndices[o] = p++;
		}

		// generate a material for the mesh
		MaterialHelper* pcMat = (MaterialHelper*)(pScene->
			mMaterials[m] = new MaterialHelper());

		mesh->mMaterialIndex = m++;

		aiString s;
		s.Set(AI_DEFAULT_MATERIAL_NAME);
		pcMat->AddProperty(&s, AI_MATKEY_NAME);

		pcMat->AddProperty(&src.shader.color,1,AI_MATKEY_COLOR_DIFFUSE);
		pcMat->AddProperty(&src.shader.color,1,AI_MATKEY_COLOR_SPECULAR);
	}
	pScene->mRootNode = root;
}
