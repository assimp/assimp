/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team



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
#include "AbstractImportExportBase.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using namespace Assimp;

class utIFCImportExport : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/IFC/AC14-FZK-Haus.ifc", aiProcess_ValidateDataStructure );
        return nullptr != scene;

        return true;
    }
};

TEST_F( utIFCImportExport, importIFCFromFileTest ) {
    EXPECT_TRUE( importerTest() );
}

TEST_F( utIFCImportExport, importComplextypeAsColor ) {
    std::string asset =
        "ISO-10303-21;\n"
        "HEADER;\n"
        "FILE_DESCRIPTION( ( 'ViewDefinition [CoordinationView, SpaceBoundary2ndLevelAddOnView]', 'Option [Filter: ]' ), '2;1' );\n"
        "FILE_NAME( 'S:\\[IFC]\\[COMPLETE-BUILDINGS]\\FZK-MODELS\\FZK-Haus\\ArchiCAD-14\\AC14-FZK-Haus.ifc', '2010-10-07T13:40:52', ( 'Architect' ), ( 'Building Designer Office' ), 'PreProc - EDM 5.0', 'ArchiCAD 14.00 Release 1. Windows Build Number of the Ifc 2x3 interface: 3427', 'The authorising person' );\n"
        "FILE_SCHEMA( ( 'IFC2X3' ) );\n"
        "ENDSEC;\n"
        "\n"
        "DATA;\n"
        "#1 = IFCORGANIZATION( 'GS', 'Graphisoft', 'Graphisoft', $, $ );\n"
        "#2 = IFCPROPERTYSINGLEVALUE( 'Red', $, IFCINTEGER( 255 ), $ );\n"
        "#3 = IFCPROPERTYSINGLEVALUE( 'Green', $, IFCINTEGER( 255 ), $ );\n"
        "#4 = IFCPROPERTYSINGLEVALUE( 'Blue', $, IFCINTEGER( 255 ), $ );\n"
        "#5 = IFCCOMPLEXPROPERTY( 'Color', $, 'Color', ( #19, #20, #21 ) );\n";
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory( asset.c_str(), asset.size(), 0 );
    EXPECT_EQ( nullptr, scene );

}
