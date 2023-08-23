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

/** @file utMDLImporter_HL1_Nodes.cpp
 *  @brief Half-Life 1 MDL loader nodes tests.
 */

#include "AbstractImportExportBase.h"
#include "AssetLib/MDL/HalfLife/HL1ImportDefinitions.h"
#include "MDLHL1TestFiles.h"
#include "UnitTestPCH.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

class utMDLImporter_HL1_Nodes : public ::testing::Test {

    /**
    * @note Represents a flattened node hierarchy where each item is a pair
    * containing the node level and it's name.
    */
    using Hierarchy = std::vector<std::pair<unsigned int, std::string>>;

    /**
    * @note A vector of strings. Used for symplifying syntax.
    */
    using StringVector = std::vector<std::string>;

public:
    /**
    * @note The following tests require a basic understanding
    * of the SMD format. For more information about SMD format,
    * please refer to the SMD importer or go to VDC
    * (Valve Developer Community).
    */

    // Given a model, verify that the bones nodes hierarchy is correctly formed.
    void checkBoneHierarchy() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "multiple_roots.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);
        ASSERT_NE(nullptr, scene->mRootNode);

        // First, check that "<MDL_root>" and "<MDL_bones>" are linked.
        const aiNode* node_MDL_root = scene->mRootNode->FindNode(AI_MDL_HL1_NODE_ROOT);
        ASSERT_NE(nullptr, node_MDL_root);

        const aiNode *node_MDL_bones = scene->mRootNode->FindNode(AI_MDL_HL1_NODE_BONES);
        ASSERT_NE(nullptr, node_MDL_bones);
        ASSERT_NE(nullptr, node_MDL_bones->mParent);
        ASSERT_EQ(node_MDL_root, node_MDL_bones->mParent);

        // Second, verify "<MDL_bones>" hierarchy.
        const Hierarchy expected_hierarchy = {
            { 0, AI_MDL_HL1_NODE_BONES },
                { 1, "root1_bone1" },
                    { 2, "root1_bone2" },
                        { 3, "root1_bone4" },
                        { 3, "root1_bone5" },
                    { 2, "root1_bone3" },
                        { 3, "root1_bone6" },
                { 1, "root2_bone1" },
                    { 2, "root2_bone2" },
                    { 2, "root2_bone3" },
                        { 3, "root2_bone5" },
                    { 2, "root2_bone4" },
                        { 3, "root2_bone6" },
                { 1, "root3_bone1" },
                    { 2, "root3_bone2" },
                    { 2, "root3_bone3" },
                    { 2, "root3_bone4" },
                        { 3, "root3_bone5" },
                            { 4, "root3_bone6" },
                            { 4, "root3_bone7" },
        };

