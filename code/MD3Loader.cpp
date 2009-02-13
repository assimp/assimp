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

/** @file MD3Loader.cpp
 *  @brief Implementation of the MD3 importer class 
 * 
 *  Sources: 
 *     http://www.gamers.org/dEngine/quake3/UQ3S
 *     http://linux.ucla.edu/~phaethon/q3/formats/md3format.html
 *     http://www.heppler.com/shader/shader/
 */

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_MD3_IMPORTER

#include "MD3Loader.h"
#include "ByteSwap.h"
#include "SceneCombiner.h"
#include "GenericProperty.h"
#include "RemoveComments.h"
#include "ParsingUtils.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Load a Quake 3 shader
void Q3Shader::LoadShader(ShaderData& fill, const std::string& pFile,IOSystem* io)
{
	boost::scoped_ptr<IOStream> file( io->Open( pFile, "rt"));
	if (!file.get())
		return; // if we can't access the file, don't worry and return

	DefaultLogger::get()->info("Loading Quake3 shader file " + pFile);

	// read file in memory
	const size_t s = file->FileSize();
	std::vector<char> _buff(s+1);
	file->Read(&_buff[0],s,1);
	_buff[s] = 0;

	// remove comments from it (C++ style)
	CommentRemover::RemoveLineComments("//",&_buff[0]);
	const char* buff = &_buff[0];

	Q3Shader::ShaderDataBlock* curData = NULL;
	Q3Shader::ShaderMapBlock*  curMap  = NULL;

	// read line per line
	for (;;SkipLine(&buff)) {
	
		if(!SkipSpacesAndLineEnd(&buff))
			break;

		if (*buff == '{') {
			// append to last section, if any
			if (!curData) {
				DefaultLogger::get()->error("Q3Shader: Unexpected shader section token \'{\'");
				return;
			}

			// read this map section
			for (;;SkipLine(&buff)) {
				if(!SkipSpacesAndLineEnd(&buff))
					break;

				if (*buff == '{') {
					// add new map section
					curData->maps.push_back(Q3Shader::ShaderMapBlock());
					curMap = &curData->maps.back();

				}
				else if (*buff == '}') {
					// close this map section
					if (curMap)
						curMap = NULL;
					else {
						curData = NULL;					
						break;
					}
				}
				// 'map' - Specifies texture file name
				else if (TokenMatchI(buff,"map",3) || TokenMatchI(buff,"clampmap",8)) {
					curMap->name = GetNextToken(buff);
				}	
				// 'blendfunc' - Alpha blending mode
				else if (TokenMatchI(buff,"blendfunc",9)) {	
					// fixme
				}
			}
		}

		// 'cull' specifies culling behaviour for the model
		else if (TokenMatch(buff,"cull",4)) {
			SkipSpaces(&buff);
			if (!ASSIMP_strincmp(buff,"back",4)) {
				curData->cull = Q3Shader::CULL_CCW;
			}
			else if (!ASSIMP_strincmp(buff,"front",5)) {
				curData->cull = Q3Shader::CULL_CW;
			}
			//else curData->cull = Q3Shader::CULL_NONE;
		}

		else {
			// add new section
			fill.blocks.push_back(Q3Shader::ShaderDataBlock());
			curData = &fill.blocks.back();

			// get the name of this section
			curData->name = GetNextToken(buff);
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Load a Quake 3 skin
void Q3Shader::LoadSkin(SkinData& fill, const std::string& pFile,IOSystem* io)
{
	boost::scoped_ptr<IOStream> file( io->Open( pFile, "rt"));
	if (!file.get())
		return; // if we can't access the file, don't worry and return

	DefaultLogger::get()->info("Loading Quake3 skin file " + pFile);

	// read file in memory
	const size_t s = file->FileSize();
	std::vector<char> _buff(s+1);const char* buff = &_buff[0];
	file->Read(&_buff[0],s,1);
	_buff[s] = 0;

	// remove commas
	std::replace(_buff.begin(),_buff.end(),',',' ');

	// read token by token and fill output table
	for (;*buff;) {
		SkipSpacesAndLineEnd(&buff);

		// get first identifier
		std::string ss = GetNextToken(buff);
		
		// ignore tokens starting with tag_
		if (!::strncmp(&ss[0],"_tag",std::min((size_t)4, ss.length())))
			continue;

		fill.textures.push_back(SkinData::TextureEntry());
		SkinData::TextureEntry& s = fill.textures.back();

		s.first  = ss;
		s.second = GetNextToken(buff);
	}
}

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
MD3Importer::MD3Importer()
: configFrameID  (0)
, configHandleMP (true)
{}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
MD3Importer::~MD3Importer()
{}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool MD3Importer::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	// simple check of file extension is enough for the moment
	std::string::size_type pos = pFile.find_last_of('.');
	// no file extension - can't read
	if( pos == std::string::npos)
		return false;
	std::string extension = pFile.substr( pos);
	for( std::string::iterator it = extension.begin(); it != extension.end(); ++it)
		*it = tolower( *it);

	return ( extension == ".md3");
}

// ------------------------------------------------------------------------------------------------
void MD3Importer::ValidateHeaderOffsets()
{
	// Check magic number
	if (pcHeader->IDENT != AI_MD3_MAGIC_NUMBER_BE &&
		pcHeader->IDENT != AI_MD3_MAGIC_NUMBER_LE)
			throw new ImportErrorException( "Invalid MD3 file: Magic bytes not found");

	// Check file format version
	if (pcHeader->VERSION > 15)
		DefaultLogger::get()->warn( "Unsupported MD3 file version. Continuing happily ...");

	// Check some offset values whether they are valid
	if (!pcHeader->NUM_SURFACES)
		throw new ImportErrorException( "Invalid md3 file: NUM_SURFACES is 0");

	if (pcHeader->OFS_FRAMES >= fileSize || pcHeader->OFS_SURFACES >= fileSize || 
		pcHeader->OFS_EOF > fileSize) {
		throw new ImportErrorException("Invalid MD3 header: some offsets are outside the file");
	}

	if (pcHeader->NUM_FRAMES <= configFrameID )
		throw new ImportErrorException("The requested frame is not existing the file");
}

// ------------------------------------------------------------------------------------------------
void MD3Importer::ValidateSurfaceHeaderOffsets(const MD3::Surface* pcSurf)
{
	// Calculate the relative offset of the surface
	const int32_t ofs = int32_t((const unsigned char*)pcSurf-this->mBuffer);

	// Check whether all data chunks are inside the valid range
	if (pcSurf->OFS_TRIANGLES + ofs + pcSurf->NUM_TRIANGLES * sizeof(MD3::Triangle)	> fileSize  ||
		pcSurf->OFS_SHADERS + ofs + pcSurf->NUM_SHADER * sizeof(MD3::Shader) > fileSize         ||
		pcSurf->OFS_ST + ofs + pcSurf->NUM_VERTICES * sizeof(MD3::TexCoord) > fileSize          ||
		pcSurf->OFS_XYZNORMAL + ofs + pcSurf->NUM_VERTICES * sizeof(MD3::Vertex) > fileSize)	{

		throw new ImportErrorException("Invalid MD3 surface header: some offsets are outside the file");
	}

	// Check whether all requirements for Q3 files are met. We don't
	// care, but probably someone does.
	if (pcSurf->NUM_TRIANGLES > AI_MD3_MAX_TRIANGLES)
		DefaultLogger::get()->warn("MD3: Quake III triangle limit exceeded");
	if (pcSurf->NUM_SHADER > AI_MD3_MAX_SHADERS)
		DefaultLogger::get()->warn("MD3: Quake III shader limit exceeded");
	if (pcSurf->NUM_VERTICES > AI_MD3_MAX_VERTS)
		DefaultLogger::get()->warn("MD3: Quake III vertex limit exceeded");
	if (pcSurf->NUM_FRAMES > AI_MD3_MAX_FRAMES)
		DefaultLogger::get()->warn("MD3: Quake III frame limit exceeded");
}

// ------------------------------------------------------------------------------------------------
void MD3Importer::GetExtensionList(std::string& append)
{
	append.append("*.md3");
}

// ------------------------------------------------------------------------------------------------
// Setup configuration properties
void MD3Importer::SetupProperties(const Importer* pImp)
{
	// The 
	// AI_CONFIG_IMPORT_MD3_KEYFRAME option overrides the
	// AI_CONFIG_IMPORT_GLOBAL_KEYFRAME option.
	configFrameID = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_MD3_KEYFRAME,0xffffffff);
	if(0xffffffff == configFrameID) {
		configFrameID = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_GLOBAL_KEYFRAME,0);
	}

	// AI_CONFIG_IMPORT_MD3_HANDLE_MULTIPART
	configHandleMP = (0 != pImp->GetPropertyInteger(AI_CONFIG_IMPORT_MD3_HANDLE_MULTIPART,1));

	// AI_CONFIG_IMPORT_MD3_SKIN_NAME
	configSkinFile = (pImp->GetPropertyString(AI_CONFIG_IMPORT_MD3_SKIN_NAME,"default"));
}

