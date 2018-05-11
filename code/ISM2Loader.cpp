/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team



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

/** @file ISM2Loader.cpp
 *  @brief Implementation of the main parts of the ISM2 importer class
 */

#ifndef ASSIMP_BUILD_NO_ISM2_IMPORTER

#include <assimp/ByteSwapper.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/scene.h>
#include <assimp/importerdesc.h>
#include <assimp/StreamReader.h>
#include <memory>
#include "ISM2Loader.h"
#include "ISM2FileData.h"

using namespace Assimp;
using namespace Assimp::ISM2;

static const aiImporterDesc desc = {
    "Compile Heart ISM2 Importer",
    "",
    "",
    "",
    aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_LimitedSupport |
        aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    "ism2"
};

// ------------------------------------------------------------------------------------------------
// Get list of all file extensions that are handled by this loader
const aiImporterDesc* ISM2Importer::GetInfo () const
{
    return &desc;
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file.
bool ISM2Importer::CanRead(const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
    const std::string extension = GetExtension(pFile);

    if (extension == "ism2")
        return true;

    if (!extension.length() && checkSig) {
        uint32_t token = AI_ISM2_MAGIC;

        if (CheckMagicToken(pIOHandler, pFile, &token, 0, 4))
            return true;
    }
    return false;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure.
void ISM2Importer::InternReadFile( const std::string& pFile, aiScene* pScene,
    IOSystem* pIOHandler)
{
    std::unique_ptr<IOStream> io(pIOHandler->Open(pFile.c_str(), "rb"));
    Logger *log = DefaultLogger::get();

    Model model;
    ModelHeader *mh = &model.header;
    SectionData *msd = &model.sectionData;
    StringBlock *msb = &model.stringBlock;
    BoneBlock *mbb = &model.boneBlock;
    VertexBlock *mvb = &model.vertexBlock;
    MaterialBlock *mmb = &model.materialBlock;

    io->Read(mh, sizeof(ModelHeader), 1);
    io->Seek(0, aiOrigin_SET);

    bool le = mh->sectionCount > 0 && mh->sectionCount < 65535;
    StreamReaderAny sr(io.get(), le);

    sr.SetCurrentPos(sizeof(ModelHeader));
    if (!le) ByteSwap::Swap(&mh->sectionCount);

    msd->offsets = new uint32_t[mh->sectionCount];
    msd->types = new uint32_t[mh->sectionCount];
    for (uint32_t b = 0; b < mh->sectionCount; b++)
        sr >> msd->types[b] >> msd->offsets[b];

    // populate the strings array first, otherwise we'll get undefined references
    for (uint32_t b = 0; b < mh->sectionCount; b++)
    {
        if (msd->types[b] == Section_Strings)
        {
            sr.SetCurrentPos(msd->offsets[b]);
            sr.CopyAndAdvance(&msb->header, sizeof(StringHeader));
            if (!le) ByteSwap::Swap(&msb->header.total);
            msb->offsets = new uint32_t[msb->header.total];
            msb->strings = new std::string[msb->header.total + 1];

            for (uint32_t c = 0; c < msb->header.total; c++)
                sr >> msb->offsets[c];

            for (uint32_t c = 0; c < msb->header.total; c++)
            {
                char txt = 0;

                sr.SetCurrentPos(msb->offsets[c]);
                sr >> txt;
                while (txt != 0) {
                    msb->strings[c] += txt;
                    sr >> txt;
                }
            }

            // for materials with no submats
            msb->strings[msb->header.total] = std::string("Tex_c.dds");

            break;
        }
    }

    for (uint32_t b = 0; b < mh->sectionCount; b++)
    {
        sr.SetCurrentPos(msd->offsets[b]);

        switch (msd->types[b])
        {
            case Section_Bones:
                sr.CopyAndAdvance(&mbb->header, sizeof(BoneDataHeader));
                if (!le) ByteSwap::Swap(&mbb->header.total);
                mbb->offsets = new uint32_t[mbb->header.total];
                mbb->bones = new Bone[mbb->header.total];

                for (uint32_t c = 0; c < mbb->header.total; c++)
                    sr >> mbb->offsets[c];

                for (uint32_t c = 0; c < mbb->header.total; c++)
                {
                    Bone *bone = &mbb->bones[c];

                    sr.SetCurrentPos(mbb->offsets[c]);
                    sr.CopyAndAdvance(&bone->header, sizeof(BoneHeader));
                    if (!le) {
                        ByteSwap::Swap(&bone->header.nameStringIndex[0]);
                        ByteSwap::Swap(&bone->header.id);
                    }
                    bone->sectionOffsets = new uint32_t[bone->header.headerTotal];
                    bone->sections = new BoneSection[bone->header.headerTotal];

                    for (uint32_t d = 0; d < bone->header.headerTotal; d++)
                        sr >> bone->sectionOffsets[d];

                    for (uint32_t d = 0; d < bone->header.headerTotal; d++)
                    {
                        BoneSection *bs = &bone->sections[d];

                        sr.SetCurrentPos(bone->sectionOffsets[d]);
                        sr >> bs->type;

                        switch (bs->type) {
                            case Section_SurfaceOffsets:
                                sr.CopyAndAdvance(&bs->surfaceOffsetsHeader, sizeof(SurfaceOffsetsHeader));
                                if (!le) ByteSwap::Swap(&bs->surfaceOffsetsHeader.total);
                                bs->surfaceOffsets = new uint32_t[bs->surfaceOffsetsHeader.total];
                                bs->surfaces = new SurfaceHeader[bs->surfaceOffsetsHeader.total];

                                for (uint32_t e = 0; e < bs->surfaceOffsetsHeader.total; e++)
                                    sr >> bs->surfaceOffsets[e];

                                for (uint32_t e = 0; e < bs->surfaceOffsetsHeader.total; e++)
                                {
                                    sr.SetCurrentPos(bs->surfaceOffsets[e]);
                                    sr.CopyAndAdvance(&bs->surfaces[e], sizeof(SurfaceHeader));
                                    if (!le) ByteSwap::Swap(&bs->surfaces[e].textureNameStringIndex);
                                }

                                break;
                            case Section_BoneTransforms:
                                sr.CopyAndAdvance(&bs->transformHeader, sizeof(TransformHeader));
                                if (!le) ByteSwap::Swap(&bs->transformHeader.total);
                                bs->transformOffsets = new uint32_t[bs->transformHeader.total];
                                bs->transformSections = new TransformSection[bs->transformHeader.total];

                                for (uint32_t e = 0; e < bs->transformHeader.total; e++)
                                    sr >> bs->transformOffsets[e];

                                for (uint32_t t = 0; t < bs->transformHeader.total; t++)
                                {
                                    TransformSectionData *tsd = &bs->transformSections[t].data;

                                    sr.SetCurrentPos(bs->transformOffsets[t]);
                                    sr >> bs->transformSections[t].type;

                                    switch (bs->transformSections[t].type)
                                    {
                                        case Section_BoneTranslation:
                                            sr >> tsd->translation[0] >> tsd->translation[1] >> tsd->translation[2];
                                            break;
                                        case Section_BoneScale:
                                            sr >> tsd->scale[0] >> tsd->scale[1]>> tsd->scale[2];
                                            break;
                                        case Section_BoneX:
                                            sr >> tsd->x[0] >> tsd->x[1] >> tsd->x[2] >> tsd->x[3];
                                            break;
                                        case Section_BoneY:
                                            sr >> tsd->y[0] >> tsd->y[1] >> tsd->y[2] >> tsd->y[3];
                                            break;
                                        case Section_BoneZ:
                                            sr >> tsd->z[0] >> tsd->z[1] >> tsd->z[2] >> tsd->z[3];
                                            break;
                                        case Section_BoneRotationX:
                                            sr >> tsd->xRotate[0] >> tsd->xRotate[1] >> tsd->xRotate[2] >> tsd->xRotate[3];
                                            break;
                                        case Section_BoneRotationY:
                                            sr >> tsd->yRotate[0] >> tsd->yRotate[1] >> tsd->yRotate[2] >> tsd->yRotate[3];
                                            break;
                                        case Section_BoneRotationZ:
                                            sr >> tsd->zRotate[0] >> tsd->zRotate[1] >> tsd->zRotate[2] >> tsd->zRotate[3];
                                            break;
                                        default:
                                            log->warn(std::string("Unsupported/unknown bone transform section: ") +
                                                std::to_string(bs->transformSections[t].type));
                                    }
                                }
                                break;
                            default:
                                log->warn(std::string("Unsupported/unknown bone section: ") +
                                    std::to_string(bs->type));
                        }
                    }
                }

                break;

            case Section_VertexBlockHeader:
                sr.CopyAndAdvance(&mvb->header, sizeof(VertexBlockHeader));
                if (!le) ByteSwap::Swap(&mvb->header.headerTotal);
                mvb->offsets = new uint32_t[mvb->header.headerTotal];
                mvb->sections = new VertexBlockSection[mvb->header.headerTotal];

                for (uint32_t c = 0; c < mvb->header.headerTotal; c++)
                    sr >> mvb->offsets[c];

                for (uint32_t c = 0; c < mvb->header.headerTotal; c++)
                {
                    VertexBlockSection *vbs = &mvb->sections[c];

                    sr.SetCurrentPos(mvb->offsets[c]);
                    sr >> vbs->type;

                    switch (vbs->type)
                    {
                        case Section_VertexMetaHeader:
                            sr.CopyAndAdvance(&vbs->header, sizeof(VertexMetaHeader));
                            if (!le) ByteSwap::Swap(&vbs->header.headerTotal);
                            vbs->offsets = new uint32_t[vbs->header.headerTotal];
                            vbs->sections = new VertexHeaderSection[vbs->header.headerTotal];

                            for (uint32_t d = 0; d < vbs->header.headerTotal; d++)
                                sr >> vbs->offsets[d];

                            for (uint32_t d = 0; d < vbs->header.headerTotal; d++)
                            {
                                VertexHeaderSection *vhs = &vbs->sections[d];
                                VertexData *vd = &vhs->data;

                                sr.SetCurrentPos(vbs->offsets[d]);
                                sr >> vhs->type;

                                switch (vhs->type)
                                {
                                    case Section_Polygon:
                                        sr.CopyAndAdvance(&vhs->polygonBlock.header, sizeof(PolygonBlockHeader));
                                        if (!le) {
                                            ByteSwap::Swap(&vhs->polygonBlock.header.dataTotal);
                                            ByteSwap::Swap(&vhs->polygonBlock.header.nameStringIndex);
                                        }
                                        vhs->polygonBlock.offsets = new uint32_t[vhs->polygonBlock.header.dataTotal];
                                        vhs->polygonBlock.polygons = new Polygon[vhs->polygonBlock.header.dataTotal];

                                        for (uint32_t e = 0; e < vhs->polygonBlock.header.dataTotal; e++)
                                            sr >> vhs->polygonBlock.offsets[e];

                                        for (uint32_t e = 0; e < vhs->polygonBlock.header.dataTotal; e++)
                                        {
                                            Polygon *p = &vhs->polygonBlock.polygons[e];

                                            sr.SetCurrentPos(vhs->polygonBlock.offsets[e]);
                                            sr >> p->type;

                                            switch (p->type)
                                            {
                                                case Section_PolygonBlock:
                                                    sr.CopyAndAdvance(&p->header, sizeof(PolygonHeader));
                                                    if (!le) {
                                                        ByteSwap::Swap(&p->header.total);
                                                        ByteSwap::Swap(&p->header.type[0]);
                                                    }
                                                    p->faces = new uint32_t[p->header.total / 3][3];

                                                    switch (p->header.type[0])
                                                    {
                                                        case 5:
                                                            for (uint32_t g = 0; g < p->header.total / 3; g++) {
                                                                p->faces[g][0] = sr.GetU2();
                                                                p->faces[g][1] = sr.GetU2();
                                                                p->faces[g][2] = sr.GetU2();
                                                            }
                                                            break;
                                                        case 7:
                                                            for (uint32_t g = 0; g < p->header.total / 3; g++)
                                                                sr >> p->faces[g][0] >> p->faces[g][1] >> p->faces[g][2];
                                                            break;
                                                        default:
                                                            log->warn(std::string("Unsupported/unknown polygon type: ") +
                                                                std::to_string(p->header.type[0]));
                                                    }

                                                    break;
                                                default:
                                                    log->warn(std::string("Unsupported/unknown polygon data section: ") +
                                                        std::to_string(p->type));
                                            }
                                        }

                                        break;
                                    case Section_VertexBlock:
                                        sr.CopyAndAdvance(&vd->header, sizeof(VertexHeader));
                                        if (!le) {
                                            ByteSwap::Swap(&vd->header.total);
                                            ByteSwap::Swap(&vd->header.count);
                                            ByteSwap::Swap(&vd->header.type[0]);
                                        }
                                        vd->offsets = new uint32_t[vd->header.total];
                                        vd->offsetHeaders = new VertexOffsetHeader[vd->header.total];
                                        vd->vertices = new Vertex[vd->header.count];

                                        for (uint32_t e = 0; e < vd->header.total; e++)
                                            sr >> vd->offsets[e];

                                        for (uint32_t e = 0; e < vd->header.total; e++)
                                        {
                                            sr.SetCurrentPos(vd->offsets[e]);
                                            sr.CopyAndAdvance(&vd->offsetHeaders[e], sizeof(VertexOffsetHeader));
                                            if (!le) ByteSwap::Swap(&vd->offsetHeaders[e].startOffset);
                                            sr.SetCurrentPos(vd->offsetHeaders[e].startOffset);
                                        }

                                        for (uint32_t e = 0; e < vd->header.count; e++)
                                        {
                                            switch (vd->header.type[0])
                                            {
                                                case 1:
                                                {
                                                    Vertex1 v;

                                                    sr.CopyAndAdvance(&v, sizeof(Vertex1));
                                                    if (!le) {
                                                        ByteSwap::Swap(&v.position[0]);
                                                        ByteSwap::Swap(&v.position[1]);
                                                        ByteSwap::Swap(&v.position[2]);
                                                        ByteSwap::Swap(&v.normal1[0]);
                                                        ByteSwap::Swap(&v.normal1[1]);
                                                        ByteSwap::Swap(&v.normal1[2]);
                                                        ByteSwap::Swap(&v.textureCoordX);
                                                        ByteSwap::Swap(&v.textureCoordY);
                                                    }
                                                    vd->vertices[e].type1 = v;
                                                    break;
                                                }
                                                case 3:
                                                {
                                                    switch (vd->header.size)
                                                    {
                                                        case 16:
                                                        {
                                                            Vertex3Size16 v;

                                                            for (uint32_t g = 0; g < 4; g++)
                                                                sr >> v.bones[g];
                                                            for (uint32_t g = 0; g < 4; g++)
                                                                sr >> v.weights[g];

                                                            vd->vertices[e].type3Size16 = v;
                                                            break;
                                                        }
                                                        case 32:
                                                            if (mh->version[0] == 1) {
                                                                Vertex3Size32V1 v;

                                                                for (uint32_t g = 0; g < 4; g++)
                                                                    sr >> v.bones[g];
                                                                for (uint32_t g = 0; g < 4; g++)
                                                                    sr >> v.weights[g];

                                                                vd->vertices[e].type3Size32V1 = v;
                                                            } else if (mh->version[0] == 2) {
                                                                Vertex3Size32V2 v;

                                                                for (uint32_t g = 0; g < 4; g++)
                                                                    sr >> v.bones[g];
                                                                for (uint32_t g = 0; g < 4; g++)
                                                                    sr >> v.weights[g];

                                                                vd->vertices[e].type3Size32V2 = v;
                                                            } else {
                                                                log->warn(std::string("Unsupported/unknown vertex structure version: ") +
                                                                    std::to_string(static_cast<unsigned>(mh->version[0])));
                                                            }
                                                            break;
                                                        case 48:
                                                        {
                                                            Vertex3Size48 v;

                                                            for (uint32_t g = 0; g < 8; g++)
                                                                sr >> v.bones[g];
                                                            for (uint32_t g = 0; g < 8; g++)
                                                                sr >> v.weights[g];

                                                            vd->vertices[e].type3Size48 = v;
                                                            break;
                                                        }
                                                        default:
                                                            log->warn(std::string("Unsupported/unknown vertex size: ") +
                                                                std::to_string(vd->header.size));
                                                    }
                                                }
                                                default:
                                                    log->warn(std::string("Unsupported/unknown vertex type: ") +
                                                        std::to_string(vd->header.type[0]));
                                            }
                                        }
                                        break;
                                    default:
                                        log->warn(std::string("Unsupported/unknown vertex header section: ") +
                                            std::to_string(vhs->type));
                                }
                            }
                            break;
                        default:
                            log->warn(std::string("Unsupported/unknown vertex data section: ") +
                                std::to_string(vbs->type));
                    }
                }
                break;

            case Section_Strings: // already done
                break;

            // Seems redundant as material data also points to tex name, which is all we'd
            // grab from here for now, pending any more info on Texture struct being found
            case Section_Textures:
                /*TextureHeader th;

                io->Read(&th, sizeof(TextureHeader), 1);
                if (!le) ByteSwap::Swap(&th.total);
                uint32_t *textureOffsets = new uint32_t[th.total];

                for (uint32_t c = 0; c < th.total; c++)
                    textureOffsets[c] = sr->GetU4();

                for (uint32_t c = 0; c < th.total; c++)
                {
                    Texture tex;

                    io->Seek(textureOffsets[c], aiOrigin_SET);
                    io->Read(&tex, sizeof(Texture), 1);
                    if (!le) ByteSwap::Swap(&tex.nameStringIndex);
                    textures.push_back(tex);
                }

                delete[] textureOffsets;*/
                break;
            case Section_Materials:
                sr.CopyAndAdvance(&mmb->header, sizeof(MaterialHeader));
                if (!le) ByteSwap::Swap(&mmb->header.total);
                mmb->offsets = new uint32_t[mmb->header.total];
                mmb->materials = new Material[mmb->header.total];

                for (uint32_t z = 0; z < mmb->header.total; z++)
                    sr >> mmb->offsets[z];

                for (uint32_t z = 0; z < mmb->header.total; z++)
                {
                    Material *m = &mmb->materials[z];

                    sr.SetCurrentPos(mmb->offsets[z]);
                    sr.CopyAndAdvance(&m->a, sizeof(MaterialA));
                    if (!le) {
                        ByteSwap::Swap(&m->a.nameStringIndex);
                        ByteSwap::Swap(&m->a.total);
                    }

                    if (m->a.total > 0)
                    {
                        sr >> m->bOffset;
                        sr.SetCurrentPos(m->bOffset);
                        sr.CopyAndAdvance(&m->b, sizeof(MaterialB));
                        if (!le) ByteSwap::Swap(&m->b.cOffset);

                        sr.SetCurrentPos(m->b.cOffset);
                        sr.CopyAndAdvance(&m->c, sizeof(MaterialC));
                        if (!le) ByteSwap::Swap(&m->c.dOffset);

                        sr.SetCurrentPos(m->c.dOffset);
                        sr.CopyAndAdvance(&m->d, sizeof(MaterialD));
                        if (!le) ByteSwap::Swap(&m->d.eOffset);

                        sr.SetCurrentPos(m->d.eOffset);
                        sr.CopyAndAdvance(&m->e, sizeof(MaterialE));
                        if (!le) ByteSwap::Swap(&m->e.fOffset);

                        sr.SetCurrentPos(m->e.fOffset);
                        sr >> m->textureNameStringIndex;
                    }
                }
                break;
            default:
                log->warn(std::string("Unsupported/unknown section: ") +
                    std::to_string(msd->types[b]));
        }
    }
}

#endif // ASSIMP_BUILD_NO_ISM2_IMPORTER
