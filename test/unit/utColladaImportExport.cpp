/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team



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
#include "AbstractImportExportBase.h"
#include "UnitTestPCH.h"

#include <assimp/commonMetaData.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

class utColladaImportExport : public AbstractImportExportBase {
public:
	virtual bool importerTest() {
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/duck.dae", aiProcess_ValidateDataStructure);
		if (scene == nullptr)
			return false;

		// Expected number of items
		EXPECT_EQ(scene->mNumMeshes, 1u);
		EXPECT_EQ(scene->mNumMaterials, 1u);
		EXPECT_EQ(scene->mNumAnimations, 0u);
		EXPECT_EQ(scene->mNumTextures, 0u);
		EXPECT_EQ(scene->mNumLights, 1u);
		EXPECT_EQ(scene->mNumCameras, 1u);

		// Expected common metadata
		aiString value;
		EXPECT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT, value)) << "No importer format metadata";
		EXPECT_STREQ("Collada Importer", value.C_Str());

		EXPECT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT_VERSION, value)) << "No format version metadata";
		EXPECT_STREQ("1.4.1", value.C_Str());

		EXPECT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_GENERATOR, value)) << "No generator metadata";
		EXPECT_EQ(strncmp(value.C_Str(), "Maya 8.0", 8), 0) << "AI_METADATA_SOURCE_GENERATOR was: " << value.C_Str();

		EXPECT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_COPYRIGHT, value)) << "No copyright metadata";
		EXPECT_EQ(strncmp(value.C_Str(), "Copyright 2006", 14), 0) << "AI_METADATA_SOURCE_COPYRIGHT was: " << value.C_Str();

		return true;
	}
};

TEST_F(utColladaImportExport, importBlenFromFileTest) {
	EXPECT_TRUE(importerTest());
}

class utColladaZaeImportExport : public AbstractImportExportBase {
public:
	virtual bool importerTest() {
		{
			Assimp::Importer importer;
			const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/duck.zae", aiProcess_ValidateDataStructure);
			if (scene == nullptr)
				return false;

			// Expected number of items
			EXPECT_EQ(scene->mNumMeshes, 1u);
			EXPECT_EQ(scene->mNumMaterials, 1u);
			EXPECT_EQ(scene->mNumAnimations, 0u);
			EXPECT_EQ(scene->mNumTextures, 1u);
			EXPECT_EQ(scene->mNumLights, 1u);
			EXPECT_EQ(scene->mNumCameras, 1u);
		}

		{
			Assimp::Importer importer;
			const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/duck_nomanifest.zae", aiProcess_ValidateDataStructure);
			if (scene == nullptr)
				return false;

			// Expected number of items
			EXPECT_EQ(scene->mNumMeshes, 1u);
			EXPECT_EQ(scene->mNumMaterials, 1u);
			EXPECT_EQ(scene->mNumAnimations, 0u);
			EXPECT_EQ(scene->mNumTextures, 1u);
			EXPECT_EQ(scene->mNumLights, 1u);
			EXPECT_EQ(scene->mNumCameras, 1u);
		}

		return true;
	}
};

TEST_F(utColladaZaeImportExport, importBlenFromFileTest) {
	EXPECT_TRUE(importerTest());
}
