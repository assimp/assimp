/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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
#ifndef OBJ_FILEPARSER_H_INC
#define OBJ_FILEPARSER_H_INC

#include "ObjFileData.h"

#include <assimp/IOStreamBuffer.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/vector2.h>
#include <assimp/vector3.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Assimp {

// Forward declarations
class ObjFileImporter;
class IOSystem;
class ProgressHandler;

// ------------------------------------------------------------------------------------------------
/// \class  ObjFileParser
/// \brief  Parser for a obj waveform file
// ------------------------------------------------------------------------------------------------
class ASSIMP_API ObjFileParser {
public:
    static constexpr size_t Buffersize = 4096;
    using DataArray = std::vector<char>;
    using DataArrayIt = std::vector<char>::iterator;
    using ConstDataArrayIt = std::vector<char>::const_iterator;

    /// @brief  The default constructor.
    ObjFileParser();
    /// @brief  Constructor with data array.
    ObjFileParser(IOStreamBuffer<char> &streamBuffer, const std::string &modelName, IOSystem *io, ProgressHandler *progress, const std::string &originalObjFileName);
    /// @brief  Destructor
    ~ObjFileParser() = default;
    /// @brief  If you want to load in-core data.
    void setBuffer(std::vector<char> &buffer);
    /// @brief  Model getter.
    ObjFile::Model *GetModel() const;

    ObjFileParser(const ObjFileParser&) = delete;
    ObjFileParser &operator=(const ObjFileParser& ) = delete;

protected:
    /// Parse the loaded file
    void parseFile(IOStreamBuffer<char> &streamBuffer);
    /// Method to copy the new delimited word in the current line.
    void copyNextWord(char *pBuffer, size_t length);
    /// Get the number of components in a line.
    size_t getNumComponentsInDataDefinition();
    /// Stores the vector
    size_t getTexCoordVector(std::vector<aiVector3D> &point3d_array);
    /// Stores the following 3d vector.
    void getVector3(std::vector<aiVector3D> &point3d_array);
    /// Stores the following homogeneous vector as a 3D vector
    void getHomogeneousVector3(std::vector<aiVector3D> &point3d_array);
    /// Stores the following two 3d vectors on the line.
    void getTwoVectors3(std::vector<aiVector3D> &point3d_array_a, std::vector<aiVector3D> &point3d_array_b);
    /// Stores the following 3d vector.
    void getVector2(std::vector<aiVector2D> &point2d_array);
    /// Stores the following face.
    void getFace(aiPrimitiveType type);
    /// Reads the material description.
    void getMaterialDesc();
    /// Gets a comment.
    void getComment();
    /// Gets a a material library.
    void getMaterialLib();
    /// Creates a new material.
    void getNewMaterial();
    /// Gets the group name from file.
    void getGroupName();
    /// Gets the group number from file.
    void getGroupNumber();
    /// Gets the group number and resolution from file.
    void getGroupNumberAndResolution();
    /// Returns the index of the material. Is -1 if not material was found.
    int getMaterialIndex(const std::string &strMaterialName);
    /// Parse object name
    void getObjectName();
    /// Creates a new object.
    void createObject(const std::string &strObjectName);
    /// Creates a new mesh.
    void createMesh(const std::string &meshName);
    /// Returns true, if a new mesh instance must be created.
    bool needsNewMesh(const std::string &rMaterialName);
    /// Error report in token
    void reportErrorTokenInFace();

private:
    /// Default material name
    static constexpr const char DEFAULT_MATERIAL[] = AI_DEFAULT_MATERIAL_NAME;
    //! Iterator to current position in buffer
    DataArrayIt m_DataIt;
    //! Iterator to end position of buffer
    DataArrayIt m_DataItEnd;
    //! Pointer to model instance
    std::unique_ptr<ObjFile::Model> m_pModel;
    //! Current line (for debugging)
    unsigned int m_uiLine;
    //! Helper buffer
    char m_buffer[Buffersize];
	/// End of buffer
    const char *mEnd;
    /// Pointer to IO system instance.
    IOSystem *m_pIO;
    //! Pointer to progress handler
    ProgressHandler *m_progress;
    /// Path to the current model, name of the obj file where the buffer comes from
    const std::string m_originalObjFileName;
};

} // Namespace Assimp

#endif
