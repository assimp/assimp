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

#include <gtest/gtest.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <cstring>

// Test OOM vulnerability fix (OSS-Fuzz 476180586)
TEST(OFFSecurityTest, NoOOMOnMaliciousCount) {
    Assimp::Importer importer;
    // Input that previously caused malloc(2.2GB) crash
    const char* maliciousOFF = "OFF\n2000000000 1 0\n";
    const aiScene* scene = importer.ReadFileFromMemory(
        maliciousOFF, strlen(maliciousOFF),
        aiProcess_ValidateDataStructure
    );
    // Should fail gracefully, not crash/OOM
    EXPECT_EQ(scene, nullptr);
    std::string error = importer.GetErrorString();
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("exceeds limit") != std::string::npos ||
                error.find("overflow") != std::string::npos ||
                error.find("no valid faces") != std::string::npos);
}

// Test valid OFF file still works (regression)
TEST(OFFSecurityTest, ValidOFFLoadsSuccessfully) {
    Assimp::Importer importer;
    const char* validOFF =
        "OFF\n"
        "4 2 0\n"
        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n"
        "1.0 1.0 0.0\n"
        "0.0 1.0 0.0\n"
        "3 0 1 2\n"
        "3 0 2 3\n";
    const aiScene* scene = importer.ReadFileFromMemory(
        validOFF, strlen(validOFF),
        aiProcess_ValidateDataStructure
    );
    EXPECT_NE(scene, nullptr);
}