// ------------------------------------------------------------------------------------------------
// Try to read the skin for a MD3 file
void MD3Importer::ReadSkin(Q3Shader::SkinData& fill)
{
	// skip any postfixes (e.g. lower_1.md3)
	std::string::size_type s = filename.find_last_of('_');
	if (s == std::string::npos) {
		s = filename.find_last_of('.');
	}
	ai_assert(s != std::string::npos);

	const std::string skin_file = path + filename.substr(0,s) + "_" + configSkinFile + ".skin";
	Q3Shader::LoadSkin(fill,skin_file,mIOHandler);
}

// ------------------------------------------------------------------------------------------------
// Read a multi-part Q3 player model
bool MD3Importer::ReadMultipartFile()
{
	// check whether the file name contains a common postfix, e.g lower_2.md3
	std::string::size_type s = filename.find_last_of('_'), t = filename.find_last_of('.');
	ai_assert(t != std::string::npos);
	if (s == std::string::npos)
		s = t;

	const std::string mod_filename = filename.substr(0,s);
	const std::string suffix = filename.substr(s,t-s);

	if (mod_filename == "lower" || mod_filename == "upper" || mod_filename == "head"){
		const std::string lower = path + "lower" + suffix + ".md3";
		const std::string upper = path + "upper" + suffix + ".md3";
		const std::string head  = path + "head"  + suffix + ".md3";

		aiScene* scene_upper = NULL;
		aiScene* scene_lower = NULL;
		aiScene* scene_head = NULL;
		std::string failure;

		aiNode* tag_torso, *tag_head;
		std::vector<AttachmentInfo> attach;

		DefaultLogger::get()->info("Multi-part MD3 player model: lower, upper and head parts are joined");

		// ensure we won't try to load ourselves recursively
		BatchLoader::PropertyMap props;
		SetGenericProperty( props.ints, AI_CONFIG_IMPORT_MD3_HANDLE_MULTIPART, 0, NULL);

		// now read these three files
		BatchLoader batch(mIOHandler);
		batch.AddLoadRequest(lower,0,&props);
		batch.AddLoadRequest(upper,0,&props);
		batch.AddLoadRequest(head,0,&props);
		batch.LoadAll();

		// now construct a dummy scene to place these three parts in
		aiScene* master   = new aiScene();
		aiNode* nd = master->mRootNode = new aiNode();
		nd->mName.Set("<M3D_Player>");

		// ... and get them. We need all of them.
		scene_lower = batch.GetImport(lower);
		if (!scene_lower) {
			DefaultLogger::get()->error("M3D: Failed to read multipart model, lower.md3 fails to load");
			failure = "lower";
			goto error_cleanup;
		}

		scene_upper = batch.GetImport(upper);
		if (!scene_upper) {
			DefaultLogger::get()->error("M3D: Failed to read multipart model, upper.md3 fails to load");
			failure = "upper";
			goto error_cleanup;
		}

		scene_head  = batch.GetImport(head);
		if (!scene_head) {
			DefaultLogger::get()->error("M3D: Failed to read multipart model, head.md3 fails to load");
			failure = "head";
			goto error_cleanup;
		}

		// build attachment infos. search for typical Q3 tags

		// original root
		attach.push_back(AttachmentInfo(scene_lower, nd));

		// tag_torso
		tag_torso = scene_lower->mRootNode->FindNode("tag_torso");
		if (!tag_torso) {
			DefaultLogger::get()->error("M3D: Failed to find attachment tag for multipart model: tag_torso expected");
			goto error_cleanup;
		}
		attach.push_back(AttachmentInfo(scene_upper,tag_torso));

		// tag_head
		tag_head = scene_upper->mRootNode->FindNode("tag_head");
		if (!tag_head) {
			DefaultLogger::get()->error("M3D: Failed to find attachment tag for multipart model: tag_head expected");
			goto error_cleanup;
		}
		attach.push_back(AttachmentInfo(scene_head,tag_head));

		// and merge the scenes
		SceneCombiner::MergeScenes(&mScene,master, attach,
			AI_INT_MERGE_SCENE_GEN_UNIQUE_NAMES |
			AI_INT_MERGE_SCENE_GEN_UNIQUE_MATNAMES |
			AI_INT_MERGE_SCENE_RESOLVE_CROSS_ATTACHMENTS);

		return true;

error_cleanup:
		delete scene_upper;
		delete scene_lower;
		delete scene_head;
		delete master;

		if (failure == mod_filename) {
			throw new ImportErrorException("MD3: failure to read multipart host file");
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void MD3Importer::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	mFile = pFile;
	mScene = pScene;
	mIOHandler = pIOHandler;

	// get base path and file name
	// todo ... move to PathConverter
	std::string::size_type s = mFile.find_last_of('/');
	if (s == std::string::npos) {
		s = mFile.find_last_of('\\');
	}
	if (s == std::string::npos) {
		s = 0;
	}
	else ++s;
	filename = mFile.substr(s), path = mFile.substr(0,s);
	for( std::string::iterator it = filename .begin(); it != filename.end(); ++it)
		*it = tolower( *it);

	// Load multi-part model file, if necessary
	if (configHandleMP) {
		if (ReadMultipartFile())
			return;
	}

	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open MD3 file " + pFile + ".");

	// Check whether the md3 file is large enough to contain the header
	fileSize = (unsigned int)file->FileSize();
	if( fileSize < sizeof(MD3::Header))
		throw new ImportErrorException( "MD3 File is too small.");

	// Allocate storage and copy the contents of the file to a memory buffer
	std::vector<unsigned char> mBuffer2 (fileSize);
	file->Read( &mBuffer2[0], 1, fileSize);
	mBuffer = &mBuffer2[0];

	pcHeader = (BE_NCONST MD3::Header*)mBuffer;

	// Ensure correct endianess
#ifdef AI_BUILD_BIG_ENDIAN

	AI_SWAP4(pcHeader->VERSION);
	AI_SWAP4(pcHeader->FLAGS);
	AI_SWAP4(pcHeader->IDENT);
	AI_SWAP4(pcHeader->NUM_FRAMES);
	AI_SWAP4(pcHeader->NUM_SKINS);
	AI_SWAP4(pcHeader->NUM_SURFACES);
	AI_SWAP4(pcHeader->NUM_TAGS);
	AI_SWAP4(pcHeader->OFS_EOF);
	AI_SWAP4(pcHeader->OFS_FRAMES);
	AI_SWAP4(pcHeader->OFS_SURFACES);
	AI_SWAP4(pcHeader->OFS_TAGS);

#endif

	// Validate the file header
	ValidateHeaderOffsets();

	// Navigate to the list of surfaces
	BE_NCONST MD3::Surface* pcSurfaces = (BE_NCONST MD3::Surface*)(mBuffer + pcHeader->OFS_SURFACES);

	// Navigate to the list of tags
	BE_NCONST MD3::Tag* pcTags = (BE_NCONST MD3::Tag*)(mBuffer + pcHeader->OFS_TAGS);

	// Allocate output storage
	pScene->mNumMeshes = pcHeader->NUM_SURFACES;
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];

	pScene->mNumMaterials = pcHeader->NUM_SURFACES;
	pScene->mMaterials = new aiMaterial*[pScene->mNumMeshes];

	// Set arrays to zero to ensue proper destruction if an exception is raised
	::memset(pScene->mMeshes,0,pScene->mNumMeshes*sizeof(aiMesh*));
	::memset(pScene->mMaterials,0,pScene->mNumMaterials*sizeof(aiMaterial*));

	// Now read possible skins from .skin file
	Q3Shader::SkinData skins;
	ReadSkin(skins);

	// Read all surfaces from the file
	unsigned int iNum = pcHeader->NUM_SURFACES;
	unsigned int iNumMaterials = 0;
	unsigned int iDefaultMatIndex = 0xFFFFFFFF;
	while (iNum-- > 0)
	{

		// Ensure correct endianess
#ifdef AI_BUILD_BIG_ENDIAN

		AI_SWAP4(pcSurfaces->FLAGS);
		AI_SWAP4(pcSurfaces->IDENT);
		AI_SWAP4(pcSurfaces->NUM_FRAMES);
		AI_SWAP4(pcSurfaces->NUM_SHADER);
		AI_SWAP4(pcSurfaces->NUM_TRIANGLES);
		AI_SWAP4(pcSurfaces->NUM_VERTICES);
		AI_SWAP4(pcSurfaces->OFS_END);
		AI_SWAP4(pcSurfaces->OFS_SHADERS);
		AI_SWAP4(pcSurfaces->OFS_ST);
		AI_SWAP4(pcSurfaces->OFS_TRIANGLES);
		AI_SWAP4(pcSurfaces->OFS_XYZNORMAL);

#endif

		// Validate the surface header
		ValidateSurfaceHeaderOffsets(pcSurfaces);

		// Navigate to the vertex list of the surface
		BE_NCONST MD3::Vertex* pcVertices = (BE_NCONST MD3::Vertex*)
			(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_XYZNORMAL);

		// Navigate to the triangle list of the surface
		BE_NCONST MD3::Triangle* pcTriangles = (BE_NCONST MD3::Triangle*)
			(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_TRIANGLES);

		// Navigate to the texture coordinate list of the surface
		BE_NCONST MD3::TexCoord* pcUVs = (BE_NCONST MD3::TexCoord*)
			(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_ST);

		// Navigate to the shader list of the surface
		BE_NCONST MD3::Shader* pcShaders = (BE_NCONST MD3::Shader*)
			(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_SHADERS);

		// If the submesh is empty ignore it
		if (0 == pcSurfaces->NUM_VERTICES || 0 == pcSurfaces->NUM_TRIANGLES)
		{
			pcSurfaces = (BE_NCONST MD3::Surface*)(((uint8_t*)pcSurfaces) + pcSurfaces->OFS_END);
			pScene->mNumMeshes--;
			continue;
		}

		// Ensure correct endianess
#ifdef AI_BUILD_BIG_ENDIAN

		for (uint32_t i = 0; i < pcSurfaces->NUM_VERTICES;++i)
		{
			AI_SWAP2( pcVertices[i].NORMAL );
			AI_SWAP2( pcVertices[i].X );
			AI_SWAP2( pcVertices[i].Y );
			AI_SWAP2( pcVertices[i].Z );

			AI_SWAP4( pcUVs[i].U );
			AI_SWAP4( pcUVs[i].U );
		}
		for (uint32_t i = 0; i < pcSurfaces->NUM_TRIANGLES;++i)
		{
			AI_SWAP4(pcTriangles[i].INDEXES[0]);
			AI_SWAP4(pcTriangles[i].INDEXES[1]);
			AI_SWAP4(pcTriangles[i].INDEXES[2]);
		}

#endif

		// Allocate the output mesh
		pScene->mMeshes[iNum] = new aiMesh();
		aiMesh* pcMesh = pScene->mMeshes[iNum];
		pcMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

		pcMesh->mNumVertices		= pcSurfaces->NUM_TRIANGLES*3;
		pcMesh->mNumFaces			= pcSurfaces->NUM_TRIANGLES;
		pcMesh->mFaces				= new aiFace[pcSurfaces->NUM_TRIANGLES];
		pcMesh->mNormals			= new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mVertices			= new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mTextureCoords[0]	= new aiVector3D[pcMesh->mNumVertices];
		pcMesh->mNumUVComponents[0] = 2;

		// Fill in all triangles
		unsigned int iCurrent = 0;
		for (unsigned int i = 0; i < (unsigned int)pcSurfaces->NUM_TRIANGLES;++i)
		{
			pcMesh->mFaces[i].mIndices = new unsigned int[3];
			pcMesh->mFaces[i].mNumIndices = 3;

			unsigned int iTemp = iCurrent;
			for (unsigned int c = 0; c < 3;++c,++iCurrent)
			{
				// Read vertices
				pcMesh->mVertices[iCurrent].x = pcVertices[ pcTriangles->INDEXES[c]].X*AI_MD3_XYZ_SCALE;
				pcMesh->mVertices[iCurrent].y = pcVertices[ pcTriangles->INDEXES[c]].Y*AI_MD3_XYZ_SCALE;
				pcMesh->mVertices[iCurrent].z = pcVertices[ pcTriangles->INDEXES[c]].Z*AI_MD3_XYZ_SCALE;

				// Convert the normal vector to uncompressed float3 format
				LatLngNormalToVec3(pcVertices[pcTriangles->INDEXES[c]].NORMAL,
					(float*)&pcMesh->mNormals[iCurrent]);

				// Read texture coordinates
				pcMesh->mTextureCoords[0][iCurrent].x = pcUVs[ pcTriangles->INDEXES[c]].U;
				pcMesh->mTextureCoords[0][iCurrent].y = 1.0f-pcUVs[ pcTriangles->INDEXES[c]].V;
			}
			// FIX: flip the face ordering for use with OpenGL
			pcMesh->mFaces[i].mIndices[0] = iTemp+2;
			pcMesh->mFaces[i].mIndices[1] = iTemp+1;
			pcMesh->mFaces[i].mIndices[2] = iTemp+0;
			pcTriangles++;
		}

		std::string _texture_name;
		const char* texture_name = NULL, *header_name = pcHeader->NAME;

		// Check whether we have a texture record for this surface in the .skin file
		std::list< Q3Shader::SkinData::TextureEntry >::iterator it = std::find( 
			skins.textures.begin(), skins.textures.end(), pcSurfaces->NAME );

		if (it != skins.textures.end()) {
			texture_name = &*( _texture_name = (*it).second).begin();
			DefaultLogger::get()->debug("MD3: Assigning skin texture " + (*it).second + " to surface " + pcSurfaces->NAME);
			(*it).resolved = true; // mark entry as resolved
		}

		// Get the first shader (= texture?) assigned to the surface
		if (!texture_name && pcSurfaces->NUM_SHADER)	{
			texture_name = pcShaders->NAME;
		}

		const char* end2 = NULL;
		if (texture_name) {

			// If the MD3's internal path itself and the given path are using
			// the same directory, remove it completely to get right output paths.
			const char* end1 = ::strrchr(header_name,'\\');
			if (!end1)end1   = ::strrchr(header_name,'/');

			end2 = ::strrchr(texture_name,'\\');
			if (!end2)end2   = ::strrchr(texture_name,'/');

			// HACK: If the paths starts with "models/players", ignore the
			// next hierarchy level, it specifies just the model name.
			// Ignored by Q3, it might be not equal to the real model location.
			if (end1 && end2)	{

				size_t len2;
				const size_t len1 = (size_t)(end1 - header_name);
				if (!ASSIMP_strincmp(header_name,"models/players/",15)) {
					len2 = 15;
				}
				else len2 = std::min (len1, (size_t)(end2 - texture_name ));

				if (!ASSIMP_strincmp(texture_name,header_name,len2)) {
					// Use the file name only
					end2++;
				}
				else {
					// Use the full path
					end2 = (const char*)texture_name;
				}
			}
		}

		MaterialHelper* pcHelper = new MaterialHelper();

		// Setup dummy texture file name to ensure UV coordinates are kept during postprocessing
		aiString szString;
		if (end2 && end2[0])	{
			const size_t iLen = ::strlen(end2);
			::memcpy(szString.data,end2,iLen);
			szString.data[iLen] = '\0';
			szString.length = iLen;
		}
		else	{
			DefaultLogger::get()->warn("Texture file name has zero length. Using default name");
			szString.Set("dummy_texture.bmp");
		}
		pcHelper->AddProperty(&szString,AI_MATKEY_TEXTURE_DIFFUSE(0));

		const int iMode = (int)aiShadingMode_Gouraud;
		pcHelper->AddProperty<int>(&iMode, 1, AI_MATKEY_SHADING_MODEL);

		// Add a small ambient color value - Quake 3 seems to have one
		aiColor3D clr;
		clr.b = clr.g = clr.r = 0.05f;
		pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_AMBIENT);

		clr.b = clr.g = clr.r = 1.0f;
		pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_DIFFUSE);
		pcHelper->AddProperty<aiColor3D>(&clr, 1,AI_MATKEY_COLOR_SPECULAR);

		// use surface name + skin_name as material name
		aiString name;
		name.Set("MD3_[" + configSkinFile + "][" + pcSurfaces->NAME + "]");
		pcHelper->AddProperty(&name,AI_MATKEY_NAME);

		pScene->mMaterials[iNumMaterials] = (aiMaterial*)pcHelper;
		pcMesh->mMaterialIndex = iNumMaterials++;
	
		// Go to the next surface
		pcSurfaces = (BE_NCONST MD3::Surface*)(((unsigned char*)pcSurfaces) + pcSurfaces->OFS_END);
	}

	// For debugging purposes: check whether we found matches for all entries in the skins file
	if (!DefaultLogger::isNullLogger()) {
		for (std::list< Q3Shader::SkinData::TextureEntry>::const_iterator it = skins.textures.begin();it != skins.textures.end(); ++it) {
			if (!(*it).resolved) {
				DefaultLogger::get()->error("MD3: Failed to match skin " + (*it).first + " to surface " + (*it).second);
			}
		}
	}

	if (!pScene->mNumMeshes)
		throw new ImportErrorException( "MD3: File contains no valid mesh");
	pScene->mNumMaterials = iNumMaterials;

	// Now we need to generate an empty node graph
	pScene->mRootNode = new aiNode("<MD3Root>");
	pScene->mRootNode->mNumMeshes = pScene->mNumMeshes;
	pScene->mRootNode->mMeshes = new unsigned int[pScene->mNumMeshes];

	// Attach tiny children for all tags
	if (pcHeader->NUM_TAGS) {
		pScene->mRootNode->mNumChildren = pcHeader->NUM_TAGS;
		pScene->mRootNode->mChildren = new aiNode*[pcHeader->NUM_TAGS];

		for (unsigned int i = 0; i < pcHeader->NUM_TAGS; ++i, ++pcTags) {

			aiNode* nd = pScene->mRootNode->mChildren[i] = new aiNode();
			nd->mName.Set((const char*)pcTags->NAME);
			nd->mParent = pScene->mRootNode;

			AI_SWAP4(pcTags->origin.x);
			AI_SWAP4(pcTags->origin.y);
			AI_SWAP4(pcTags->origin.z);

			// Copy local origin
			nd->mTransformation.a4 = pcTags->origin.x;
			nd->mTransformation.b4 = pcTags->origin.y;
			nd->mTransformation.c4 = pcTags->origin.z;

			// Copy rest of transformation (need to transpose to match row-order matrix)
			for (unsigned int a = 0; a < 3;++a) {
				for (unsigned int m = 0; m < 3;++m) {
					nd->mTransformation[m][a] = pcTags->orientation[a][m];
					AI_SWAP4(nd->mTransformation[m][a]);
				}
			}
		}
	}

	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		pScene->mRootNode->mMeshes[i] = i;
}

#endif // !! ASSIMP_BUILD_NO_MD3_IMPORTER
