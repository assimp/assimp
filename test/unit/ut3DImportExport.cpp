/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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
---------------------------------------------------------------------------
*/

#include "UnitTestPCH.h"

#include <assimp/DefaultIOSystem.h>
#include <assimp/MemoryIOWrapper.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <cstring>
#include <string>

using namespace Assimp;

namespace {

// Serves the .uc file from memory, the _d.3d and _a.3d files come from disk.
class UcFromMemoryIOSystem : public IOSystem {
public:
    explicit UcFromMemoryIOSystem(const char *uc) :
            mUc(uc) {}

    bool Exists(const char *file) const override {
        return mFiles.Exists(file);
    }

    char getOsSeparator() const override {
        return mFiles.getOsSeparator();
    }

    IOStream *Open(const char *file, const char *mode = "rb") override {
        const size_t len = strlen(file);
        if (len > 3 && !strcmp(file + len - 3, ".uc")) {
            return new MemoryIOStream(reinterpret_cast<const uint8_t *>(mUc.data()), mUc.size());
        }
        return mFiles.Open(file, mode);
    }

    void Close(IOStream *file) override {
        delete file;
    }

private:
    DefaultIOSystem mFiles;
    std::string mUc;
};

} // namespace

TEST(ut3DImportExport, importBoxA) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3D/box_a.3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST(ut3DImportExport, importBoxD) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3D/box_d.3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST(ut3DImportExport, importBoxUC) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3D/box.uc", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(ut3DImportExport, importUCDirectiveAtEndOfFile) {
    // Each .uc ends on a directive line.
    const char *const ucFiles[] = {
        "#exec TEXTURE IMPORT NAME=Jtex1 FILE=texture1.pcx",
        "#exec MESHMAP SETTEXTURE MESHMAP=box NUM=1",
        "#exec MESHMAP SCALE MESHMAP=box X=0.1 Y=0.1 Z=0.2",
        "#exec MESHMAP SCALE",
        "#exec MESHMAP SCALE MESHMAP=box X=0.1 Y=0.1 Z=0.2\n",
    };
    for (const char *uc : ucFiles) {
        Assimp::Importer importer;
        importer.SetIOHandler(new UcFromMemoryIOSystem(uc));
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3D/box.uc", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene) << uc;
    }
}

TEST(ut3DImportExport, importMarRifle) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/3D/mar_rifle.uc", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST(ut3DImportExport, importMarRifleA) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/3D/mar_rifle_a.3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST(ut3DImportExport, importMarRifleD) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/3D/mar_rifle_d.3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}
