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

/** @file  Info.cpp
 *  @brief Implementation of the 'assimp info' utility  */

#include "Main.h"

#include <cstdio>
#include <iostream>
#include <string>

const char *AICMD_MSG_INFO_HELP_E =
        "assimp info <file> [-r] [-v]\n"
        "\tPrint basic structure of a 3D model\n"
        "\t-r,--raw: No postprocessing, do a raw import\n"
        "\t-v,--verbose: Print verbose info such as node transform data\n"
        "\t-s, --silent: Print only minimal info\n";

const char *TREE_BRANCH_ASCII = "|-";
const char *TREE_BRANCH_UTF8 = "\xe2\x94\x9c\xe2\x95\xb4";
const char *TREE_STOP_ASCII = "'-";
const char *TREE_STOP_UTF8 = "\xe2\x94\x94\xe2\x95\xb4";
const char *TREE_CONTINUE_ASCII = "| ";
const char *TREE_CONTINUE_UTF8 = "\xe2\x94\x82 ";

// note: by default this is using utf-8 text.
// this is well supported on pretty much any linux terminal.
// if this causes problems on some platform,
// put an #ifdef to use the ascii version for that platform.
const char *TREE_BRANCH = TREE_BRANCH_UTF8;
const char *TREE_STOP = TREE_STOP_UTF8;
const char *TREE_CONTINUE = TREE_CONTINUE_UTF8;

// -----------------------------------------------------------------------------------
unsigned int CountNodes(const aiNode *root) {
    unsigned int i = 0;
    for (unsigned int a = 0; a < root->mNumChildren; ++a) {
        i += CountNodes(root->mChildren[a]);
    }
    return 1 + i;
}

// -----------------------------------------------------------------------------------
unsigned int GetMaxDepth(const aiNode *root) {
    unsigned int cnt = 0;
    for (unsigned int i = 0; i < root->mNumChildren; ++i) {
        cnt = std::max(cnt, GetMaxDepth(root->mChildren[i]));
    }
    return cnt + 1;
}

// -----------------------------------------------------------------------------------
unsigned int CountVertices(const aiScene *scene) {
    unsigned int cnt = 0;
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        cnt += scene->mMeshes[i]->mNumVertices;
    }
    return cnt;
}

// -----------------------------------------------------------------------------------
unsigned int CountFaces(const aiScene *scene) {
    unsigned int cnt = 0;
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        cnt += scene->mMeshes[i]->mNumFaces;
    }
    return cnt;
}

// -----------------------------------------------------------------------------------
unsigned int CountBones(const aiScene *scene) {
    unsigned int cnt = 0;
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        cnt += scene->mMeshes[i]->mNumBones;
    }
    return cnt;
}

// -----------------------------------------------------------------------------------
unsigned int CountAnimChannels(const aiScene *scene) {
    unsigned int cnt = 0;
    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        cnt += scene->mAnimations[i]->mNumChannels;
    }
    return cnt;
}

// -----------------------------------------------------------------------------------
unsigned int GetAvgFacePerMesh(const aiScene *scene) {
    return (scene->mNumMeshes != 0) ? static_cast<unsigned int>(CountFaces(scene) / scene->mNumMeshes) : 0;
}

// -----------------------------------------------------------------------------------
unsigned int GetAvgVertsPerMesh(const aiScene *scene) {
    return (scene->mNumMeshes != 0) ? static_cast<unsigned int>(CountVertices(scene) / scene->mNumMeshes) : 0;
}