        Hierarchy actual_hierarchy;
        flatten_hierarchy(node_MDL_bones, actual_hierarchy);
        ASSERT_EQ(expected_hierarchy, actual_hierarchy);
    }

    /*  Given a model with bones that have empty names,
        verify that all the bones of the imported model
        have unique and no empty names.

        ""        <----+---- empty names
        ""        <----+
        ""        <----+
        "Bone_3"       |
        ""        <----+
        "Bone_2"       |
        "Bone_5"       |
        ""        <----+
        ""        <----+
    */
    void emptyBonesNames() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "unnamed_bones.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);

        const StringVector expected_bones_names = {
            "Bone",
            "Bone_0",
            "Bone_1",
            "Bone_3",
            "Bone_4",
            "Bone_2",
            "Bone_5",
            "Bone_6",
            "Bone_7"
        };

        StringVector actual_bones_names;
        get_node_children_names(scene->mRootNode->FindNode(AI_MDL_HL1_NODE_BONES), actual_bones_names);
        ASSERT_EQ(expected_bones_names, actual_bones_names);
    }

    /*  Given a model with bodyparts that have empty names,
        verify that the imported model contains bodyparts with
        unique and no empty names.

        $body ""           <----+---- empty names
        $body "Bodypart_1"      |
        $body "Bodypart_5"      |
        $body "Bodypart_6"      |
        $body ""           <----+
        $body "Bodypart_2"      |
        $body ""           <----+
        $body "Bodypart_3"      |
        $body ""           <----+
    */
    void emptyBodypartsNames() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "unnamed_bodyparts.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);

        const StringVector expected_bodyparts_names = {
            "Bodypart",
            "Bodypart_1",
            "Bodypart_5",
            "Bodypart_6",
            "Bodypart_0",
            "Bodypart_2",
            "Bodypart_4",
            "Bodypart_3",
            "Bodypart_7"
        };

        StringVector actual_bodyparts_names;
        // Get the bodyparts names "without" the submodels.
        get_node_children_names(scene->mRootNode->FindNode(AI_MDL_HL1_NODE_BODYPARTS), actual_bodyparts_names, 0);
        ASSERT_EQ(expected_bodyparts_names, actual_bodyparts_names);
    }

    /*  Given a model with bodyparts that have duplicate names,
        verify that the imported model contains bodyparts with
        unique and no duplicate names.

        $body "Bodypart"   <-----+
        $body "Bodypart_1" <--+  |
        $body "Bodypart_2"    |  |
        $body "Bodypart1"     |  |
        $body "Bodypart"   ---|--+
        $body "Bodypart_1" ---+  |
        $body "Bodypart2"        |
        $body "Bodypart"   ------+
        $body "Bodypart_4"
    */
    void duplicateBodypartsNames() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "duplicate_bodyparts.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);

        const StringVector expected_bodyparts_names = {
            "Bodypart",
            "Bodypart_1",
            "Bodypart_2",
            "Bodypart1",
            "Bodypart_0",
            "Bodypart_1_0",
            "Bodypart2",
            "Bodypart_3",
            "Bodypart_4"
        };

        StringVector actual_bodyparts_names;
        // Get the bodyparts names "without" the submodels.
        get_node_children_names(scene->mRootNode->FindNode(AI_MDL_HL1_NODE_BODYPARTS), actual_bodyparts_names, 0);
        ASSERT_EQ(expected_bodyparts_names, actual_bodyparts_names);
    }

    /*  Given a model with several bodyparts that contains multiple
        sub models with the same file name, verify for each bodypart
        sub model of the imported model that they have a unique name.

        $bodygroup "first_bodypart"
        {
            studio "triangle"   <------+ duplicate file names.
            studio "triangle"   -------+
        }                              |
                                       |
        $bodygroup "second_bodypart"   |
        {                              |
            studio "triangle"   -------+ same as first bodypart, but with same file.
            studio "triangle"   -------+
        }

        $bodygroup "last_bodypart"
        {
            studio "triangle2"  <------+ duplicate names.
            studio "triangle2"  -------+
        }
    */
    void duplicateSubModelsNames() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "duplicate_submodels.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);

        const std::vector<StringVector> expected_bodypart_sub_models_names = {
            {
                    "triangle",
                    "triangle_0",
            },
            {
                    "triangle_1",
                    "triangle_2",
            },
            {
                    "triangle2",
                    "triangle2_0",
            }
        };

        const aiNode *bodyparts_node = scene->mRootNode->FindNode(AI_MDL_HL1_NODE_BODYPARTS);
        ASSERT_NE(nullptr, bodyparts_node);
        EXPECT_EQ(3u, bodyparts_node->mNumChildren);

        StringVector actual_submodels_names;
        for (unsigned int i = 0; i < bodyparts_node->mNumChildren; ++i)
        {
            actual_submodels_names.clear();
            get_node_children_names(bodyparts_node->mChildren[i], actual_submodels_names);
            ASSERT_EQ(expected_bodypart_sub_models_names[i], actual_submodels_names);
        }
    }

    /*  Given a model with sequences that have duplicate names, verify
        that each sequence from the imported model has a unique
        name.

        $sequence "idle_1" <-------+
        $sequence "idle"   <----+  |
        $sequence "idle_2"      |  |
        $sequence "idle"   -----+  |
        $sequence "idle_0"      |  |
        $sequence "idle_1" -----|--+
        $sequence "idle_3"      |
        $sequence "idle"   -----+
        $sequence "idle_7"
    */
    void duplicateSequenceNames() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "duplicate_sequences.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);

        const StringVector expected_sequence_names = {
            "idle_1",
            "idle",
            "idle_2",
            "idle_4",
            "idle_0",
            "idle_1_0",
            "idle_3",
            "idle_5",
            "idle_7"
        };

        StringVector actual_sequence_names;
        get_node_children_names(scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_INFOS), actual_sequence_names);
        ASSERT_EQ(expected_sequence_names, actual_sequence_names);
    }

    /*  Given a model with sequences that have empty names, verify
        that each sequence from the imported model has a unique
        name.

        $sequence ""            <----+---- empty names
        $sequence "Sequence_1"       |
        $sequence ""            <----+
        $sequence "Sequence_4"       |
        $sequence ""            <----+
        $sequence "Sequence_8"       |
        $sequence ""            <----+
        $sequence "Sequence_2"       |
        $sequence ""            <----+
    */
    void emptySequenceNames() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "unnamed_sequences.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);

        const StringVector expected_sequence_names = {
            "Sequence",
            "Sequence_1",
            "Sequence_0",
            "Sequence_4",
            "Sequence_3",
            "Sequence_8",
            "Sequence_5",
            "Sequence_2",
            "Sequence_6"
        };

        StringVector actual_sequence_names;
        get_node_children_names(scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_INFOS), actual_sequence_names);
        ASSERT_EQ(expected_sequence_names, actual_sequence_names);
    }

    /*  Given a model with sequence groups that have duplicate names,
        verify that each sequence group from the imported model has
        a unique name.

        "default"
        $sequencegroup "SequenceGroup"    <----+
        $sequencegroup "SequenceGroup_1"       |
        $sequencegroup "SequenceGroup_5"  <----|--+
        $sequencegroup "SequenceGroup"    -----+  |
        $sequencegroup "SequenceGroup_0"       |  |
        $sequencegroup "SequenceGroup"    -----+  |
        $sequencegroup "SequenceGroup_5"  --------+
        $sequencegroup "SequenceGroup_6"
        $sequencegroup "SequenceGroup_2"
    */
    void duplicateSequenceGroupNames() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "duplicate_sequence_groups/duplicate_sequence_groups.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);

        const StringVector expected_sequence_names = {
            "default",
            "SequenceGroup",
            "SequenceGroup_1",
            "SequenceGroup_5",
            "SequenceGroup_3",
            "SequenceGroup_0",
            "SequenceGroup_4",
            "SequenceGroup_5_0",
            "SequenceGroup_6",
            "SequenceGroup_2"
        };

        StringVector actual_sequence_names;
        get_node_children_names(scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_GROUPS), actual_sequence_names);
        ASSERT_EQ(expected_sequence_names, actual_sequence_names);
    }

    /*  Given a model with sequence groups that have empty names,
        verify that each sequence group from the imported model has
        a unique name.

        "default"
        $sequencegroup ""                 <----+---- empty names
        $sequencegroup "SequenceGroup_2"       |
        $sequencegroup "SequenceGroup_6"       |
        $sequencegroup ""                 <----+
        $sequencegroup ""                 <----+
        $sequencegroup "SequenceGroup_1"       |
        $sequencegroup "SequenceGroup_5"       |
        $sequencegroup ""                 <----+
        $sequencegroup "SequenceGroup_4"
    */
    void emptySequenceGroupNames() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "unnamed_sequence_groups/unnamed_sequence_groups.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);

        const StringVector expected_sequence_names = {
            "default",
            "SequenceGroup",
            "SequenceGroup_2",
            "SequenceGroup_6",
            "SequenceGroup_0",
            "SequenceGroup_3",
            "SequenceGroup_1",
            "SequenceGroup_5",
            "SequenceGroup_7",
            "SequenceGroup_4"
        };

        StringVector actual_sequence_names;
        get_node_children_names(scene->mRootNode->FindNode(AI_MDL_HL1_NODE_SEQUENCE_GROUPS), actual_sequence_names);
        ASSERT_EQ(expected_sequence_names, actual_sequence_names);
    }

    /*  Verify that mOffsetMatrix applies the correct
        inverse bind pose transform. */
    void offsetMatrixUnappliesTransformations() {

        const float TOLERANCE = 0.01f;

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(MDL_HL1_FILE_MAN, aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);

        aiNode *scene_bones_node = scene->mRootNode->FindNode(AI_MDL_HL1_NODE_BONES);

        const aiMatrix4x4 identity_matrix;

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh *scene_mesh = scene->mMeshes[i];
            for (unsigned int j = 0; j < scene_mesh->mNumBones; ++j) {
                aiBone *scene_mesh_bone = scene_mesh->mBones[j];

                // Store local node transforms.
                aiNode *n = scene_bones_node->FindNode(scene_mesh_bone->mName);
                std::vector<aiMatrix4x4> bone_matrices = { n->mTransformation };
                while (n->mParent != scene->mRootNode) {
                    n = n->mParent;
                    bone_matrices.push_back(n->mTransformation);
                }

                // Compute absolute node transform.
                aiMatrix4x4 transform;
                for (auto it = bone_matrices.rbegin(); it != bone_matrices.rend(); ++it)
                    transform *= *it;

                // Unapply the transformation using the offset matrix.
                aiMatrix4x4 unapplied_transform = scene_mesh_bone->mOffsetMatrix * transform;

                // Ensure that we have, approximately, the identity matrix.
                expect_equal_matrices(identity_matrix, unapplied_transform, TOLERANCE);
            }
        }
    }

