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

/** @file ColladaExporter.h
 * Declares the exporter class to write a scene to a Collada file
 */
#ifndef AI_COLLADAEXPORTER_H_INC
#define AI_COLLADAEXPORTER_H_INC

#include "../include/assimp/ai_assert.h"
#include <sstream>

struct aiScene;
struct aiNode;

namespace Assimp	
{

/// Helper class to export a given scene to a Collada file. Just for my personal
/// comfort when implementing it.
class ColladaExporter
{
public:
	/// Constructor for a specific scene to export
	ColladaExporter( const aiScene* pScene);

protected:
	/// Starts writing the contents
	void WriteFile();

	/// Writes the asset header
	void WriteHeader();

  /// Writes the material setup
  void WriteMaterials();

	/// Writes the geometry library
	void WriteGeometryLibrary();

	/// Writes the given mesh
	void WriteGeometry( size_t pIndex);

	enum FloatDataType { FloatType_Vector, FloatType_TexCoord2, FloatType_TexCoord3, FloatType_Color };

	/// Writes a float array of the given type
	void WriteFloatArray( const std::string& pIdString, FloatDataType pType, const float* pData, size_t pElementCount);

	/// Writes the scene library
	void WriteSceneLibrary();

	/// Recursively writes the given node
	void WriteNode( const aiNode* pNode);

	/// Enters a new xml element, which increases the indentation
	void PushTag() { startstr.append( "  "); }
	/// Leaves an element, decreasing the indentation
	void PopTag() { ai_assert( startstr.length() > 1); startstr.erase( startstr.length() - 2); }

	/// Creates a mesh ID for the given mesh
	std::string GetMeshId( size_t pIndex) const { return std::string( "meshId" ) + boost::lexical_cast<std::string> (pIndex); }

public:
	/// Stringstream to write all output into
	std::stringstream mOutput;

protected:
	/// The scene to be written
	const aiScene* mScene;

	/// current line start string, contains the current indentation for simple stream insertion
	std::string startstr;
	/// current line end string for simple stream insertion
	std::string endstr;

  // pair of color and texture - texture precedences color
  struct Surface 
  { 
    aiColor4D color; 
    std::string texture; 
    size_t channel; 
    Surface() { channel = 0; }
  };

  // summarize a material in an convinient way. 
  struct Material
  {
    std::string name;
    Surface ambient, diffuse, specular, emissive, reflective, normal;
    float shininess; /// specular exponent

    Material() { shininess = 16.0f; }
  };

  std::vector<Material> materials;

protected:
  /// Dammit C++ - y u no compile two-pass? No I have to add all methods below the struct definitions
  /// Reads a single surface entry from the given material keys
  void ReadMaterialSurface( Surface& poSurface, const aiMaterial* pSrcMat, aiTextureType pTexture, const char* pKey, size_t pType, size_t pIndex);
  /// Writes an image entry for the given surface
  void WriteImageEntry( const Surface& pSurface, const std::string& pNameAdd);
  /// Writes the two parameters necessary for referencing a texture in an effect entry
  void WriteTextureParamEntry( const Surface& pSurface, const std::string& pTypeName, const std::string& pMatName);
  /// Writes a color-or-texture entry into an effect definition
  void WriteTextureColorEntry( const Surface& pSurface, const std::string& pTypeName, const std::string& pImageName);
};

}

#endif // !! AI_COLLADAEXPORTER_H_INC
