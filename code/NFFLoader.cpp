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

#include "AssimpPCH.h"

// internal headers
#include "NFFLoader.h"
#include "ParsingUtils.h"
#include "StandardShapes.h"
#include "fast_atof.h"


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

	// extensions: enff and nff
	if (!extension.length() || extension[0] != '.')return false;
	if (extension.length() == 4)
	{
		return !(extension[1] != 'n' && extension[1] != 'N' ||
				 extension[2] != 'f' && extension[2] != 'F' ||
				 extension[3] != 'f' && extension[3] != 'F');
	}
	else return !(	extension.length() != 5 ||
					extension[1] != 'e' && extension[1] != 'E' ||
					extension[2] != 'n' && extension[2] != 'N' ||
					extension[3] != 'f' && extension[3] != 'F' ||
					extension[4] != 'f' && extension[4] != 'F');
}

// ------------------------------------------------------------------------------------------------
#define AI_NFF_PARSE_FLOAT(f) \
	SkipSpaces(&sz); \
	if (!::IsLineEnd(*sz))sz = fast_atof_move(sz, (float&)f); 

// ------------------------------------------------------------------------------------------------
#define AI_NFF_PARSE_TRIPLE(v) \
	AI_NFF_PARSE_FLOAT(v[0]) \
	AI_NFF_PARSE_FLOAT(v[1]) \
	AI_NFF_PARSE_FLOAT(v[2]) 

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
	std::vector<MeshInfo> meshesWithUVCoords;
	std::vector<MeshInfo> meshesLocked;

	char line[4096];
	const char* sz;

	// camera parameters
	aiVector3D camPos, camUp(0.f,1.f,0.f), camLookAt(0.f,0.f,1.f);
	float angle;
	aiVector2D resolution;

	bool hasCam = false;

	MeshInfo* currentMeshWithNormals = NULL;
	MeshInfo* currentMesh = NULL;
	MeshInfo* currentMeshWithUVCoords = NULL;

	ShadingInfo s; // current material info

	// degree of tesselation
	unsigned int iTesselation = 4;

	// some temporary variables we need to parse the file
	unsigned int sphere		= 0,
		cylinder			= 0,
		cone				= 0,
		numNamed			= 0,
		dodecahedron		= 0,
		octahedron			= 0,
		tetrahedron			= 0,
		hexahedron			= 0;

	// lights imported from the file
	std::vector<Light> lights;

	// check whether this is the NFF2 file format
	if (TokenMatch(buffer,"nff",3))
	{
		// another NFF file format ... just a raw parser has been implemented
		// no support for textures yet, I don't think it is worth the effort
		// http://ozviz.wasp.uwa.edu.au/~pbourke/dataformats/nff/nff2.html

		while (GetNextLine(buffer,line))
		{
			sz = line;
			if (TokenMatch(sz,"version",7))
			{
				DefaultLogger::get()->info("NFF (alt.) file format: " + std::string(sz));
			}
			else if (TokenMatch(sz,"viewpos",7))
			{
				AI_NFF_PARSE_TRIPLE(camPos);
				hasCam = true;
			}
			else if (TokenMatch(sz,"viewdir",7))
			{
				AI_NFF_PARSE_TRIPLE(camLookAt);
				hasCam = true;
			}
			else if (TokenMatch(sz,"//",2))
			{
				// comment ...
				DefaultLogger::get()->info(sz);
			}
			else if (!IsSpace(*sz))
			{
				// must be a new object
				meshes.push_back(MeshInfo(PatchType_Simple));
				MeshInfo& mesh = meshes.back();

				if (!GetNextLine(buffer,line))
				{DefaultLogger::get()->warn("NFF2: Unexpected EOF, can't read number of vertices");break;}

				SkipSpaces(line,&sz);
				unsigned int num = ::strtol10(sz,&sz);
				
				std::vector<aiVector3D> tempPositions;
				std::vector<aiVector3D> outPositions;
				mesh.vertices.reserve(num*3);
				mesh.colors.reserve (num*3);
				tempPositions.reserve(num);
				for (unsigned int i = 0; i < num; ++i)
				{
					if (!GetNextLine(buffer,line))
						{DefaultLogger::get()->warn("NFF2: Unexpected EOF, can't read vertices");break;}

					sz = line;
					aiVector3D v;
					AI_NFF_PARSE_TRIPLE(v);
					tempPositions.push_back(v);
				}
				if (!GetNextLine(buffer,line))
					{DefaultLogger::get()->warn("NFF2: Unexpected EOF, can't read number of faces");break;}

				if (!num)throw new ImportErrorException("NFF2: There are zero vertices");

				SkipSpaces(line,&sz);
				num = ::strtol10(sz,&sz);
				mesh.faces.reserve(num);

				for (unsigned int i = 0; i < num; ++i)
				{
					if (!GetNextLine(buffer,line))
						{DefaultLogger::get()->warn("NFF2: Unexpected EOF, can't read faces");break;}

					SkipSpaces(line,&sz);
					unsigned int idx, numIdx = ::strtol10(sz,&sz);
					if (numIdx)
					{
						mesh.faces.push_back(numIdx);
						for (unsigned int a = 0; a < numIdx;++a)
						{
							SkipSpaces(sz,&sz);
							idx = ::strtol10(sz,&sz);
							if (idx >= (unsigned int)tempPositions.size())
							{
								DefaultLogger::get()->error("NFF2: Index overflow");
								idx = 0;
							}
							mesh.vertices.push_back(tempPositions[idx]);
						}
					}

					SkipSpaces(sz,&sz);
					idx = ::strtol_cppstyle(sz,&sz);
					aiColor4D clr;
					clr.r = ((numIdx >> 8u) & 0xf) / 16.f;
					clr.g = ((numIdx >> 4u) & 0xf) / 16.f;
					clr.b = ((numIdx)       & 0xf) / 16.f;
					clr.a = 1.f;
					for (unsigned int a = 0; a < numIdx;++a)
						mesh.colors.push_back(clr);
				}
				if (!num)throw new ImportErrorException("NFF2: There are zero faces");
			}
		}
		camLookAt = camLookAt + camPos;
	}
	else // "Normal" Neutral file format that is quite more common
	{
		while (GetNextLine(buffer,line))
		{
			sz = line;
			if ('p' == line[0] || TokenMatch(sz,"tpp",3))
			{
				MeshInfo* out = NULL;

				// 'tpp' - texture polygon patch primitive
				if ('t' == line[0])
				{
					if (meshesWithUVCoords.empty())
					{
						meshesWithUVCoords.push_back(MeshInfo(PatchType_UVAndNormals));
						currentMeshWithUVCoords = &meshesWithUVCoords.back();
					}

					out = currentMeshWithUVCoords;
				}
				// 'pp' - polygon patch primitive
				else if ('p' == line[1])
				{
					if (meshesWithNormals.empty())
					{
						meshesWithNormals.push_back(MeshInfo(PatchType_Normals));
						currentMeshWithNormals = &meshesWithNormals.back();
					}

					sz = &line[2];out = currentMeshWithNormals;
				}
				// 'p' - polygon primitive
				else
				{
					if (meshes.empty())
					{
						meshes.push_back(MeshInfo(PatchType_Simple));
						currentMesh = &meshes.back();
					}
					sz = &line[1];out = currentMesh;
				}
				SkipSpaces(sz,&sz);
				m = strtol10(sz);

				// ---- flip the face order
				out->vertices.resize(out->vertices.size()+m);
				if (out != currentMesh)
				{
					out->normals.resize(out->vertices.size());
				}
				if (out == currentMeshWithUVCoords)
				{
					out->uvs.resize(out->vertices.size());
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

					if (out != currentMesh)
					{
						AI_NFF_PARSE_TRIPLE(v);
						out->normals[out->vertices.size()-n-1] = v;
					}
					if (out == currentMeshWithUVCoords)
					{
						// FIX: in one test file this wraps over multiple lines
						SkipSpaces(&sz);
						if (IsLineEnd(*sz))
						{
							GetNextLine(buffer,line);
							sz = line;
						}
						AI_NFF_PARSE_FLOAT(v.x);
						SkipSpaces(&sz);
						if (IsLineEnd(*sz))
						{
							GetNextLine(buffer,line);
							sz = line;
						}
						AI_NFF_PARSE_FLOAT(v.y);
						v.y = 1.f - v.y;
						out->uvs[out->vertices.size()-n-1] = v;
					}
				}
				out->faces.push_back(m);
			}
			// 'f' - shading information block
			else if (TokenMatch(sz,"f",1))
			{
				float d;

				// read the RGB colors
				AI_NFF_PARSE_TRIPLE(s.color);

				// read the other properties
				AI_NFF_PARSE_FLOAT(s.diffuse);
				AI_NFF_PARSE_FLOAT(s.specular);
				AI_NFF_PARSE_FLOAT(d); // skip shininess and transmittance
				AI_NFF_PARSE_FLOAT(d);
				AI_NFF_PARSE_FLOAT(s.refracti);

				// if the next one is NOT a number we assume it is a texture file name
				// this feature is used by some NFF files on the internet and it has
				// been implemented as it can be really useful
				SkipSpaces(&sz);
				if (!IsNumeric(*sz))
				{
					// TODO: Support full file names with spaces and quotation marks ...
					const char* p = sz;
					while (!IsSpaceOrNewLine( *sz ))++sz;

					unsigned int diff = (unsigned int)(sz-p);
					if (diff)
					{
						s.texFile = std::string(p,diff);
					}
				}
				else
				{
					AI_NFF_PARSE_FLOAT(s.ambient); // optional
				}

				// check whether we have this material already -
				// although we have the RRM-Step, this is necessary here.
				// otherwise we would generate hundreds of small meshes
				// with just a few faces - this is surely never wanted.
				currentMesh = currentMeshWithNormals = currentMeshWithUVCoords = NULL;
				for (std::vector<MeshInfo>::iterator it = meshes.begin(), end = meshes.end();
					it != end;++it)
				{
					if ((*it).bLocked)continue;
					if ((*it).shader == s)
					{
						switch ((*it).pType)
						{
						case PatchType_Normals:
							currentMeshWithNormals = &(*it);
							break;

						case PatchType_Simple:
							currentMesh = &(*it);
							break;

						default:
							currentMeshWithUVCoords = &(*it);
							break;
						};
					}
				}

				if (!currentMesh)
				{
					meshes.push_back(MeshInfo(PatchType_Simple));
					currentMesh = &meshes.back();
					currentMesh->shader = s;
				}

				if (!currentMeshWithNormals)
				{
					meshesWithNormals.push_back(MeshInfo(PatchType_Normals));
					currentMeshWithNormals = &meshesWithNormals.back();
					currentMeshWithNormals->shader = s;
				}

				if (!currentMeshWithUVCoords)
				{
					meshesWithUVCoords.push_back(MeshInfo(PatchType_UVAndNormals));
					currentMeshWithUVCoords = &meshesWithUVCoords.back();
					currentMeshWithUVCoords->shader = s;
				}
			}
			// 'l' - light source
			else if (TokenMatch(sz,"l",1))
			{
				lights.push_back(Light());
				Light& light = lights.back();

				AI_NFF_PARSE_TRIPLE(light.position);
				AI_NFF_PARSE_FLOAT (light.intensity);
				AI_NFF_PARSE_TRIPLE(light.color);
			}
			// 's' - sphere
			else if (TokenMatch(sz,"s",1))
			{
				meshesLocked.push_back(MeshInfo(PatchType_Simple,true));
				MeshInfo& currentMesh = meshesLocked.back();
				currentMesh.shader = s;

				AI_NFF_PARSE_SHAPE_INFORMATION();

				// we don't need scaling or translation here - we do it in the node's transform
				StandardShapes::MakeSphere(iTesselation, currentMesh.vertices);
				currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

				// generate a name for the mesh
				::sprintf(currentMesh.name,"sphere_%i",sphere++);
			}
			// 'dod' - dodecahedron
			else if (TokenMatch(sz,"dod",3))
			{
				meshesLocked.push_back(MeshInfo(PatchType_Simple,true));
				MeshInfo& currentMesh = meshesLocked.back();
				currentMesh.shader = s;

				AI_NFF_PARSE_SHAPE_INFORMATION();

				// we don't need scaling or translation here - we do it in the node's transform
				StandardShapes::MakeDodecahedron(currentMesh.vertices);
				currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

				// generate a name for the mesh
				::sprintf(currentMesh.name,"dodecahedron_%i",dodecahedron++);
			}

			// 'oct' - octahedron
			else if (TokenMatch(sz,"oct",3))
			{
				meshesLocked.push_back(MeshInfo(PatchType_Simple,true));
				MeshInfo& currentMesh = meshesLocked.back();
				currentMesh.shader = s;

				AI_NFF_PARSE_SHAPE_INFORMATION();

				// we don't need scaling or translation here - we do it in the node's transform
				StandardShapes::MakeOctahedron(currentMesh.vertices);
				currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

				// generate a name for the mesh
				::sprintf(currentMesh.name,"octahedron_%i",octahedron++);
			}

			// 'tet' - tetrahedron
			else if (TokenMatch(sz,"tet",3))
			{
				meshesLocked.push_back(MeshInfo(PatchType_Simple,true));
				MeshInfo& currentMesh = meshesLocked.back();
				currentMesh.shader = s;

				AI_NFF_PARSE_SHAPE_INFORMATION();

				// we don't need scaling or translation here - we do it in the node's transform
				StandardShapes::MakeTetrahedron(currentMesh.vertices);
				currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

				// generate a name for the mesh
				::sprintf(currentMesh.name,"tetrahedron_%i",tetrahedron++);
			}

			// 'hex' - hexahedron
			else if (TokenMatch(sz,"hex",3))
			{
				meshesLocked.push_back(MeshInfo(PatchType_Simple,true));
				MeshInfo& currentMesh = meshesLocked.back();
				currentMesh.shader = s;

				AI_NFF_PARSE_SHAPE_INFORMATION();

				// we don't need scaling or translation here - we do it in the node's transform
				StandardShapes::MakeHexahedron(currentMesh.vertices);
				currentMesh.faces.resize(currentMesh.vertices.size()/3,3);

				// generate a name for the mesh
				::sprintf(currentMesh.name,"hexahedron_%i",hexahedron++);
			}
			// 'c' - cone
			else if (TokenMatch(sz,"c",1))
			{
				meshesLocked.push_back(MeshInfo(PatchType_Simple,true));
				MeshInfo& currentMesh = meshes.back();
				currentMesh.shader = s;

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
			// 'tess' - tesselation
			else if (TokenMatch(sz,"tess",4))
			{
				SkipSpaces(&sz);
				iTesselation = strtol10(sz);
			}
			// 'from' - camera position
			else if (TokenMatch(sz,"from",4))
			{
				AI_NFF_PARSE_TRIPLE(camPos);
				hasCam = true;
			}
			// 'at' - camera look-at vector
			else if (TokenMatch(sz,"at",2))
			{
				AI_NFF_PARSE_TRIPLE(camLookAt);
				hasCam = true;
			}
			// 'up' - camera up vector
			else if (TokenMatch(sz,"up",2))
			{
				AI_NFF_PARSE_TRIPLE(camUp);
				hasCam = true;
			}
			// 'angle' - (half?) camera field of view
			else if (TokenMatch(sz,"angle",5))
			{
				AI_NFF_PARSE_FLOAT(angle);
				hasCam = true;
			}
			// 'resolution' - used to compute the screen aspect
			else if (TokenMatch(sz,"resolution",10))
			{
				AI_NFF_PARSE_FLOAT(resolution.x);
				AI_NFF_PARSE_FLOAT(resolution.y);
				hasCam = true;
			}
			// 'pb' - bezier patch. Not supported yet
			else if (TokenMatch(sz,"pb",2))
			{
				DefaultLogger::get()->error("NFF: Encountered unsupported ID: bezier patch");
			}
			// 'pn' - NURBS. Not supported yet
			else if (TokenMatch(sz,"pn",2) || TokenMatch(sz,"pnn",3))
			{
				DefaultLogger::get()->error("NFF: Encountered unsupported ID: NURBS");
			}
			// '' - comment
			else if ('#' == line[0])
			{
				const char* sz;SkipSpaces(&line[1],&sz);
				if (!IsLineEnd(*sz))DefaultLogger::get()->info(sz);
			}
		}
	}

	// copy all arrays into one large
	meshes.reserve (meshes.size()+meshesLocked.size()+meshesWithNormals.size()+meshesWithUVCoords.size());
	meshes.insert  (meshes.end(),meshesLocked.begin(),meshesLocked.end());
	meshes.insert  (meshes.end(),meshesWithNormals.begin(),meshesWithNormals.end());
	meshes.insert  (meshes.end(),meshesWithUVCoords.begin(),meshesWithUVCoords.end());

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
	root->mNumChildren = numNamed + (hasCam ? 1 : 0) + (unsigned int) lights.size();
	root->mNumMeshes = pScene->mNumMeshes-numNamed;

	aiNode** ppcChildren;
	unsigned int* pMeshes;
	if (root->mNumMeshes)
		pMeshes = root->mMeshes = new unsigned int[root->mNumMeshes];
	if (root->mNumChildren)
		ppcChildren = root->mChildren = new aiNode*[root->mNumChildren];

	// generate the camera
	if (hasCam)
	{
		aiNode* nd = *ppcChildren = new aiNode();
		nd->mName.Set("<NFF_Camera>");
		nd->mParent = root;

		// allocate the camera in the scene
		pScene->mNumCameras = 1;
		pScene->mCameras = new aiCamera*[1];
		aiCamera* c = pScene->mCameras[0] = new aiCamera;

		c->mName = nd->mName; // make sure the names are identical
		c->mHorizontalFOV = AI_DEG_TO_RAD( angle );
		c->mLookAt		= camLookAt - camPos;
		c->mPosition	= camPos;
		c->mUp			= camUp;
		c->mAspect		= resolution.x / resolution.y;
		++ppcChildren;
	}

	// generate light sources
	if (!lights.empty())
	{
		pScene->mNumLights = (unsigned int)lights.size();
		pScene->mLights = new aiLight*[pScene->mNumLights];
		for (unsigned int i = 0; i < pScene->mNumLights;++i,++ppcChildren)
		{
			const Light& l = lights[i];

			aiNode* nd = *ppcChildren  = new aiNode();
			nd->mParent = root;

			nd->mName.length = ::sprintf(nd->mName.data,"<NFF_Light%i>",i);

			// allocate the light in the scene data structure
			aiLight* out = pScene->mLights[i] = new aiLight();
			out->mName = nd->mName; // make sure the names are identical
			out->mType = aiLightSource_POINT;
			out->mColorDiffuse = out->mColorSpecular = l.color * l.intensity;
			out->mPosition = l.position;
		}
	}

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
		::memcpy(mesh->mVertices,&src.vertices[0],
			sizeof(aiVector3D)*mesh->mNumVertices);

		// NFF2: there could be vertex colors
		if (!src.colors.empty())
		{
			ai_assert(src.colors.size() == src.vertices.size());

			// copy vertex colors
			mesh->mColors[0] = new aiColor4D[mesh->mNumVertices];
			::memcpy(mesh->mColors[0],&src.colors[0],
				sizeof(aiColor4D)*mesh->mNumVertices);
		}

		if (src.pType != PatchType_Simple)
		{
			ai_assert(src.normals.size() == src.vertices.size());

			// copy normal vectors
			mesh->mNormals = new aiVector3D[mesh->mNumVertices];
			::memcpy(mesh->mNormals,&src.normals[0],
				sizeof(aiVector3D)*mesh->mNumVertices);
		}

		if (src.pType == PatchType_UVAndNormals)
		{
			ai_assert(src.uvs.size() == src.vertices.size());

			// copy texture coordinates
			mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
			::memcpy(mesh->mTextureCoords[0],&src.uvs[0],
				sizeof(aiVector3D)*mesh->mNumVertices);
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
		MaterialHelper* pcMat = (MaterialHelper*)(pScene->mMaterials[m] = new MaterialHelper());

		mesh->mMaterialIndex = m++;

		aiString s;
		s.Set(AI_DEFAULT_MATERIAL_NAME);
		pcMat->AddProperty(&s, AI_MATKEY_NAME);

		aiColor3D c = src.shader.color * src.shader.diffuse;
		pcMat->AddProperty(&c,1,AI_MATKEY_COLOR_DIFFUSE);
		c = src.shader.color * src.shader.specular;
		pcMat->AddProperty(&c,1,AI_MATKEY_COLOR_SPECULAR);

		if (src.shader.texFile.length())
		{
			s.Set(src.shader.texFile);
			pcMat->AddProperty(&s,AI_MATKEY_TEXTURE_DIFFUSE(0));
		}
	}
	pScene->mRootNode = root;
}
