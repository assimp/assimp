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

#include <assimp/Exporter.hpp>
#include <assimp/ProgressHandler.hpp>

using namespace Assimp;

#ifndef ASSIMP_BUILD_NO_EXPORT

class TestProgressHandler : public ProgressHandler {
public:
    TestProgressHandler() :
            ProgressHandler(),
            mPercentage(0.f) {
        // empty
    }

    virtual ~TestProgressHandler() {
        // empty
    }

    bool Update(float percentage = -1.f) override {
        mPercentage = percentage;
        return true;
    }
    float mPercentage;
};

class ExporterTest : public ::testing::Test {
    // empty
};

TEST_F(ExporterTest, ProgressHandlerTest) {
    Exporter exporter;
    TestProgressHandler *ph(new TestProgressHandler);
    exporter.SetProgressHandler(ph);
}

// Make sure all the registered exporters have useful descriptions
TEST_F(ExporterTest, ExporterIdTest) {
    Exporter exporter;
    size_t exportFormatCount = exporter.GetExportFormatCount();
    EXPECT_NE(0u, exportFormatCount) << "No registered exporters";
    typedef std::map<std::string, const aiExportFormatDesc *> ExportIdMap;
    ExportIdMap exporterMap;
    for (size_t i = 0; i < exportFormatCount; ++i) {
        // Check that the exporter description exists and makes sense
        const aiExportFormatDesc *desc = exporter.GetExportFormatDescription(i);
        ASSERT_NE(nullptr, desc) << "Missing aiExportFormatDesc at index " << i;
        EXPECT_NE(nullptr, desc->id) << "Null exporter ID at index " << i;
        EXPECT_STRNE("", desc->id) << "Empty exporter ID at index " << i;
        EXPECT_NE(nullptr, desc->description) << "Null exporter description at index " << i;
        EXPECT_STRNE("", desc->description) << "Empty exporter description at index " << i;
        EXPECT_NE(nullptr, desc->fileExtension) << "Null exporter file extension at index " << i;
        EXPECT_STRNE("", desc->fileExtension) << "Empty exporter file extension at index " << i;

        // Check the ID is unique
        std::string key(desc->id);
        std::pair<ExportIdMap::iterator, bool> result = exporterMap.emplace(key, desc);
        EXPECT_TRUE(result.second) << "Duplicate exported id: '" << key << "' " << desc->description << " *." << desc->fileExtension << " at index " << i;
    }

    const aiExportFormatDesc *desc = exporter.GetExportFormatDescription(exportFormatCount);
    EXPECT_EQ(nullptr, desc) << "More exporters than claimed";
}

#endif