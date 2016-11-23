/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team

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
#include "BlenderIntermediate.h"
#include "./../include/assimp/camera.h"
#include "./../include/assimp/light.h"
#include "./../include/assimp/mesh.h"
#include "./../include/assimp/texture.h"

using namespace ::Assimp;
using namespace ::Assimp::Blender;

class BlenderIntermediateTest : public ::testing::Test {
    // empty
};

#define NAME_1 "name1"
#define NAME_2 "name2"

TEST_F( BlenderIntermediateTest,ConversionData_ObjectCompareTest ) {
    Object obj1, obj2;
    strncpy( obj1.id.name, NAME_1, sizeof(NAME_1) );
    strncpy( obj2.id.name, NAME_2, sizeof(NAME_2) );
    Blender::ObjectCompare cmp_false;
    bool res( cmp_false( &obj1, &obj2 ) );
    EXPECT_FALSE( res );

    Blender::ObjectCompare cmp_true;
    res = cmp_true( &obj1, &obj1 );
    EXPECT_TRUE( res );
}





