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

----------------------------------------------------------------------*/
#ifndef OBJFILEMTLIMPORTER_H_INC
#define OBJFILEMTLIMPORTER_H_INC

#include <assimp/defs.h>
#include <string>
#include <vector>
#include "Common/Maybe.h"

struct aiColor3D;
struct aiString;

namespace Assimp {

namespace ObjFile {
struct Model;
struct Material;
} // namespace ObjFile

/**
 *  @class  ObjFileMtlImporter
 *  @brief  Loads the material description from a mtl file.
 */
class ObjFileMtlImporter {
public:
    static const size_t BUFFERSIZE = 2048;
    using DataArray = std::vector<char>;
    using DataArrayIt = std::vector<char>::iterator;
    using ConstDataArrayIt = std::vector<char>::const_iterator;

    //! \brief  The class default constructor
    ObjFileMtlImporter(std::vector<char> &buffer, const std::string &strAbsPath,
            ObjFile::Model *pModel);

    //! \brief  The class destructor
    ~ObjFileMtlImporter();

    ObjFileMtlImporter(const ObjFileMtlImporter &rOther) = delete;
    ObjFileMtlImporter &operator=(const ObjFileMtlImporter &rOther) = delete;

private:
    /// Copy constructor, empty.
    /// Load the whole material description
    void load();
    /// Get color data.
    void getColorRGBA(aiColor3D *pColor);
    void getColorRGBA(Maybe<aiColor3D> &value);
    /// Get illumination model from loaded data
    void getIlluminationModel(int &illum_model);
    /// Gets a float value from data.
    void getFloatValue(ai_real &value);
    void getFloatValue(Maybe<ai_real> &value);
    /// Creates a new material from loaded data.
    void createMaterial();
    /// Get texture name from loaded data.
    void getTexture();
    void getTextureOption(bool &clamp, int &clampIndex, aiString *&out);

private:
    //! Absolute pathname
    std::string m_strAbsPath;
    //! Data iterator showing to the current position in data buffer
    DataArrayIt m_DataIt;
    //! Data iterator to end of buffer
    DataArrayIt m_DataItEnd;
    //! USed model instance
    ObjFile::Model *m_pModel;
    //! Current line in file
    unsigned int m_uiLine;
    //! Helper buffer
    std::vector<char> m_buffer;
};

// ------------------------------------------------------------------------------------------------

} // Namespace Assimp

#endif // OBJFILEMTLIMPORTER_H_INC