// -----------------------------------------------------------------------------------
void FindSpecialPoints(const aiScene *scene, const aiNode *root, aiVector3D special_points[3], const aiMatrix4x4 &mat = aiMatrix4x4()) {
    // XXX that could be greatly simplified by using code from code/ProcessHelper.h
    // XXX I just don't want to include it here.
    const aiMatrix4x4 trafo = root->mTransformation * mat;
    for (unsigned int i = 0; i < root->mNumMeshes; ++i) {
        const aiMesh *mesh = scene->mMeshes[root->mMeshes[i]];

        for (unsigned int a = 0; a < mesh->mNumVertices; ++a) {
            aiVector3D v = trafo * mesh->mVertices[a];

            special_points[0].x = std::min(special_points[0].x, v.x);
            special_points[0].y = std::min(special_points[0].y, v.y);
            special_points[0].z = std::min(special_points[0].z, v.z);

            special_points[1].x = std::max(special_points[1].x, v.x);
            special_points[1].y = std::max(special_points[1].y, v.y);
            special_points[1].z = std::max(special_points[1].z, v.z);
        }
    }

    for (unsigned int i = 0; i < root->mNumChildren; ++i) {
        FindSpecialPoints(scene, root->mChildren[i], special_points, trafo);
    }
}

// -----------------------------------------------------------------------------------
void FindSpecialPoints(const aiScene *scene, aiVector3D special_points[3]) {
    special_points[0] = aiVector3D(1e10, 1e10, 1e10);
    special_points[1] = aiVector3D(-1e10, -1e10, -1e10);

    FindSpecialPoints(scene, scene->mRootNode, special_points);
    special_points[2] = (special_points[0] + special_points[1]) * (ai_real)0.5;
}

// -----------------------------------------------------------------------------------
std::string FindPTypes(const aiScene *scene) {
    bool haveit[4] = { 0 };
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const unsigned int pt = scene->mMeshes[i]->mPrimitiveTypes;
        if (pt & aiPrimitiveType_POINT) {
            haveit[0] = true;
        }
        if (pt & aiPrimitiveType_LINE) {
            haveit[1] = true;
        }
        if (pt & aiPrimitiveType_TRIANGLE) {
            haveit[2] = true;
        }
        if (pt & aiPrimitiveType_POLYGON) {
            haveit[3] = true;
        }
    }
    return (haveit[0] ? std::string("points") : "") + (haveit[1] ? "lines" : "") +
           (haveit[2] ? "triangles" : "") + (haveit[3] ? "n-polygons" : "");
}

// -----------------------------------------------------------------------------------
// Prettily print the node graph to stdout
void PrintHierarchy(
        const aiNode *node,
        const std::string &indent,
        bool verbose,
        bool last = false,
        bool first = true) {
    // tree visualization
    std::string branchchar;
    if (first) {
        branchchar = "";
    } else if (last) {
        branchchar = TREE_STOP;
    } // "'-"
    else {
        branchchar = TREE_BRANCH;
    } // "|-"

    // print the indent and the branch character and the name
    std::cout << indent << branchchar << node->mName.C_Str();

    // if there are meshes attached, indicate this
    if (node->mNumMeshes) {
        std::cout << " (mesh ";
        bool sep = false;
        for (size_t i = 0; i < node->mNumMeshes; ++i) {
            unsigned int mesh_index = node->mMeshes[i];
            if (sep) {
                std::cout << ", ";
            }
            std::cout << mesh_index;
            sep = true;
        }
        std::cout << ")";
    }

    // finish the line
    std::cout << std::endl;

    // in verbose mode, print the transform data as well
    if (verbose) {
        // indent to use
        std::string indentadd;
        if (last) {
            indentadd += "  ";
        } else {
            indentadd += TREE_CONTINUE;
        } // "| "..
        if (node->mNumChildren == 0) {
            indentadd += "  ";
        } else {
            indentadd += TREE_CONTINUE;
        } // .."| "
        aiVector3D s, r, t;
        node->mTransformation.Decompose(s, r, t);
        if (s.x != 1.0 || s.y != 1.0 || s.z != 1.0) {
            std::cout << indent << indentadd;
            printf("  S:[%f %f %f]\n", s.x, s.y, s.z);
        }
        if (r.x || r.y || r.z) {
            std::cout << indent << indentadd;
            printf("  R:[%f %f %f]\n", r.x, r.y, r.z);
        }
        if (t.x || t.y || t.z) {
            std::cout << indent << indentadd;
            printf("  T:[%f %f %f]\n", t.x, t.y, t.z);
        }
    }

    // and recurse
    std::string nextIndent;
    if (first) {
        nextIndent = indent;
    } else if (last) {
        nextIndent = indent + "  ";
    } else {
        nextIndent = indent + TREE_CONTINUE;
    } // "| "
    for (size_t i = 0; i < node->mNumChildren; ++i) {
        bool lastone = (i == node->mNumChildren - 1);
        PrintHierarchy(
                node->mChildren[i],
                nextIndent,
                verbose,
                lastone,
                false);
    }
}