private:
    void expect_equal_matrices(const aiMatrix4x4 &expected, const aiMatrix4x4 &actual, float abs_error) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j)
                EXPECT_NEAR(expected[i][j], actual[i][j], abs_error);
        }
    }

    /** Get a flattened representation of a node's hierarchy.
     * \param[in] node The node.
     * \param[out] hierarchy The flattened node's hierarchy.
     */
    void flatten_hierarchy(const aiNode *node, Hierarchy &hierarchy)
    {
        flatten_hierarchy_impl(node, hierarchy, 0);
    }

    void flatten_hierarchy_impl(const aiNode *node, Hierarchy &hierarchy, unsigned int level)
    {
        hierarchy.push_back({ level, node->mName.C_Str() });
        for (size_t i = 0; i < node->mNumChildren; ++i)
        {
            flatten_hierarchy_impl(node->mChildren[i], hierarchy, level + 1);
        }
    }

    /** Get all node's children names beneath max_level.
     * \param[in] node The parent node from which to get all children names.
     * \param[out] names The list of children names.
     * \param[in] max_level If set to -1, all children names will be collected.
     */
    void get_node_children_names(const aiNode *node, StringVector &names, const int max_level = -1)
    {
        get_node_children_names_impl(node, names, 0, max_level);
    }

    void get_node_children_names_impl(const aiNode *node, StringVector &names, int level, const int max_level = -1)
    {
        for (size_t i = 0; i < node->mNumChildren; ++i)
        {
            names.push_back(node->mChildren[i]->mName.C_Str());
            if (max_level == -1 || level < max_level)
            {
                get_node_children_names_impl(node->mChildren[i], names, level + 1, max_level);
            }
        }
    }
};

