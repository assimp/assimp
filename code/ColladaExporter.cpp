/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

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

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_COLLADA_EXPORTER
#include "ColladaExporter.h"

using namespace Assimp;

namespace Assimp
{

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to Collada. Prototyped and registered in Exporter.cpp
void ExportSceneCollada(const char* pFile,IOSystem* pIOSystem, const aiScene* pScene)
{
	// invoke the exporter 
	ColladaExporter iDoTheExportThing( pScene);

	// we're still here - export successfully completed. Write result to the given IOSYstem
	boost::scoped_ptr<IOStream> outfile (pIOSystem->Open(pFile,"wt"));
	if(outfile == NULL) {
		throw DeadlyExportError("could not open output .dae file: " + std::string(pFile));
	}

	// XXX maybe use a small wrapper around IOStream that behaves like std::stringstream in order to avoid the extra copy.
	outfile->Write( iDoTheExportThing.mOutput.str().c_str(), static_cast<size_t>(iDoTheExportThing.mOutput.tellp()),1);
}

} // end of namespace Assimp


// ------------------------------------------------------------------------------------------------
// Constructor for a specific scene to export
ColladaExporter::ColladaExporter( const aiScene* pScene)
{
	// make sure that all formatting happens using the standard, C locale and not the user's current locale
	mOutput.imbue( std::locale("C") );

	mScene = pScene;

	// set up strings
	endstr = "\n"; 

	// start writing
	WriteFile();
}