// -----------------------------------------------------------------------------------
// Implementation of the assimp info utility to print basic file info
int Assimp_Info(const char *const *params, unsigned int num) {
    // asssimp info <file> [-r]
    if (num < 1) {
        printf("assimp info: Invalid number of arguments. "
               "See \'assimp info --help\'\n");
        return AssimpCmdError::InvalidNumberOfArguments;
    }

    // --help
    if (!strcmp(params[0], "-h") || !strcmp(params[0], "--help") || !strcmp(params[0], "-?")) {
        printf("%s", AICMD_MSG_INFO_HELP_E);
        return AssimpCmdError::Success;
    }

    const std::string in = std::string(params[0]);

    // get -r and -v arguments
    bool raw = false;
    bool verbose = false;
    bool silent = false;
    for (unsigned int i = 1; i < num; ++i) {
        if (!strcmp(params[i], "--raw") || !strcmp(params[i], "-r")) {
            raw = true;
        }
        if (!strcmp(params[i], "--verbose") || !strcmp(params[i], "-v")) {
            verbose = true;
        }
        if (!strcmp(params[i], "--silent") || !strcmp(params[i], "-s")) {
            silent = true;
        }
    }

    // Verbose and silent at the same time are not allowed
    if (verbose && silent) {
        printf("assimp info: Invalid arguments, verbose and silent at the same time are forbidden. ");
        return AssimpCmdInfoError::InvalidCombinaisonOfArguments;
    }

    // Parse post-processing flags unless -r was specified
    ImportData import;
    if (!raw) {
        // get import flags
        ProcessStandardArguments(import, params + 1, num - 1);

        //No custom post process flags defined, we set all the post process flags active
        if (import.ppFlags == 0)
            import.ppFlags |= aiProcessPreset_TargetRealtime_MaxQuality;
    }

    // import the main model
    const aiScene *scene = ImportModel(import, in);
    if (!scene) {
        printf("assimp info: Unable to load input file %s\n",
                in.c_str());
        return AssimpCmdError::FailedToLoadInputFile;
    }

    aiMemoryInfo mem;
    globalImporter->GetMemoryRequirements(mem);

    static const char *format_string =
            "Memory consumption: %i B\n"
            "Nodes:              %i\n"
            "Maximum depth       %i\n"
            "Meshes:             %i\n"
            "Animations:         %i\n"
            "Textures (embed.):  %i\n"
            "Materials:          %i\n"
            "Cameras:            %i\n"
            "Lights:             %i\n"
            "Vertices:           %i\n"
            "Faces:              %i\n"
            "Bones:              %i\n"
            "Animation Channels: %i\n"
            "Primitive Types:    %s\n"
            "Average faces/mesh  %i\n"
            "Average verts/mesh  %i\n"
            "Minimum point      (%f %f %f)\n"
            "Maximum point      (%f %f %f)\n"
            "Center point       (%f %f %f)\n"

            ;

    aiVector3D special_points[3];
    FindSpecialPoints(scene, special_points);
    printf(format_string,
            mem.total,
            CountNodes(scene->mRootNode),
            GetMaxDepth(scene->mRootNode),
            scene->mNumMeshes,
            scene->mNumAnimations,
            scene->mNumTextures,
            scene->mNumMaterials,
            scene->mNumCameras,
            scene->mNumLights,
            CountVertices(scene),
            CountFaces(scene),
            CountBones(scene),
            CountAnimChannels(scene),
            FindPTypes(scene).c_str(),
            GetAvgFacePerMesh(scene),
            GetAvgVertsPerMesh(scene),
            special_points[0][0], special_points[0][1], special_points[0][2],
            special_points[1][0], special_points[1][1], special_points[1][2],
            special_points[2][0], special_points[2][1], special_points[2][2]);

    if (silent) {
        printf("\n");
        return AssimpCmdError::Success;
    }

    // meshes
    if (scene->mNumMeshes) {
        printf("\nMeshes:  (name) [vertices / bones / faces | primitive_types]\n");
    }
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh *mesh = scene->mMeshes[i];
        printf("    %d (%s)", i, mesh->mName.C_Str());
        printf(
                ": [%d / %d / %d |",
                mesh->mNumVertices,
                mesh->mNumBones,
                mesh->mNumFaces);
        const unsigned int ptypes = mesh->mPrimitiveTypes;
        if (ptypes & aiPrimitiveType_POINT) {
            printf(" point");
        }
        if (ptypes & aiPrimitiveType_LINE) {
            printf(" line");
        }
        if (ptypes & aiPrimitiveType_TRIANGLE) {
            printf(" triangle");
        }
        if (ptypes & aiPrimitiveType_POLYGON) {
            printf(" polygon");
        }
        printf("]\n");
    }

    // materials
    if (scene->mNumMaterials)
        printf("\nNamed Materials:");
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial *mat = scene->mMaterials[i];
        aiString name = mat->GetName();
        printf("\n    \'%s\'", name.data);
        if (mat->mNumProperties)
            printf(" (prop) [index / bytes | texture semantic]");
        for (unsigned p = 0; p < mat->mNumProperties; p++) {
            const aiMaterialProperty *prop = mat->mProperties[p];
            const aiTextureType textype = static_cast<aiTextureType>(prop->mSemantic);
            printf("\n        %d (%s): [%d / %d | %s]",
                    p,
                    prop->mKey.data,
                    prop->mIndex,
                    prop->mDataLength,
                    aiTextureTypeToString(textype));
        }
    }
    if (scene->mNumMaterials) {
        printf("\n");
    }

    // textures
    unsigned int total = 0;
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        aiString name;
        static const aiTextureType types[] = {
            aiTextureType_NONE,
            aiTextureType_DIFFUSE,
            aiTextureType_SPECULAR,
            aiTextureType_AMBIENT,
            aiTextureType_EMISSIVE,
            aiTextureType_HEIGHT,
            aiTextureType_NORMALS,
            aiTextureType_SHININESS,
            aiTextureType_OPACITY,
            aiTextureType_DISPLACEMENT,
            aiTextureType_LIGHTMAP,
            aiTextureType_REFLECTION,
            aiTextureType_BASE_COLOR,
            aiTextureType_NORMAL_CAMERA,
            aiTextureType_EMISSION_COLOR,
            aiTextureType_METALNESS,
            aiTextureType_DIFFUSE_ROUGHNESS,
            aiTextureType_AMBIENT_OCCLUSION,
            aiTextureType_UNKNOWN
        };
        for (unsigned int type = 0; type < sizeof(types) / sizeof(types[0]); ++type) {
            for (unsigned int idx = 0; AI_SUCCESS == aiGetMaterialString(scene->mMaterials[i],
                                                             AI_MATKEY_TEXTURE(types[type], idx), &name);
                    ++idx) {
                printf("%s\n    \'%s\'", (total++ ? "" : "\nTexture Refs:"), name.data);
            }
        }
    }
    if (total) {
        printf("\n");
    }

    // animations
    total = 0;
    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        if (scene->mAnimations[i]->mName.length) {
            printf("%s\n     \'%s\'", (total++ ? "" : "\nNamed Animations:"), scene->mAnimations[i]->mName.data);
        }
    }
    if (total) {
        printf("\n");
    }

    // node hierarchy
    printf("\nNode hierarchy:\n");
    PrintHierarchy(scene->mRootNode, "", verbose);

    printf("\n");
    return AssimpCmdError::Success;
}
