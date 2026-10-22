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

/** @file  GEOLoader.cpp
 *  @brief Implementation of the GEO importer class (Videoscape .geo)
 *
 *  Ported from the Assimp 5.x ZsoltTech private tree (AssetLib/GEO).
 */

#ifndef ASSIMP_BUILD_NO_GEO_IMPORTER

#include "GEOLoader.h"
#include "GEOColorTable.h"
#include "GEOHelper.h"

#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/ParsingUtils.h>
#include <assimp/StringUtils.h>
#include <assimp/fast_atof.h>
#include <assimp/importerdesc.h>
#include <assimp/light.h>
#include <assimp/scene.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace Assimp {

namespace {
constexpr unsigned int kMaxGeoLamps = 100000u;
} // namespace

// *INDENT-OFF*
static constexpr aiImporterDesc desc = {
        "Videoscape GEO Importer",
        "ZsoltTech.Com <arris@zsolttech.com>",
        "",
        "https://paulbourke.net/dataformats/geo/",
        aiImporterFlags_SupportTextFlavour |
                aiImporterFlags_LimitedSupport |
                aiImporterFlags_Experimental,
        0,
        0,
        0,
        0,
        "3dg geo gour"
};
// *INDENT-ON*

// ------------------------------------------------------------------------------------------------
GEOImporter::GEOImporter() :
        flav(Mesh_with_coloured_faces),
        rgbH(false),
        mScene(nullptr),
        buffer(nullptr),
        sz(nullptr),
        mesh(nullptr),
        faces(nullptr),
        lastcolor(0),
        color(0),
        verts(nullptr),
        colOut(nullptr) {
    line[0] = '\0';
    (void)lastcolor;
    (void)colOut;
}

// ------------------------------------------------------------------------------------------------
GEOImporter::~GEOImporter() = default;

// ------------------------------------------------------------------------------------------------
bool GEOImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler,
        bool checkSig) const {
    if (SimpleExtensionCheck(pFile, "geo", "3dg", "gour")) {
        return true;
    }
    if (!GetExtension(pFile).length() || checkSig) {
        if (!pIOHandler) {
            return true;
        }
        static const char *tokens[] = { "gour", "3dg" }; /* ref: 3dg1 3dg2 3dg3 gour */
        return SearchFileHeaderForToken(pIOHandler, pFile, tokens, AI_COUNT_OF(tokens));
    }
    return false;
}