TEST_F(utMDLImporter_HL1_Nodes, checkBoneHierarchy) {
    checkBoneHierarchy();
}

TEST_F(utMDLImporter_HL1_Nodes, emptyBonesNames) {
    emptyBonesNames();
}

TEST_F(utMDLImporter_HL1_Nodes, emptyBodypartsNames) {
    emptyBodypartsNames();
}

TEST_F(utMDLImporter_HL1_Nodes, duplicateBodypartsNames) {
    duplicateBodypartsNames();
}

TEST_F(utMDLImporter_HL1_Nodes, duplicateSubModelsNames) {
    duplicateSubModelsNames();
}

TEST_F(utMDLImporter_HL1_Nodes, emptySequenceNames) {
    emptySequenceNames();
}

TEST_F(utMDLImporter_HL1_Nodes, duplicateSequenceNames) {
    duplicateSequenceNames();
}

TEST_F(utMDLImporter_HL1_Nodes, emptySequenceGroupNames) {
    emptySequenceGroupNames();
}

TEST_F(utMDLImporter_HL1_Nodes, duplicateSequenceGroupNames) {
    duplicateSequenceGroupNames();
}

TEST_F(utMDLImporter_HL1_Nodes, offsetMatrixUnappliesTransformations) {
    offsetMatrixUnappliesTransformations();
}
