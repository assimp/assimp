/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

/** @file  UnrealLoader.cpp
 *  @brief Implementation of the UNREAL (*.3D) importer class
 *
 *  Sources:
 *    http://local.wasp.uwa.edu.au/~pbourke/dataformats/unreal/
 */

#ifndef ASSIMP_BUILD_NO_3D_IMPORTER

#include "AssetLib/Unreal/UnrealLoader.h"
#include "PostProcessing/ConvertToLHProcess.h"

#include <assimp/ParsingUtils.h>
#include <assimp/StreamReader.h>
#include <assimp/fast_atof.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>

#include <cstdint>
#include <memory>

namespace Assimp {

namespace Unreal {

// Mesh-specific fags.
enum MeshFlags {
    MF_INVALID = -1,            // Not set
    MF_NORMAL_OS = 0,           // Normal one-sided
    MF_NORMAL_TS = 1,           // Normal two-sided
    MF_NORMAL_TRANS_TS = 2,     // Translucent two-sided
    MF_NORMAL_MASKED_TS = 3,    // Masked two-sided
    MF_NORMAL_MOD_TS = 4,       // Modulation blended two-sided
    MF_WEAPON_PLACEHOLDER = 8   // Placeholder triangle for weapon positioning (invisible)
};

// a single triangle
struct Triangle {
    uint16_t mVertex[3];        // Vertex indices
    char mType;                 // James' Mesh Type
    char mColor;                // Color for flat and Gourand Shaded
    unsigned char mTex[3][2];   // Texture UV coordinates
    unsigned char mTextureNum;  // Source texture offset
    char mFlags;                // Unreal Mesh Flags (unused)
    unsigned int matIndex;      // Material index
};

// temporary representation for a material
struct TempMat {
    TempMat() :
            type(MF_NORMAL_OS), tex(), numFaces(0) {}

    explicit TempMat(const Triangle &in) :
            type((Unreal::MeshFlags)in.mType), tex(in.mTextureNum), numFaces(0) {}

    // type of mesh
    Unreal::MeshFlags type;

    // index of texture
    unsigned int tex;

    // number of faces using us
    unsigned int numFaces;

    // for std::find
    bool operator==(const TempMat &o) {
        return (tex == o.tex && type == o.type);
    }
};

// A single vertex in an unsigned int 32 bit
struct Vertex {
    int32_t X : 11;
    int32_t Y : 11;
    int32_t Z : 10;
};

// UNREAL vertex compression
inline void CompressVertex(const aiVector3D &v, uint32_t &out) {
    union {
        Vertex n;
        int32_t t;
    };
    t = 0;
    n.X = (int32_t)v.x;
    n.Y = (int32_t)v.y;
    n.Z = (int32_t)v.z;
    ::memcpy(&out, &t, sizeof(int32_t));
}

// UNREAL vertex decompression
inline void DecompressVertex(aiVector3D &v, int32_t in) {
    union {
        Vertex n;
        int32_t i;
    };
    i = in;

    v.x = (float)n.X;
    v.y = (float)n.Y;
    v.z = (float)n.Z;
}

} // end namespace Unreal

static constexpr aiImporterDesc desc = {
    "Unreal Mesh Importer",
    "",
    "",
    "",
    aiImporterFlags_SupportTextFlavour,
    0,
    0,
    0,
    0,
    "3d uc"
};

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
UnrealImporter::UnrealImporter() :
        mConfigFrameID(0), mConfigHandleFlags(true) {
    // empty
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file.
bool UnrealImporter::CanRead(const std::string &filename, IOSystem * /*pIOHandler*/, bool /*checkSig*/) const {
    return SimpleExtensionCheck(filename, "3d", "uc");
}

// ------------------------------------------------------------------------------------------------
// Build a string of all file extensions supported
const aiImporterDesc *UnrealImporter::GetInfo() const {
    return &desc;
}

// ------------------------------------------------------------------------------------------------
// Setup configuration properties for the loader
void UnrealImporter::SetupProperties(const Importer *pImp) {
    // The
    // AI_CONFIG_IMPORT_UNREAL_KEYFRAME option overrides the
    // AI_CONFIG_IMPORT_GLOBAL_KEYFRAME option.
    mConfigFrameID = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_UNREAL_KEYFRAME, -1);
    if (static_cast<unsigned int>(-1) == mConfigFrameID) {
        mConfigFrameID = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_GLOBAL_KEYFRAME, 0);
    }

