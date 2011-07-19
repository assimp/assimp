/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
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

	WriteGeometryLibrary();

	WriteSceneLibrary();

	// useless Collada bullshit at the end, just in case we haven't had enough indirections, yet. 
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
	mOutput << startstr << "<unit meter=\"1.0\" name=\"meter\" />" << endstr;
	mOutput << startstr << "<up_axis>Y_UP</up_axis>" << endstr;
	PopTag();
	mOutput << startstr << "</asset>" << endstr;
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
			mOutput << startstr << "<input semantic=\"TEXCOORD\" source=\"#" << idstr << "-tex" << a << "\" set=\"" << a << "\" />" << endstr;
	}
	for( size_t a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; ++a )
	{
		if( mesh->HasVertexColors( a) )
			mOutput << startstr << "<input semantic=\"COLOR\" source=\"#" << idstr << "-color" << a << "\" set=\"" << a << "\" />" << endstr;
	}
	
	PopTag();
	mOutput << startstr << "</vertices>" << endstr;

	// write face setup
	mOutput << startstr << "<polylist count=\"" << mesh->mNumFaces << "\" material=\"tellme\">" << endstr;
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

	// the usual Collada bullshit. Let's bloat it even more!
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
		// const aiMesh* mesh = mScene->mMeshes[pNode->mMeshes[a]];
		mOutput << startstr << "<instance_geometry url=\"#" << GetMeshId( a) << "\">" << endstr;
		PushTag();

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

