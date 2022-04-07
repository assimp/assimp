/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

TEST(utCOBImporter, importDwarfASCII) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/dwarf_ascii.cob", aiProcess_ValidateDataStructure);
    // FIXME: this is wrong, it should succeed
    // change to ASSERT_NE after it's been fixed
    ASSERT_EQ(nullptr, scene);
}

TEST(utCOBImporter, importDwarf) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/dwarf.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importMoleculeASCII) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/molecule_ascii.cob", aiProcess_ValidateDataStructure);
    // FIXME: this is wrong, it should succeed
    // change to ASSERT_NE after it's been fixed
    ASSERT_EQ(nullptr, scene);
}

TEST(utCOBImporter, importMolecule) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/molecule.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importSpider43ASCII) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/spider_4_3_ascii.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importSpider43) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/spider_4_3.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importSpider66ASCII) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/spider_6_6_ascii.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importSpider66) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/spider_6_6.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}