    // AI_CONFIG_IMPORT_UNREAL_HANDLE_FLAGS, default is true
    mConfigHandleFlags = (0 != pImp->GetPropertyInteger(AI_CONFIG_IMPORT_UNREAL_HANDLE_FLAGS, 1));
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure.
void UnrealImporter::InternReadFile(const std::string &pFile,
        aiScene *pScene, IOSystem *pIOHandler) {
    // For any of the 3 files being passed get the three correct paths
    // First of all, determine file extension
    std::string::size_type pos = pFile.find_last_of('.');
    std::string extension = GetExtension(pFile);

    std::string d_path, a_path, uc_path;
    if (extension == "3d") {
        // jjjj_d.3d
        // jjjj_a.3d
        pos = pFile.find_last_of('_');
        if (std::string::npos == pos) {
            throw DeadlyImportError("UNREAL: Unexpected naming scheme");
        }
        extension = pFile.substr(0, pos);
    } else {
        extension = pFile.substr(0, pos);
    }

    // build proper paths
    d_path = extension + "_d.3d";
    a_path = extension + "_a.3d";
    uc_path = extension + ".uc";

    ASSIMP_LOG_DEBUG("UNREAL: data file is ", d_path);
    ASSIMP_LOG_DEBUG("UNREAL: aniv file is ", a_path);
    ASSIMP_LOG_DEBUG("UNREAL: uc file is ", uc_path);

    // and open the files ... we can't live without them
    std::unique_ptr<IOStream> p(pIOHandler->Open(d_path));
    if (!p)
        throw DeadlyImportError("UNREAL: Unable to open _d file");
    StreamReaderLE d_reader(pIOHandler->Open(d_path));

    const uint16_t numTris = d_reader.GetI2();
    const uint16_t numVert = d_reader.GetI2();
    d_reader.IncPtr(44);
    if (!numTris || numVert < 3) {
        throw DeadlyImportError("UNREAL: Invalid number of vertices/triangles");
    }

    // maximum texture index
    unsigned int maxTexIdx = 0;

    // collect triangles
    std::vector<Unreal::Triangle> triangles(numTris);
    for (auto &tri : triangles) {
        for (unsigned int i = 0; i < 3; ++i) {
            tri.mVertex[i] = d_reader.GetI2();
            if (tri.mVertex[i] >= numTris) {
                ASSIMP_LOG_WARN("UNREAL: vertex index out of range");
                tri.mVertex[i] = 0;
            }
        }
        tri.mType = d_reader.GetI1();

        // handle mesh flagss?
        if (mConfigHandleFlags) {
            tri.mType = Unreal::MF_NORMAL_OS;
        } else {
            // ignore MOD and MASKED for the moment, treat them as two-sided
            if (tri.mType == Unreal::MF_NORMAL_MOD_TS || tri.mType == Unreal::MF_NORMAL_MASKED_TS)
                tri.mType = Unreal::MF_NORMAL_TS;
        }
        d_reader.IncPtr(1);

        for (unsigned int i = 0; i < 3; ++i) {
            for (unsigned int i2 = 0; i2 < 2; ++i2) {
                tri.mTex[i][i2] = d_reader.GetI1();
            }
        }

        tri.mTextureNum = d_reader.GetI1();
        maxTexIdx = std::max(maxTexIdx, (unsigned int)tri.mTextureNum);
        d_reader.IncPtr(1);
    }

    p.reset(pIOHandler->Open(a_path));
    if (!p) {
        throw DeadlyImportError("UNREAL: Unable to open _a file");
    }
    StreamReaderLE a_reader(pIOHandler->Open(a_path));

    // read number of frames
    const uint32_t numFrames = a_reader.GetI2();
    if (mConfigFrameID >= numFrames) {
        throw DeadlyImportError("UNREAL: The requested frame does not exist");
    }

    // read aniv file length
    if (uint32_t st = a_reader.GetI2(); st != numVert * 4u) {
        throw DeadlyImportError("UNREAL: Unexpected aniv file length");
    }

    // skip to our frame
    a_reader.IncPtr(mConfigFrameID * numVert * 4);

    // collect vertices
    std::vector<aiVector3D> vertices(numVert);
    for (auto &vertex : vertices) {
        int32_t val = a_reader.GetI4();
        Unreal::DecompressVertex(vertex, val);
    }

    // list of textures.
    std::vector<std::pair<unsigned int, std::string>> textures;

    // allocate the output scene
    aiNode *nd = pScene->mRootNode = new aiNode();
    nd->mName.Set("<UnrealRoot>");

    // we can live without the uc file if necessary
    std::unique_ptr<IOStream> pb(pIOHandler->Open(uc_path));
    if (pb) {

        std::vector<char> _data;
        TextFileToBuffer(pb.get(), _data);
        const char *data = &_data[0];
        const char *end = &_data[_data.size() - 1] + 1;

        std::vector<std::pair<std::string, std::string>> tempTextures;

        // do a quick search in the UC file for some known, usually texture-related, tags
        for (; *data; ++data) {
            if (TokenMatchI(data, "#exec", 5)) {
                SkipSpacesAndLineEnd(&data, end);

                // #exec TEXTURE IMPORT [...] NAME=jjjjj [...] FILE=jjjj.pcx [...]
                if (TokenMatchI(data, "TEXTURE", 7)) {
                    SkipSpacesAndLineEnd(&data, end);

                    if (TokenMatchI(data, "IMPORT", 6)) {
                        tempTextures.emplace_back();
                        std::pair<std::string, std::string> &me = tempTextures.back();
                        for (; !IsLineEnd(*data); ++data) {
                            if (!ASSIMP_strincmp(data, "NAME=", 5)) {
                                const char *d = data += 5;
                                for (; !IsSpaceOrNewLine(*data); ++data)
                                    ;
                                me.first = std::string(d, (size_t)(data - d));
                            } else if (!ASSIMP_strincmp(data, "FILE=", 5)) {
                                const char *d = data += 5;
                                for (; !IsSpaceOrNewLine(*data); ++data)
                                    ;
                                me.second = std::string(d, (size_t)(data - d));
                            }
                        }
                        if (!me.first.length() || !me.second.length()) {
                            tempTextures.pop_back();
                        }
                    }
                }
                // #exec MESHMAP SETTEXTURE MESHMAP=box NUM=1 TEXTURE=Jtex1
                // #exec MESHMAP SCALE MESHMAP=box X=0.1 Y=0.1 Z=0.2
                else if (TokenMatchI(data, "MESHMAP", 7)) {
                    SkipSpacesAndLineEnd(&data, end);

                    if (TokenMatchI(data, "SETTEXTURE", 10)) {

                        textures.emplace_back();
                        std::pair<unsigned int, std::string> &me = textures.back();

                        for (; !IsLineEnd(*data); ++data) {
                            if (!ASSIMP_strincmp(data, "NUM=", 4)) {
                                data += 4;
                                me.first = strtoul10(data, &data);
                            } else if (!ASSIMP_strincmp(data, "TEXTURE=", 8)) {
                                data += 8;
                                const char *d = data;
                                for (; !IsSpaceOrNewLine(*data); ++data);
                                me.second = std::string(d, (size_t)(data - d));

                                // try to find matching path names, doesn't care if we don't find them
                                for (std::vector<std::pair<std::string, std::string>>::const_iterator it = tempTextures.begin();
                                        it != tempTextures.end(); ++it) {
                                    if ((*it).first == me.second) {
                                        me.second = (*it).second;
                                        break;
                                    }
                                }
                            }
                        }
                    } else if (TokenMatchI(data, "SCALE", 5)) {

                        for (; !IsLineEnd(*data); ++data) {
                            if (data[0] == 'X' && data[1] == '=') {
                                data = fast_atoreal_move<float>(data + 2, (float &)nd->mTransformation.a1);
                            } else if (data[0] == 'Y' && data[1] == '=') {
                                data = fast_atoreal_move<float>(data + 2, (float &)nd->mTransformation.b2);
                            } else if (data[0] == 'Z' && data[1] == '=') {
                                data = fast_atoreal_move<float>(data + 2, (float &)nd->mTransformation.c3);
                            }
                        }
                    }
                }
            }
        }
    } else {
        ASSIMP_LOG_ERROR("Unable to open .uc file");
    }

    std::vector<Unreal::TempMat> materials;
    materials.reserve(textures.size() * 2 + 5);

    // find out how many output meshes and materials we'll have and build material indices
    for (auto &tri : triangles) {
        Unreal::TempMat mat(tri);
        auto nt = std::find(materials.begin(), materials.end(), mat);
        if (nt == materials.end()) {
            // add material
            tri.matIndex = static_cast<unsigned int>(materials.size());
            mat.numFaces = 1;
            materials.push_back(mat);

            ++pScene->mNumMeshes;
        } else {
            tri.matIndex = static_cast<unsigned int>(nt - materials.begin());
            ++nt->numFaces;
        }
    }

    if (!pScene->mNumMeshes) {
        throw DeadlyImportError("UNREAL: Unable to find valid mesh data");
    }

    // allocate meshes and bind them to the node graph
    pScene->mMeshes = new aiMesh *[pScene->mNumMeshes];
    pScene->mMaterials = new aiMaterial *[pScene->mNumMaterials = pScene->mNumMeshes];

    nd->mNumMeshes = pScene->mNumMeshes;
    nd->mMeshes = new unsigned int[nd->mNumMeshes];
    for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
        aiMesh *m = pScene->mMeshes[i] = new aiMesh();
        m->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

        const unsigned int num = materials[i].numFaces;
        m->mFaces = new aiFace[num];
        m->mVertices = new aiVector3D[num * 3];
        m->mTextureCoords[0] = new aiVector3D[num * 3];

        nd->mMeshes[i] = i;

        // create materials, too
        aiMaterial *mat = new aiMaterial();
        pScene->mMaterials[i] = mat;

        // all white by default - texture rulez
        aiColor3D color(1.f, 1.f, 1.f);

        aiString s;
        ::ai_snprintf(s.data, AI_MAXLEN, "mat%u_tx%u_", i, materials[i].tex);

        // set the two-sided flag
        if (materials[i].type == Unreal::MF_NORMAL_TS) {
            const int twosided = 1;
            mat->AddProperty(&twosided, 1, AI_MATKEY_TWOSIDED);
            ::strcat(s.data, "ts_");
        } else
            ::strcat(s.data, "os_");

        // make TRANS faces 90% opaque that RemRedundantMaterials won't catch us
        if (materials[i].type == Unreal::MF_NORMAL_TRANS_TS) {
            const float opac = 0.9f;
            mat->AddProperty(&opac, 1, AI_MATKEY_OPACITY);
            ::strcat(s.data, "tran_");
        } else
            ::strcat(s.data, "opaq_");

        // a special name for the weapon attachment point
        if (materials[i].type == Unreal::MF_WEAPON_PLACEHOLDER) {
            s.length = ::ai_snprintf(s.data, AI_MAXLEN, "$WeaponTag$");
            color = aiColor3D(0.f, 0.f, 0.f);
        }

        // set color and name
        mat->AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);
        s.length = static_cast<ai_uint32>(::strlen(s.data));
        mat->AddProperty(&s, AI_MATKEY_NAME);

