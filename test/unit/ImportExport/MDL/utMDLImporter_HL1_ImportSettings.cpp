/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

/** @file utMDLImporter_HL1_ImportSettings.cpp
 *  @brief Half-Life 1 MDL loader import settings tests.
 */

#include "AbstractImportExportBase.h"
#include "AssetLib/MDL/HalfLife/HL1ImportDefinitions.h"
#include "MDLHL1TestFiles.h"
#include "UnitTestPCH.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <functional>
#include <initializer_list>

using namespace Assimp;

class utMDLImporter_HL1_ImportSettings : public ::testing::Test {

public:
    // Test various import settings scenarios.

    void importSettings() {

        /* Verify that animations are *NOT* imported when
           'Read animations' is disabled. */
        load_with_import_setting_bool(
                MDL_HL1_FILE_MAN,
                AI_CONFIG_IMPORT_MDL_HL1_READ_ANIMATIONS,
                false, // Set config value to false.
                [&](const aiScene *scene) {
                    EXPECT_EQ(0u, scene->mNumAnimations);
                    EXPECT_EQ(nullptr, scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_INFOS));
                    EXPECT_EQ(nullptr, scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_GROUPS));
                    EXPECT_EQ(nullptr, scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_TRANSITION_GRAPH));

                    expect_global_info_eq<int>(scene, { { 0, "NumSequences" },
                                                              { 0, "NumTransitionNodes" } });
                });

        /* Verify that blend controllers info is *NOT* imported when
           'Read blend controllers' is disabled. */
        load_with_import_setting_bool(
                MDL_HL1_FILE_MAN,
                AI_CONFIG_IMPORT_MDL_HL1_READ_BLEND_CONTROLLERS,
                false, // Set config value to false.
                [&](const aiScene *scene) {
                    EXPECT_NE(0u, scene->mNumAnimations);

                    const aiNode *sequence_infos = scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_INFOS);
                    EXPECT_NE(nullptr, sequence_infos);

                    for (unsigned int i = 0; i < sequence_infos->mNumChildren; ++i)
                        EXPECT_EQ(nullptr, sequence_infos->mChildren[i]->FindNode(AI_MDL_HL1_NODE_BLEND_CONTROLLERS));

                    expect_global_info_eq(scene, 0, "NumBlendControllers");
                });

        /* Verify that animation events are *NOT* imported when
           'Read animation events' is disabled. */
        load_with_import_setting_bool(
                MDL_HL1_FILE_MAN,
                AI_CONFIG_IMPORT_MDL_HL1_READ_ANIMATION_EVENTS,
                false, // Set config value to false.
                [&](const aiScene *scene) {
                    EXPECT_NE(0u, scene->mNumAnimations);

                    const aiNode *sequence_infos = scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_INFOS);
                    EXPECT_NE(nullptr, sequence_infos);

                    for (unsigned int i = 0; i < sequence_infos->mNumChildren; ++i)
                        EXPECT_EQ(nullptr, sequence_infos->mChildren[i]->FindNode(AI_MDL_HL1_NODE_ANIMATION_EVENTS));
                });

        /* Verify that sequence transitions info is read when
           'Read sequence transitions' is enabled. */
        load_with_import_setting_bool(
                ASSIMP_TEST_MDL_HL1_MODELS_DIR "sequence_transitions.mdl",
                AI_CONFIG_IMPORT_MDL_HL1_READ_SEQUENCE_TRANSITIONS,
                true, // Set config value to true.
                [&](const aiScene *scene) {
                    EXPECT_NE(nullptr, scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_TRANSITION_GRAPH));
                    expect_global_info_eq(scene, 4, "NumTransitionNodes");
                });

        /* Verify that sequence transitions info is *NOT* read when
           'Read sequence transitions' is disabled. */
        load_with_import_setting_bool(
                ASSIMP_TEST_MDL_HL1_MODELS_DIR "sequence_transitions.mdl",
                AI_CONFIG_IMPORT_MDL_HL1_READ_SEQUENCE_TRANSITIONS,
                false, // Set config value to false.
                [&](const aiScene *scene) {
                    EXPECT_EQ(nullptr, scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_TRANSITION_GRAPH));
                    expect_global_info_eq(scene, 0, "NumTransitionNodes");
                });

        /* Verify that bone controllers info is *NOT* read when
           'Read bone controllers' is disabled. */
        load_with_import_setting_bool(
                MDL_HL1_FILE_MAN,
                AI_CONFIG_IMPORT_MDL_HL1_READ_BONE_CONTROLLERS,
                false, // Set config value to false.
                [&](const aiScene *scene) {
                    EXPECT_EQ(nullptr, scene->mRootNode->FindNode(AI_MDL_HL1_NODE_BONE_CONTROLLERS));
                    expect_global_info_eq(scene, 0, "NumBoneControllers");
                });

        /* Verify that attachments info is *NOT* read when
           'Read attachments' is disabled. */
        load_with_import_setting_bool(
                MDL_HL1_FILE_MAN,
                AI_CONFIG_IMPORT_MDL_HL1_READ_ATTACHMENTS,
                false, // Set config value to false.
                [&](const aiScene *scene) {
                    EXPECT_EQ(nullptr, scene->mRootNode->FindNode(AI_MDL_HL1_NODE_ATTACHMENTS));
                    expect_global_info_eq(scene, 0, "NumAttachments");
                });

        /* Verify that hitboxes info is *NOT* read when
           'Read hitboxes' is disabled. */
        load_with_import_setting_bool(
                MDL_HL1_FILE_MAN,
                AI_CONFIG_IMPORT_MDL_HL1_READ_HITBOXES,
                false, // Set config value to false.
                [&](const aiScene *scene) {
                    EXPECT_EQ(nullptr, scene->mRootNode->FindNode(AI_MDL_HL1_NODE_HITBOXES));
                    expect_global_info_eq(scene, 0, "NumHitboxes");
                });

        /* Verify that misc global info is *NOT* read when
           'Read misc global info' is disabled. */
        load_with_import_setting_bool(
                MDL_HL1_FILE_MAN,
                AI_CONFIG_IMPORT_MDL_HL1_READ_MISC_GLOBAL_INFO,
                false, // Set config value to false.
                [&](const aiScene *scene) {
                    aiNode *global_info = get_global_info(scene);
                    EXPECT_NE(nullptr, global_info);
                    aiVector3D temp;
                    EXPECT_FALSE(global_info->mMetaData->Get("EyePosition", temp));
                });
    }