// ------------------------------------------------------------------------------------------------
const aiImporterDesc *GEOImporter::GetInfo() const {
    return &desc;
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::SetupProperties(const Importer * /*pImp*/) {
    ASSIMP_LOG_DEBUG("GEO: SetupProperties");
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadFile(const std::string &pFile, aiScene *_pScene,
        IOSystem *pIOHandler) {
    std::unique_ptr<IOStream> file(pIOHandler->Open(pFile, "rb"));

    if (file == nullptr) {
        throw DeadlyImportError("Failed to open GEO file ", pFile, ".");
    }

    mScene = _pScene;

    std::vector<char> mBuffer2;
    TextFileToBuffer(file.get(), mBuffer2);
    buffer = &mBuffer2[0];

    GetNextLine(buffer, line);
    while (('G' == line[0] || ('3' == line[0] && 'D' == line[1]) || '#' == line[0])) {
        if ('G' == line[0] || ('3' == line[0] && 'D' == line[1])) {
            switch (line[3]) {
            case '1':
                flav = Mesh_with_coloured_faces;
                ASSIMP_LOG_DEBUG("Signature: ", line, ", must read color with face data");
                break;
            case '2':
                flav = Lamp;
                ASSIMP_LOG_DEBUG("Signature: ", line, ", must not read any color data, just lights, some postprocess steps will fail.");
                break;
            case '3':
                flav = Gouraud_curves_or_NURBS_surfaces;
                ASSIMP_LOG_DEBUG("Signature: ", line, ", must not read any color data, but surfaces or curves");
                break;
            case 'R':
                flav = Mesh_with_coloured_vertices;
                ASSIMP_LOG_DEBUG("Signature: ", line, ", must read color with vertex data");
                break;
            default:
                ASSIMP_LOG_WARN("Unknown Signature: ", line);
                throw DeadlyImportError("GEO: Unknown file version");
            }
        }
        GetNextLine(buffer, line); // skip the signature line and comment lines (#...)
    }

    m_progress->UpdateFileRead(1, 5);

    sz = line;

    const unsigned int numElementsToImport = strtoul10(sz, &sz);

    // 3DG2 lamp files are lights-only: no mesh, mark incomplete so ValidateDS accepts
    // mNumMeshes == 0 (Assimp is otherwise mesh-centric).
    if (flav == Lamp) {
        InternReadLamp(numElementsToImport);
        m_progress->UpdateFileRead(4, 5);
        InternReadFinish();
        return;
    }

    if (flav == Gouraud_curves_or_NURBS_surfaces) {
        InternReadFbS(numElementsToImport); // throws: not supported yet
    }

    mScene->mMeshes = new aiMesh *[mScene->mNumMeshes = 1];
    mesh = mScene->mMeshes[0] = new aiMesh();
    tempPositions.resize(numElementsToImport);

    if (flav == Mesh_with_coloured_faces) {
        InternReadVertices(numElementsToImport, false);
    } else if (flav == Mesh_with_coloured_vertices) {
        InternReadVertices(numElementsToImport, true);
    } else {
        throw DeadlyImportError("GEO: Never see me, bug in assimp's api design.");
    }

    m_progress->UpdateFileRead(2, 5);

    const char *old = buffer;

    // First pass: count faces / expanded vertices (exact allocation, no fixed cap).
    unsigned int numFaces = 0;
    unsigned int numVertices = 0;
    while (GetNextLine(buffer, line)) {
        sz = line;
        const unsigned int nidx = strtoul10(sz, &sz);
        if (!nidx) {
            ASSIMP_LOG_ERROR("GEO: Faces with zero indices aren't allowed");
            continue;
        }
        ++numFaces;
        numVertices += nidx;
    }

    if (!numVertices) {
        throw DeadlyImportError("GEO: There are no valid faces");
    }

    ASSIMP_LOG_DEBUG("GEO: face storage needs ", numFaces, " faces / ", numVertices, " vertices");

    mesh->mNumFaces = numFaces;
    mesh->mNumVertices = numVertices;
    faces = mesh->mFaces = new aiFace[numFaces];
    verts = mesh->mVertices = new aiVector3D[numVertices];

    buffer = old;
    m_progress->UpdateFileRead(3, 5);

    if (flav == Mesh_with_coloured_faces) {
        InternReadFaces(mesh->mNumFaces, true);
    } else {
        InternReadFaces(mesh->mNumFaces, false);
    }

    m_progress->UpdateFileRead(4, 5);

    InternReadFinish();
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadLamp(unsigned int count) {
    ASSIMP_LOG_DEBUG("GEO: Has to import ", count, " light(s)");

    if (!count) {
        throw DeadlyImportError("GEO: 3DG2 lamp file has zero lights");
    }
    if (count > kMaxGeoLamps) {
        throw DeadlyImportError("GEO: lamp count too large");
    }

    mScene->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;

    // Own lights until the full set is parsed, then transfer to the scene.
    // Avoids LeakSanitizer hits if a corrupt/fuzzed file throws mid-lamp.
    std::vector<std::unique_ptr<aiLight>> lights;
    lights.reserve(count);

    for (unsigned int i = 0; i < count; ++i) {
        if (!GetNextLine(buffer, line)) {
            throw DeadlyImportError("GEO: Unexpected EOF while reading lamps");
        }
        sz = line;
        // Videoscape: 0 point, 1 spot, 2 sun — not Assimp's enum ordinals.
        const unsigned int geoType = strtoul10(sz, &sz);
        aiLightSourceType mapped = aiLightSource_POINT;
        if (geoType == 1) {
            mapped = aiLightSource_SPOT;
        } else if (geoType == 2) {
            mapped = aiLightSource_DIRECTIONAL;
        } else if (geoType != 0) {
            ASSIMP_LOG_WARN("GEO: unknown lamp type ", geoType, ", treating as point");
        }

        auto tmpLight = std::make_unique<aiLight>();
        tmpLight->mName.length = static_cast<ai_uint32>(
                ::ai_snprintf(tmpLight->mName.data, AI_MAXLEN, "Lamp%04u", i + 1));
        tmpLight->mType = mapped;
        ASSIMP_LOG_DEBUG("GEO: Create light: ", tmpLight->mName.C_Str(), " geoType=", geoType);

        if (!GetNextLine(buffer, line)) {
            throw DeadlyImportError("GEO: Unexpected EOF (spotsize/blend)");
        }
        sz = line;
        float spotsizeDeg = 0.f;
        float spotblend = 0.f;
        sz = fast_atoreal_move<float>(sz, spotsizeDeg);
        SkipSpaces(&sz, line + sizeof(line));
        fast_atoreal_move<float>(sz, spotblend);
        // Assimp cones are radians; outer = beam size, inner shrinks with blend.
        if (mapped == aiLightSource_SPOT) {
            const float outer = spotsizeDeg * AI_MATH_PI_F / 180.f;
            const float blend = std::max(0.f, std::min(spotblend, 1.f));
            tmpLight->mAngleOuterCone = outer;
            tmpLight->mAngleInnerCone = outer * (1.f - blend);
        }

        if (!GetNextLine(buffer, line)) {
            throw DeadlyImportError("GEO: Unexpected EOF (lamp color/energy)");
        }
        sz = line;
        float energy = 1.f;
        aiColor3D &c = tmpLight->mColorDiffuse;
        sz = fast_atoreal_move<float>(sz, (float &)c.r);
        SkipSpaces(&sz, line + sizeof(line));
        sz = fast_atoreal_move<float>(sz, (float &)c.g);
        SkipSpaces(&sz, line + sizeof(line));
        sz = fast_atoreal_move<float>(sz, (float &)c.b);
        SkipSpaces(&sz, line + sizeof(line));
        if (sz && *sz) {
            fast_atoreal_move<float>(sz, energy);
        }
        c.r *= energy;
        c.g *= energy;
        c.b *= energy;
        tmpLight->mColorSpecular = c;
        tmpLight->mColorAmbient = aiColor3D(c.r * 0.1f, c.g * 0.1f, c.b * 0.1f);

        if (!GetNextLine(buffer, line)) {
            throw DeadlyImportError("GEO: Unexpected EOF (lamp position)");
        }
        sz = line;
        aiVector3D &p = tmpLight->mPosition;
        sz = fast_atoreal_move<float>(sz, (float &)p.x);
        SkipSpaces(&sz, line + sizeof(line));
        sz = fast_atoreal_move<float>(sz, (float &)p.y);
        SkipSpaces(&sz, line + sizeof(line));
        fast_atoreal_move<float>(sz, (float &)p.z);

        if (!GetNextLine(buffer, line)) {
            throw DeadlyImportError("GEO: Unexpected EOF (lamp direction)");
        }
        sz = line;
        aiVector3D &d = tmpLight->mDirection;
        sz = fast_atoreal_move<float>(sz, (float &)d.x);
        SkipSpaces(&sz, line + sizeof(line));
        sz = fast_atoreal_move<float>(sz, (float &)d.y);
        SkipSpaces(&sz, line + sizeof(line));
        fast_atoreal_move<float>(sz, (float &)d.z);

        // Sensible defaults for point/spot attenuation (GEO has none).
        if (mapped != aiLightSource_DIRECTIONAL) {
            tmpLight->mAttenuationConstant = 1.f;
            tmpLight->mAttenuationLinear = 0.f;
            tmpLight->mAttenuationQuadratic = 0.f;
        }

        lights.push_back(std::move(tmpLight));
    }

    mScene->mNumLights = static_cast<unsigned int>(lights.size());
    mScene->mLights = new aiLight *[mScene->mNumLights]();
    for (unsigned int i = 0; i < mScene->mNumLights; ++i) {
        mScene->mLights[i] = lights[i].release();
    }
}

// ------------------------------------------------------------------------------------------------
[[noreturn]] void GEOImporter::InternReadFbS(unsigned int count) {
    ASSIMP_LOG_DEBUG("GEO: Has to import type ", count, " form(s)");
    while (GetNextLine(buffer, line)) {
        sz = line;
    }
    throw DeadlyImportError("GEO: Curves and surfaces not supported yet.");
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadVertices(unsigned int count, bool colored) {
    ASSIMP_LOG_DEBUG("GEO: Has to import ", count,
            colored ? " colored vertex/vertices" : " not colored vertex/vertices");

    if (colored) {
        tempColors.resize(count);
    }

    for (unsigned int i = 0; i < count; i++) {
        if (!GetNextLine(buffer, line)) {
            ASSIMP_LOG_ERROR("GEO: The number of verts in the header is incorrect");
            break;
        }

        aiVector3D &v = tempPositions[i];

        sz = line;
        sz = fast_atoreal_move<float>(sz, (float &)v.x);
        SkipSpaces(&sz, line + sizeof(line));
        sz = fast_atoreal_move<float>(sz, (float &)v.y);
        SkipSpaces(&sz, line + sizeof(line));
        sz = fast_atoreal_move<float>(sz, (float &)v.z);

        if (colored) {
            InternReadColor(i);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadFaces(unsigned int count, bool faceColors) {
    ASSIMP_LOG_DEBUG("GEO: Has to import ", count,
            faceColors ? " colored face(s)" : " not colored face(s)");

    if (faceColors) {
        tempColors.resize(count);
    }

    for (unsigned int i = 0, p = 0; i < count;) {
        if (!GetNextLine(buffer, line)) {
            return;
        }

        sz = line;

        const unsigned int idx = strtoul10(sz, &sz);
        if (!idx) {
            continue;
        }

        faces->mIndices = new unsigned int[faces->mNumIndices = idx];
        if (!mesh->mColors[0]) {
            mesh->mColors[0] = new aiColor4D[mesh->mNumVertices];
            ASSIMP_LOG_DEBUG("GEO: got new mesh");
        }
        for (unsigned int m = 0; m < faces->mNumIndices; m++) {
            SkipSpaces(&sz, line + sizeof(line));
            const unsigned int pos = strtoul10(sz, &sz);
            faces->mIndices[m] = p++;
            *verts++ = tempPositions[pos];
            if (!faceColors) {
                mesh->mColors[0][faces->mIndices[m]] = tempColors[pos];
            }
        }

        if (faceColors) {
            InternReadColor(i);
            for (unsigned int l = 0; l < faces->mNumIndices; l++) {
                mesh->mColors[0][faces->mIndices[l]] = tempColors[i];
            }
        }
        // Face material splits from palette high-bits are not implemented yet.
        ++i;
        ++faces;
    }
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadFinish() {
    mScene->mRootNode = new aiNode();
    mScene->mRootNode->mName.Set("<GEORoot>");

    if (mScene->mNumMeshes) {
        mScene->mRootNode->mMeshes =
                new unsigned int[mScene->mRootNode->mNumMeshes = mScene->mNumMeshes];
        for (unsigned int i = 0; i < mScene->mNumMeshes; i++) {
            mScene->mRootNode->mMeshes[i] = i;
        }
    }

    // Assimp expects a scene-graph node with the same name as each light.
    if (mScene->mNumLights) {
        mScene->mRootNode->mChildren = new aiNode *[mScene->mNumLights];
        mScene->mRootNode->mNumChildren = mScene->mNumLights;
        for (unsigned int i = 0; i < mScene->mNumLights; ++i) {
            aiNode *n = new aiNode();
            n->mName = mScene->mLights[i]->mName;
            n->mParent = mScene->mRootNode;
            // Bake light position into the node transform (identity otherwise).
            const aiVector3D &lp = mScene->mLights[i]->mPosition;
            n->mTransformation.a4 = lp.x;
            n->mTransformation.b4 = lp.y;
            n->mTransformation.c4 = lp.z;
            mScene->mRootNode->mChildren[i] = n;
        }
    }
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadColor(unsigned int pos) {
    aiColor4D &c = tempColors[pos];

    SkipSpaces(&sz, line + sizeof(line));
    if (!(color = static_cast<long>(strtoul10(sz, &sz)))) {
        --sz;
        if (!(color = static_cast<long>(hexstrtoul10(sz, &sz)))) {
            ASSIMP_LOG_ERROR("GEO: color read failed (sz) ", sz, " color ", color);
            return;
        }
        rgbH = true;
    } else {
        rgbH = false;
    }

    LookupColor(color, c);
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::LookupColor(long iColorIndex, aiColor4D &cOut) {
    if (rgbH) {
        cOut[0] = ((iColorIndex & 0x00ff0000) >> 16) / 255.0f;
        cOut[1] = ((iColorIndex & 0x0000ff00) >> 8) / 255.0f;
        cOut[2] = (iColorIndex & 0x000000ff) / 255.0f;
        cOut[3] = 1.0f;
    } else {
        const int index = static_cast<int>(iColorIndex & 0x0f);
        cOut = *reinterpret_cast<const aiColor4D *>(&g_ColorTable[index]);

        if ((iColorIndex & 0xf0)) {
            ASSIMP_LOG_DEBUG("Achtung! (unimplemented function) Material required: ", iColorIndex,
                    " Surface effect: ", (iColorIndex & 0x30) >> 4, " Hibit:", (iColorIndex & 0xC0));
        }
    }
}

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_GEO_IMPORTER