// ------------------------------------------------------------------------------------------------
// Starts writing the contents
void ColladaExporter::WriteFile()
{
	// write the DTD
	mOutput << "<?xml version=\"1.0\"?>" << endstr;
	// COLLADA element start
	mOutput << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">" << endstr;
	PushTag();

	WriteHeader();

  WriteMaterials();
	WriteGeometryLibrary();

	WriteSceneLibrary();

	// useless Collada fu at the end, just in case we haven't had enough indirections, yet. 
	mOutput << startstr << "<scene>" << endstr;
	PushTag();
	mOutput << startstr << "<instance_visual_scene url=\"#myScene\" />" << endstr;
	PopTag();
	mOutput << startstr << "</scene>" << endstr;
	PopTag();
	mOutput << "</COLLADA>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the asset header
void ColladaExporter::WriteHeader()
{
	// Dummy stuff. Nobody actually cares for it anyways
	mOutput << startstr << "<asset>" << endstr;
	PushTag();
	mOutput << startstr << "<contributor>" << endstr;
	PushTag();
	mOutput << startstr << "<author>Someone</author>" << endstr;
	mOutput << startstr << "<authoring_tool>Assimp Collada Exporter</authoring_tool>" << endstr;
	PopTag();
	mOutput << startstr << "</contributor>" << endstr;
  mOutput << startstr << "<created>2000-01-01T23:59:59</created>" << endstr;
  mOutput << startstr << "<modified>2000-01-01T23:59:59</modified>" << endstr;
	mOutput << startstr << "<unit name=\"centimeter\" meter=\"0.01\" />" << endstr;
	mOutput << startstr << "<up_axis>Y_UP</up_axis>" << endstr;
	PopTag();
	mOutput << startstr << "</asset>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Reads a single surface entry from the given material keys
void ColladaExporter::ReadMaterialSurface( Surface& poSurface, const aiMaterial* pSrcMat, aiTextureType pTexture, const char* pKey, size_t pType, size_t pIndex)
{
  if( pSrcMat->GetTextureCount( pTexture) > 0 )
  {
    aiString texfile;
    unsigned int uvChannel = 0;
    pSrcMat->GetTexture( pTexture, 0, &texfile, NULL, &uvChannel);
    poSurface.texture = texfile.C_Str();
    poSurface.channel = uvChannel;
  } else
  {
    if( pKey )
      pSrcMat->Get( pKey, pType, pIndex, poSurface.color);
  }
}

// ------------------------------------------------------------------------------------------------
// Writes an image entry for the given surface
void ColladaExporter::WriteImageEntry( const Surface& pSurface, const std::string& pNameAdd)
{
  if( !pSurface.texture.empty() )
  {
    mOutput << startstr << "<image id=\"" << pNameAdd << "\">" << endstr;
    PushTag(); 
    mOutput << startstr << "<init_from>";
    for( std::string::const_iterator it = pSurface.texture.begin(); it != pSurface.texture.end(); ++it )
    {
      if( isalnum( *it) || *it == '_' || *it == '.' || *it == '/' || *it == '\\' )
        mOutput << *it;
      else
        mOutput << '%' << std::hex << size_t( (unsigned char) *it) << std::dec;
    }
    mOutput << "</init_from>" << endstr;
    PopTag();
    mOutput << startstr << "</image>" << endstr;
  }
}

// ------------------------------------------------------------------------------------------------
// Writes a color-or-texture entry into an effect definition
void ColladaExporter::WriteTextureColorEntry( const Surface& pSurface, const std::string& pTypeName, const std::string& pImageName)
{
  mOutput << startstr << "<" << pTypeName << ">" << endstr;
  PushTag();
  if( pSurface.texture.empty() )
  {
    mOutput << startstr << "<color sid=\"" << pTypeName << "\">" << pSurface.color.r << "   " << pSurface.color.g << "   " << pSurface.color.b << "   " << pSurface.color.a << "</color>" << endstr;
  } else
  {
    mOutput << startstr << "<texture texture=\"" << pImageName << "\" texcoord=\"CHANNEL" << pSurface.channel << "\" />" << endstr;
  }
  PopTag();
  mOutput << startstr << "</" << pTypeName << ">" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the two parameters necessary for referencing a texture in an effect entry
void ColladaExporter::WriteTextureParamEntry( const Surface& pSurface, const std::string& pTypeName, const std::string& pMatName)
{
  // if surface is a texture, write out the sampler and the surface parameters necessary to reference the texture
  if( !pSurface.texture.empty() )
  {
    mOutput << startstr << "<newparam sid=\"" << pMatName << "-" << pTypeName << "-surface\">" << endstr;
    PushTag();
    mOutput << startstr << "<surface type=\"2D\">" << endstr;
    PushTag();
    mOutput << startstr << "<init_from>" << pMatName << "-" << pTypeName << "-image</init_from>" << endstr;
    PopTag();
    mOutput << startstr << "</surface>" << endstr;
    PopTag();
    mOutput << startstr << "</newparam>" << endstr;

    mOutput << startstr << "<newparam sid=\"" << pMatName << "-" << pTypeName << "-sampler\">" << endstr;
    PushTag();
    mOutput << startstr << "<sampler2D>" << endstr;
    PushTag();
    mOutput << startstr << "<source>" << pMatName << "-" << pTypeName << "-surface</source>" << endstr;
    PopTag();
    mOutput << startstr << "</sampler2D>" << endstr;
    PopTag();
    mOutput << startstr << "</newparam>" << endstr;
  }
}

// ------------------------------------------------------------------------------------------------
// Writes the material setup
void ColladaExporter::WriteMaterials()
{
  materials.resize( mScene->mNumMaterials);

  /// collect all materials from the scene
  size_t numTextures = 0;
  for( size_t a = 0; a < mScene->mNumMaterials; ++a )
  {
    const aiMaterial* mat = mScene->mMaterials[a];

    aiString name;
    if( mat->Get( AI_MATKEY_NAME, name) != aiReturn_SUCCESS )
      name = "mat";
    materials[a].name = std::string( "m") + boost::lexical_cast<std::string> (a) + name.C_Str();
    for( std::string::iterator it = materials[a].name.begin(); it != materials[a].name.end(); ++it ) {
		// isalnum on MSVC asserts for code points in [0,255]. Thus prevent unwanted promotion
		// of char to signed int and take the unsigned char value.
      if( !isalnum( static_cast<uint8_t>(*it) ) ) {
        *it = '_';
	  }
	}

    ReadMaterialSurface( materials[a].ambient, mat, aiTextureType_AMBIENT, AI_MATKEY_COLOR_AMBIENT);
    if( !materials[a].ambient.texture.empty() ) numTextures++;
    ReadMaterialSurface( materials[a].diffuse, mat, aiTextureType_DIFFUSE, AI_MATKEY_COLOR_DIFFUSE);
    if( !materials[a].diffuse.texture.empty() ) numTextures++;
    ReadMaterialSurface( materials[a].specular, mat, aiTextureType_SPECULAR, AI_MATKEY_COLOR_SPECULAR);
    if( !materials[a].specular.texture.empty() ) numTextures++;
    ReadMaterialSurface( materials[a].emissive, mat, aiTextureType_EMISSIVE, AI_MATKEY_COLOR_EMISSIVE);
    if( !materials[a].emissive.texture.empty() ) numTextures++;
    ReadMaterialSurface( materials[a].reflective, mat, aiTextureType_REFLECTION, AI_MATKEY_COLOR_REFLECTIVE);
    if( !materials[a].reflective.texture.empty() ) numTextures++;
    ReadMaterialSurface( materials[a].normal, mat, aiTextureType_NORMALS, NULL, 0, 0);
    if( !materials[a].normal.texture.empty() ) numTextures++;

    mat->Get( AI_MATKEY_SHININESS, materials[a].shininess);
  }

  // output textures if present
  if( numTextures > 0 )
  {
    mOutput << startstr << "<library_images>" << endstr; 
    PushTag();
    for( std::vector<Material>::const_iterator it = materials.begin(); it != materials.end(); ++it )
    { 
      const Material& mat = *it;
      WriteImageEntry( mat.ambient, mat.name + "-ambient-image");
      WriteImageEntry( mat.diffuse, mat.name + "-diffuse-image");
      WriteImageEntry( mat.specular, mat.name + "-specular-image");
      WriteImageEntry( mat.emissive, mat.name + "-emissive-image");
      WriteImageEntry( mat.reflective, mat.name + "-reflective-image");
      WriteImageEntry( mat.normal, mat.name + "-normal-image");
    }
    PopTag();
    mOutput << startstr << "</library_images>" << endstr;
  }

  // output effects - those are the actual carriers of information
  if( !materials.empty() )
  {
    mOutput << startstr << "<library_effects>" << endstr;
    PushTag();
    for( std::vector<Material>::const_iterator it = materials.begin(); it != materials.end(); ++it )
    {
      const Material& mat = *it;
      // this is so ridiculous it must be right
      mOutput << startstr << "<effect id=\"" << mat.name << "-fx\" name=\"" << mat.name << "\">" << endstr;
      PushTag();
      mOutput << startstr << "<profile_COMMON>" << endstr;
      PushTag();

      // write sampler- and surface params for the texture entries
      WriteTextureParamEntry( mat.emissive, "emissive", mat.name);
      WriteTextureParamEntry( mat.ambient, "ambient", mat.name);
      WriteTextureParamEntry( mat.diffuse, "diffuse", mat.name);
      WriteTextureParamEntry( mat.specular, "specular", mat.name);
      WriteTextureParamEntry( mat.reflective, "reflective", mat.name);

      mOutput << startstr << "<technique sid=\"standard\">" << endstr;
      PushTag();
      mOutput << startstr << "<phong>" << endstr;
      PushTag();

      WriteTextureColorEntry( mat.emissive, "emission", mat.name + "-emissive-sampler");
      WriteTextureColorEntry( mat.ambient, "ambient", mat.name + "-ambient-sampler");
      WriteTextureColorEntry( mat.diffuse, "diffuse", mat.name + "-diffuse-sampler");
      WriteTextureColorEntry( mat.specular, "specular", mat.name + "-specular-sampler");

      mOutput << startstr << "<shininess>" << endstr;
      PushTag();
      mOutput << startstr << "<float sid=\"shininess\">" << mat.shininess << "</float>" << endstr;
      PopTag();
      mOutput << startstr << "</shininess>" << endstr;

      WriteTextureColorEntry( mat.reflective, "reflective", mat.name + "-reflective-sampler");

  // deactivated because the Collada spec PHONG model does not allow other textures.
  //    if( !mat.normal.texture.empty() )
  //      WriteTextureColorEntry( mat.normal, "bump", mat.name + "-normal-sampler");


      PopTag();
      mOutput << startstr << "</phong>" << endstr;
      PopTag();
      mOutput << startstr << "</technique>" << endstr;
      PopTag();
      mOutput << startstr << "</profile_COMMON>" << endstr;
      PopTag();
      mOutput << startstr << "</effect>" << endstr;
    }
    PopTag();
    mOutput << startstr << "</library_effects>" << endstr;

    // write materials - they're just effect references
    mOutput << startstr << "<library_materials>" << endstr;
    PushTag();
    for( std::vector<Material>::const_iterator it = materials.begin(); it != materials.end(); ++it )
    {
      const Material& mat = *it;
      mOutput << startstr << "<material id=\"" << mat.name << "\" name=\"" << mat.name << "\">" << endstr;
      PushTag();
      mOutput << startstr << "<instance_effect url=\"#" << mat.name << "-fx\"/>" << endstr;
      PopTag();
      mOutput << startstr << "</material>" << endstr;
    }
    PopTag();
    mOutput << startstr << "</library_materials>" << endstr;
  }
}

// ------------------------------------------------------------------------------------------------
// Writes the geometry library
void ColladaExporter::WriteGeometryLibrary()
{
	mOutput << startstr << "<library_geometries>" << endstr;
	PushTag();

	for( size_t a = 0; a < mScene->mNumMeshes; ++a)
		WriteGeometry( a);

	PopTag();
	mOutput << startstr << "</library_geometries>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the given mesh
void ColladaExporter::WriteGeometry( size_t pIndex)
{
	const aiMesh* mesh = mScene->mMeshes[pIndex];
	std::string idstr = GetMeshId( pIndex);

  if( mesh->mNumFaces == 0 || mesh->mNumVertices == 0 )
    return;

	// opening tag
	mOutput << startstr << "<geometry id=\"" << idstr << "\" name=\"" << idstr << "_name\" >" << endstr;
	PushTag();

	mOutput << startstr << "<mesh>" << endstr;
	PushTag();

	// Positions
	WriteFloatArray( idstr + "-positions", FloatType_Vector, (float*) mesh->mVertices, mesh->mNumVertices);
	// Normals, if any
	if( mesh->HasNormals() )
		WriteFloatArray( idstr + "-normals", FloatType_Vector, (float*) mesh->mNormals, mesh->mNumVertices);

	// texture coords
	for( size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a)
	{
		if( mesh->HasTextureCoords( a) )
		{
			WriteFloatArray( idstr + "-tex" + boost::lexical_cast<std::string> (a), mesh->mNumUVComponents[a] == 3 ? FloatType_TexCoord3 : FloatType_TexCoord2,
				(float*) mesh->mTextureCoords[a], mesh->mNumVertices);
		}
	}

	// vertex colors
	for( size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a)
	{
		if( mesh->HasVertexColors( a) )
			WriteFloatArray( idstr + "-color" + boost::lexical_cast<std::string> (a), FloatType_Color, (float*) mesh->mColors[a], mesh->mNumVertices);
	}

	// assemble vertex structure
	mOutput << startstr << "<vertices id=\"" << idstr << "-vertices" << "\">" << endstr;
	PushTag();
	mOutput << startstr << "<input semantic=\"POSITION\" source=\"#" << idstr << "-positions\" />" << endstr;
	if( mesh->HasNormals() )
		mOutput << startstr << "<input semantic=\"NORMAL\" source=\"#" << idstr << "-normals\" />" << endstr;
	for( size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a )
	{
		if( mesh->HasTextureCoords( a) )
			mOutput << startstr << "<input semantic=\"TEXCOORD\" source=\"#" << idstr << "-tex" << a << "\" " /*<< "set=\"" << a << "\"" */ << " />" << endstr;
	}
	for( size_t a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; ++a )
	{
		if( mesh->HasVertexColors( a) )
			mOutput << startstr << "<input semantic=\"COLOR\" source=\"#" << idstr << "-color" << a << "\" " /*<< set=\"" << a << "\"" */ << " />" << endstr;
	}
	
	PopTag();
	mOutput << startstr << "</vertices>" << endstr;

	// write face setup
	mOutput << startstr << "<polylist count=\"" << mesh->mNumFaces << "\" material=\"theresonlyone\">" << endstr;
	PushTag();
	mOutput << startstr << "<input offset=\"0\" semantic=\"VERTEX\" source=\"#" << idstr << "-vertices\" />" << endstr;
	
	mOutput << startstr << "<vcount>";
	for( size_t a = 0; a < mesh->mNumFaces; ++a )
		mOutput << mesh->mFaces[a].mNumIndices << " ";
	mOutput << "</vcount>" << endstr;
	
	mOutput << startstr << "<p>";
	for( size_t a = 0; a < mesh->mNumFaces; ++a )
	{
		const aiFace& face = mesh->mFaces[a];
		for( size_t b = 0; b < face.mNumIndices; ++b )
			mOutput << face.mIndices[b] << " ";
	}
	mOutput << "</p>" << endstr;
	PopTag();
	mOutput << startstr << "</polylist>" << endstr;

	// closing tags
	PopTag();
	mOutput << startstr << "</mesh>" << endstr;
	PopTag();
	mOutput << startstr << "</geometry>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes a float array of the given type
void ColladaExporter::WriteFloatArray( const std::string& pIdString, FloatDataType pType, const float* pData, size_t pElementCount)
{
	size_t floatsPerElement = 0;
	switch( pType )
	{
		case FloatType_Vector: floatsPerElement = 3; break;
		case FloatType_TexCoord2: floatsPerElement = 2; break;
		case FloatType_TexCoord3: floatsPerElement = 3; break;
		case FloatType_Color: floatsPerElement = 3; break;
		default:
			return;
	}

	std::string arrayId = pIdString + "-array";

	mOutput << startstr << "<source id=\"" << pIdString << "\" name=\"" << pIdString << "\">" << endstr;
	PushTag();

	// source array
	mOutput << startstr << "<float_array id=\"" << arrayId << "\" count=\"" << pElementCount * floatsPerElement << "\"> ";
	PushTag();

	if( pType == FloatType_TexCoord2 )
	{
		for( size_t a = 0; a < pElementCount; ++a )
		{
			mOutput << pData[a*3+0] << " ";
			mOutput << pData[a*3+1] << " ";
		}
	} 
	else if( pType == FloatType_Color )
	{
		for( size_t a = 0; a < pElementCount; ++a )
		{
			mOutput << pData[a*4+0] << " ";
			mOutput << pData[a*4+1] << " ";
			mOutput << pData[a*4+2] << " ";
		}
	}
	else
	{
		for( size_t a = 0; a < pElementCount * floatsPerElement; ++a )
			mOutput << pData[a] << " ";
	}
	mOutput << "</float_array>" << endstr; 
	PopTag();

	// the usual Collada fun. Let's bloat it even more!
	mOutput << startstr << "<technique_common>" << endstr;
	PushTag();
	mOutput << startstr << "<accessor count=\"" << pElementCount << "\" offset=\"0\" source=\"#" << arrayId << "\" stride=\"" << floatsPerElement << "\">" << endstr;
	PushTag();

	switch( pType )
	{
		case FloatType_Vector:
			mOutput << startstr << "<param name=\"X\" type=\"float\" />" << endstr;
			mOutput << startstr << "<param name=\"Y\" type=\"float\" />" << endstr;
			mOutput << startstr << "<param name=\"Z\" type=\"float\" />" << endstr;
			break;

		case FloatType_TexCoord2:
			mOutput << startstr << "<param name=\"S\" type=\"float\" />" << endstr;
			mOutput << startstr << "<param name=\"T\" type=\"float\" />" << endstr;
			break;

		case FloatType_TexCoord3:
			mOutput << startstr << "<param name=\"S\" type=\"float\" />" << endstr;
			mOutput << startstr << "<param name=\"T\" type=\"float\" />" << endstr;
			mOutput << startstr << "<param name=\"P\" type=\"float\" />" << endstr;
			break;

		case FloatType_Color:
			mOutput << startstr << "<param name=\"R\" type=\"float\" />" << endstr;
			mOutput << startstr << "<param name=\"G\" type=\"float\" />" << endstr;
			mOutput << startstr << "<param name=\"B\" type=\"float\" />" << endstr;
			break;
	}

	PopTag();
	mOutput << startstr << "</accessor>" << endstr;
	PopTag();
	mOutput << startstr << "</technique_common>" << endstr;
	PopTag();
	mOutput << startstr << "</source>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the scene library
void ColladaExporter::WriteSceneLibrary()
{
	mOutput << startstr << "<library_visual_scenes>" << endstr;
	PushTag();
	mOutput << startstr << "<visual_scene id=\"myScene\" name=\"myScene\">" << endstr;
	PushTag();

	// start recursive write at the root node
	WriteNode( mScene->mRootNode);

	PopTag();
	mOutput << startstr << "</visual_scene>" << endstr;
	PopTag();
	mOutput << startstr << "</library_visual_scenes>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Recursively writes the given node
void ColladaExporter::WriteNode( const aiNode* pNode)
{
	mOutput << startstr << "<node id=\"" << pNode->mName.data << "\" name=\"" << pNode->mName.data << "\">" << endstr;
	PushTag();

	// write transformation - we can directly put the matrix there
	// TODO: (thom) decompose into scale - rot - quad to allow adressing it by animations afterwards
	const aiMatrix4x4& mat = pNode->mTransformation;
	mOutput << startstr << "<matrix>";
	mOutput << mat.a1 << " " << mat.a2 << " " << mat.a3 << " " << mat.a4 << " ";
	mOutput << mat.b1 << " " << mat.b2 << " " << mat.b3 << " " << mat.b4 << " ";
	mOutput << mat.c1 << " " << mat.c2 << " " << mat.c3 << " " << mat.c4 << " ";
	mOutput << mat.d1 << " " << mat.d2 << " " << mat.d3 << " " << mat.d4;
	mOutput << "</matrix>" << endstr;

	// instance every geometry
	for( size_t a = 0; a < pNode->mNumMeshes; ++a )
	{
		const aiMesh* mesh = mScene->mMeshes[pNode->mMeshes[a]];
    // do not instanciate mesh if empty. I wonder how this could happen
    if( mesh->mNumFaces == 0 || mesh->mNumVertices == 0 )
      continue;

		mOutput << startstr << "<instance_geometry url=\"#" << GetMeshId( pNode->mMeshes[a]) << "\">" << endstr;
		PushTag();
    mOutput << startstr << "<bind_material>" << endstr;
    PushTag();
    mOutput << startstr << "<technique_common>" << endstr;
    PushTag();
    mOutput << startstr << "<instance_material symbol=\"theresonlyone\" target=\"#" << materials[mesh->mMaterialIndex].name << "\" />" << endstr;
		PopTag();
    mOutput << startstr << "</technique_common>" << endstr;
    PopTag();
    mOutput << startstr << "</bind_material>" << endstr;
    PopTag();
		mOutput << startstr << "</instance_geometry>" << endstr;
	}

	// recurse into subnodes
	for( size_t a = 0; a < pNode->mNumChildren; ++a )
		WriteNode( pNode->mChildren[a]);

	PopTag();
	mOutput << startstr << "</node>" << endstr;
}

#endif
#endif

