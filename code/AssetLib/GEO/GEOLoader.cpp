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
#include <assimp/TinyFormatter.h>
#include <assimp/fast_atof.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>

#include <cstdio>
#include <memory>

namespace Assimp {

// *INDENT-OFF*
static constexpr aiImporterDesc desc = {
        "Videoscape GEO Importer",
        "ZsoltTech.Com <arris@zsolttech.com>",
        "",
        "http://paulbourke.net/dataformats/geo/ "
        "color settings from: https://home.comcast.net/~erniew/getstuff/geo.html "
        "calculation http://home.comcast.net/~erniew/lwsdk/sample/vidscape/surf.c",
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
        pScene(nullptr),
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
void GEOImporter::InternReadFile(const std::string &pFile, aiScene *pScene,
        IOSystem *pIOHandler) {
    std::unique_ptr<IOStream> file(pIOHandler->Open(pFile, "rb"));

    if (file == nullptr) {
        throw DeadlyImportError("Failed to open GEO file ", pFile, ".");
    }

    this->pScene = pScene;

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
    const unsigned int numFaces = 32365 * 3; // TODO: find a dynamic way?

    pScene->mMeshes = new aiMesh *[pScene->mNumMeshes = 1];
    mesh = pScene->mMeshes[0] = new aiMesh();
    faces = mesh->mFaces = new aiFace[numFaces];
    tempPositions.resize(numElementsToImport);

    if (flav == Mesh_with_coloured_faces) {
        InternReadncV(numElementsToImport);
    } else if (flav == Lamp) {
        InternReadLamp(numElementsToImport);
    } else if (flav == Gouraud_curves_or_NURBS_surfaces) {
        InternReadFbS(numElementsToImport); // here numElementsToImport is the surface type !!!
    } else if (flav == Mesh_with_coloured_vertices) {
        InternReadcV(numElementsToImport);
    } else {
        throw DeadlyImportError("GEO: Never see me, bug in assimp's api design.");
    }

    m_progress->UpdateFileRead(2, 5);

    const char *old = buffer;

    if (flav == Mesh_with_coloured_faces || flav == Mesh_with_coloured_vertices) {
        // First find out how many vertices we'll need
        while (GetNextLine(buffer, line)) {
            sz = line;
            faces->mNumIndices = strtoul10(sz, &sz);
            if (!faces->mNumIndices) {
                ASSIMP_LOG_ERROR("GEO: Faces with zero indices aren't allowed");
                continue;
            }
            mesh->mNumFaces++; // TODO: implement material stuff needs new mesh
            mesh->mNumVertices += faces->mNumIndices;
            faces++;
        }

        if (!mesh->mNumVertices) {
            throw DeadlyImportError("GEO: There are no valid faces");
        }

        ASSIMP_LOG_DEBUG("GEO: face storage just needs ", mesh->mNumFaces, " faces, not ", numFaces);
    }

    // allocate storage for the output vertices
    verts = mesh->mVertices = new aiVector3D[mesh->mNumVertices];

    // second: now parse all face indices
    buffer = old;
    faces = mesh->mFaces;

    m_progress->UpdateFileRead(3, 5);

    if (flav != Lamp && flav != Gouraud_curves_or_NURBS_surfaces) {
        if (flav == Mesh_with_coloured_faces) {
            InternReadcF(mesh->mNumFaces); // mesh colored faces
        } else {
            InternReadncF(mesh->mNumFaces); // mesh colored vertices
        }
    }

    m_progress->UpdateFileRead(4, 5);

    InternReadFinish();
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadLamp(unsigned int count) {
    ASSIMP_LOG_DEBUG("GEO: Has to import ", count, " light(s)");

    pScene->mLights = new aiLight *[count];

    while (GetNextLine(buffer, line)) {
        sz = line;
        unsigned int type = strtoul10(sz, &sz);
        char name[16];
        std::snprintf(name, sizeof(name), "Lamp%04d%04X", pScene->mNumLights + 1, type);
        aiString tmpMatName;
        tmpMatName.Set(name);
        aiLight *tmpLight = new aiLight();
        tmpLight->mName = tmpMatName;

        ASSIMP_LOG_DEBUG("GEO: Create light: ", tmpMatName.C_Str());

        // type - lamp type (0 - point lamp, 1 - spot lamp, 2 - sun)
        tmpLight->mType = static_cast<aiLightSourceType>(type);

        GetNextLine(buffer, line);
        sz = line;
        float &ic = tmpLight->mAngleInnerCone;
        float &oc = tmpLight->mAngleOuterCone;
        sz = fast_atoreal_move<float>(sz, (float &)ic);
        SkipSpaces(&sz, line + sizeof(line));
        fast_atoreal_move<float>(sz, (float &)oc);

        GetNextLine(buffer, line);
        sz = line;
        aiColor3D &c = tmpLight->mColorDiffuse;
        sz = fast_atoreal_move<float>(sz, (float &)c.r);
        SkipSpaces(&sz, line + sizeof(line));
        sz = fast_atoreal_move<float>(sz, (float &)c.g);
        SkipSpaces(&sz, line + sizeof(line));
        fast_atoreal_move<float>(sz, (float &)c.b);

        GetNextLine(buffer, line);
        sz = line;
        aiVector3D &p = tmpLight->mPosition;
        sz = fast_atoreal_move<float>(sz, (float &)p.x);
        SkipSpaces(&sz, line + sizeof(line));
        sz = fast_atoreal_move<float>(sz, (float &)p.y);
        SkipSpaces(&sz, line + sizeof(line));
        fast_atoreal_move<float>(sz, (float &)p.z);

        GetNextLine(buffer, line);
        sz = line;
        aiVector3D &d = tmpLight->mDirection;
        sz = fast_atoreal_move<float>(sz, (float &)d.x);
        SkipSpaces(&sz, line + sizeof(line));
        sz = fast_atoreal_move<float>(sz, (float &)d.y);
        SkipSpaces(&sz, line + sizeof(line));
        fast_atoreal_move<float>(sz, (float &)d.z);

        pScene->mLights[pScene->mNumLights] = tmpLight;
        pScene->mNumLights++;
    }
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadFbS(unsigned int count) {
    ASSIMP_LOG_DEBUG("GEO: Has to import type ", count, " form(s)");
    while (GetNextLine(buffer, line)) {
        sz = line;
    }
    throw DeadlyImportError("GEO: Curves and surfaces not supported yet.");
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadcV(unsigned int count) {
    ASSIMP_LOG_DEBUG("GEO: Has to import ", count, " colored vertex/vertices");

    tempColors.resize(count);

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

        InternReadColor(i);
    }
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadncV(unsigned int count) {
    ASSIMP_LOG_DEBUG("GEO: Has to import ", count, " not colored vertex/vertices");

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
        fast_atoreal_move<float>(sz, (float &)v.z);
    }
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadcF(unsigned int count) {
    ASSIMP_LOG_DEBUG("GEO: Has to import ", count, " colored face(s)");

    tempColors.resize(count);

    for (unsigned int i = 0, p = 0; i < count;) {
        if (!GetNextLine(buffer, line)) {
            return;
        }

        sz = line;

        unsigned int idx, pos;
        if (!(idx = strtoul10(sz, &sz))) {
            continue;
        }

        faces->mIndices = new unsigned int[faces->mNumIndices = idx];
        if (!mesh->mColors[0]) {
            mesh->mColors[0] = new aiColor4D[mesh->mNumVertices];
            ASSIMP_LOG_DEBUG("GEO: got new mesh");
        }
        for (unsigned int m = 0; m < faces->mNumIndices; m++) {
            SkipSpaces(&sz, line + sizeof(line));
            pos = strtoul10(sz, &sz);
            faces->mIndices[m] = p++;
            *verts++ = tempPositions[pos];
        }

        InternReadColor(i);

        for (unsigned int l = 0; l < faces->mNumIndices; l++) {
            aiColor4D &col = mesh->mColors[0][faces->mIndices[l]];
            col = tempColors[i];
        }
        i++;
        faces++;
        // TODO: face mesh material handling
    }
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadncF(unsigned int count) {
    ASSIMP_LOG_DEBUG("GEO: Has to import ", count, " not colored face(s)");
    (void)count;

    for (unsigned int i = 0, p = 0; i < mesh->mNumFaces;) {
        if (!GetNextLine(buffer, line)) {
            break;
        }

        sz = line;

        unsigned int idx, pos;
        if (!(idx = strtoul10(sz, &sz))) {
            continue;
        }

        faces->mIndices = new unsigned int[faces->mNumIndices = idx];
        if (!mesh->mColors[0]) {
            mesh->mColors[0] = new aiColor4D[mesh->mNumVertices];
            ASSIMP_LOG_DEBUG("GEO: got new mesh");
        }
        for (unsigned int m = 0; m < faces->mNumIndices; m++) {
            SkipSpaces(&sz, line + sizeof(line));
            pos = strtoul10(sz, &sz);
            faces->mIndices[m] = p++;
            *verts++ = tempPositions[pos];
            aiColor4D &col = mesh->mColors[0][faces->mIndices[m]];
            col = tempColors[pos];
        }
        i++;
        faces++;
    }
}

// ------------------------------------------------------------------------------------------------
void GEOImporter::InternReadFinish() {
    pScene->mRootNode = new aiNode();
    pScene->mRootNode->mName.Set("<GEORoot>");

    pScene->mRootNode->mMeshes =
            new unsigned int[pScene->mRootNode->mNumMeshes = pScene->mNumMeshes];

    for (unsigned int i = 0; i < pScene->mNumMeshes; i++) {
        pScene->mRootNode->mMeshes[i] = i;
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