private:
    void load_with_import_setting_bool(
            const char *file_path,
            const char *setting_key,
            bool setting_value,
            std::function<void(const aiScene *)> &&func) {
        Assimp::Importer importer;
        importer.SetPropertyBool(setting_key, setting_value);
        const aiScene *scene = importer.ReadFile(file_path, aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);
        func(scene);
    }

    inline static aiNode *get_global_info(const aiScene *scene) {
        return scene->mRootNode->FindNode(AI_MDL_HL1_NODE_GLOBAL_INFO);
    }

    template <typename T>
    static void expect_global_info_eq(
            const aiScene *scene,
            T expected_value,
            const char *key_name) {
        aiNode *global_info = get_global_info(scene);
        EXPECT_NE(nullptr, global_info);
        T temp = 0;
        EXPECT_TRUE(global_info->mMetaData->Get(key_name, temp));
        EXPECT_EQ(expected_value, temp);
    }

    template <typename T>
    static void expect_global_info_eq(const aiScene *scene,
            std::initializer_list<std::pair<T, const char *>> p_kv) {
        aiNode *global_info = get_global_info(scene);
        EXPECT_NE(nullptr, global_info);
        for (auto it = p_kv.begin(); it != p_kv.end(); ++it) {
            T temp = 0;
            EXPECT_TRUE(global_info->mMetaData->Get(it->second, temp));
            EXPECT_EQ(it->first, temp);
        }
    }
};

TEST_F(utMDLImporter_HL1_ImportSettings, importSettings) {
    importSettings();
}