        // set texture, if any
        const unsigned int tex = materials[i].tex;
        for (auto it = textures.begin(); it != textures.end(); ++it) {
            if ((*it).first == tex) {
                s.Set((*it).second);
                mat->AddProperty(&s, AI_MATKEY_TEXTURE_DIFFUSE(0));
                break;
            }
        }
    }

    // fill them.
    for (const Unreal::Triangle &tri : triangles) {
        Unreal::TempMat mat(tri);
        auto nt = std::find(materials.begin(), materials.end(), mat);

        aiMesh *mesh = pScene->mMeshes[nt - materials.begin()];
        aiFace &f = mesh->mFaces[mesh->mNumFaces++];
        f.mIndices = new unsigned int[f.mNumIndices = 3];

        for (unsigned int i = 0; i < 3; ++i, mesh->mNumVertices++) {
            f.mIndices[i] = mesh->mNumVertices;

            mesh->mVertices[mesh->mNumVertices] = vertices[tri.mVertex[i]];
            mesh->mTextureCoords[0][mesh->mNumVertices] = aiVector3D(tri.mTex[i][0] / 255.f, 1.f - tri.mTex[i][1] / 255.f, 0.f);
        }
    }

    // convert to RH
    MakeLeftHandedProcess hero;
    hero.Execute(pScene);

    FlipWindingOrderProcess flipper;
    flipper.Execute(pScene);
}

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_3D_IMPORTER
