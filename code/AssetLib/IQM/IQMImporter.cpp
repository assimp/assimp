/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

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

----------------------------------------------------------------------
*/

#ifndef ASSIMP_BUILD_NO_IQM_IMPORTER

#include <assimp/DefaultIOSystem.h>
#include <assimp/IOStreamBuffer.h>
#include <assimp/ai_assert.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/ByteSwapper.h>
#include <memory>
#include <numeric>

#include "IQMImporter.h"
#include "iqm.h"

// RESOURCES:
// http://sauerbraten.org/iqm/
// https://github.com/lsalzman/iqm

inline void swap_block( uint32_t *block, size_t size ){
    (void)block; // suppress 'unreferenced formal parameter' MSVC warning
    size >>= 2;
    for ( size_t i = 0; i < size; ++i )
        AI_SWAP4( block[ i ] );
}

static constexpr aiImporterDesc desc = {
    "Inter-Quake Model Importer",
    "",
    "",
    "",
    aiImporterFlags_SupportBinaryFlavour,
    0,
    0,
    0,
    0,
    "iqm"
};

namespace Assimp {

// ------------------------------------------------------------------------------------------------
//  Default constructor
IQMImporter::IQMImporter() :
        mScene(nullptr) {
    // empty
}

// ------------------------------------------------------------------------------------------------
//  Returns true, if file is a binary Inter-Quake Model file.
bool IQMImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool checkSig) const {
    const std::string extension = GetExtension(pFile);

    if (extension == "iqm") {
        return true;
    }
        
    if (!extension.length() || checkSig) {
        if (!pIOHandler) {
            return true;
        }
        std::unique_ptr<IOStream> pStream(pIOHandler->Open(pFile, "rb"));
        unsigned char data[15];
        if (!pStream || 15 != pStream->Read(data, 1, 15)) {
            return false;
        }
        return !memcmp(data, "INTERQUAKEMODEL", 15);
    }
    
    return false;
}

// ------------------------------------------------------------------------------------------------
const aiImporterDesc *IQMImporter::GetInfo() const {
    return &desc;
}

// ------------------------------------------------------------------------------------------------
//  Model 3D import implementation
void IQMImporter::InternReadFile(const std::string &file, aiScene *pScene, IOSystem *pIOHandler) {
    // Read file into memory
    std::unique_ptr<IOStream> pStream(pIOHandler->Open(file, "rb"));
    if (!pStream) {
        throw DeadlyImportError("Failed to open file ", file, ".");
    }

    // Get the file-size and validate it, throwing an exception when fails
    const size_t fileSize = pStream->FileSize();
    if (fileSize < sizeof( iqmheader )) {
        throw DeadlyImportError("IQM-file ", file, " is too small.");
    }
    std::vector<unsigned char> buffer(fileSize);
    unsigned char *data = buffer.data();
    ai_assert(data != nullptr);
    if (fileSize != pStream->Read(data, 1, fileSize)) {
        throw DeadlyImportError("Failed to read the file ", file, ".");
    }

    // Get the header
    iqmheader &hdr = reinterpret_cast<iqmheader&>( *data );
    swap_block( &hdr.version, sizeof( iqmheader ) - sizeof( iqmheader::magic ) );

    // Extra check for header
    if (memcmp(data, IQM_MAGIC, sizeof( IQM_MAGIC ) )
     || hdr.version != IQM_VERSION
     || hdr.filesize != fileSize) {
        throw DeadlyImportError("Bad binary header in file ", file, ".");
    }

    ASSIMP_LOG_DEBUG("IQM: loading ", file);

    // Every offset/count below is taken straight from the (attacker-controlled)
    // header, so make sure each table and the text blob lie fully inside the file
    // before any of them is dereferenced. Arithmetic is done in 64-bit to avoid
    // wrapping a 32-bit product.
    const auto rangeInFile = [fileSize](uint64_t offset, uint64_t bytes) {
        // Written so neither side of the comparison can wrap, even if a caller
        // passes a byte count that saturated on overflow.
        return bytes <= fileSize && offset <= fileSize - bytes;
    };
    if (!rangeInFile(hdr.ofs_meshes, (uint64_t)hdr.num_meshes * sizeof(iqmmesh))
             || !rangeInFile(hdr.ofs_vertexarrays, (uint64_t)hdr.num_vertexarrays * sizeof(iqmvertexarray))
             || !rangeInFile(hdr.ofs_triangles, (uint64_t)hdr.num_triangles * sizeof(iqmtriangle))
             || !rangeInFile(hdr.ofs_text, hdr.num_text)) {
        throw DeadlyImportError("Corrupt header offsets in IQM file ", file, ".");
    }
    // A material name is read with strlen from the text blob, so it must be
    // NULL-terminated inside that blob.
    if (hdr.num_text && data[(size_t)hdr.ofs_text + hdr.num_text - 1] != 0) {
        throw DeadlyImportError("Unterminated text section in IQM file ", file, ".");
    }

    // create the root node
    pScene->mRootNode = new aiNode( "<IQMRoot>" );
    // Now rotate the whole scene 90 degrees around the x axis to convert to internal coordinate system
    pScene->mRootNode->mTransformation = aiMatrix4x4(
            1.f, 0.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, -1.f, 0.f, 0.f,
            0.f, 0.f, 0.f, 1.f);
    pScene->mRootNode->mNumMeshes = hdr.num_meshes;
    pScene->mRootNode->mMeshes = new unsigned int[hdr.num_meshes];
    std::iota( pScene->mRootNode->mMeshes, pScene->mRootNode->mMeshes + pScene->mRootNode->mNumMeshes, 0 );

    mScene = pScene;

    // Allocate output storage
    pScene->mNumMeshes = 0;
    pScene->mMeshes = new aiMesh *[hdr.num_meshes](); // Set arrays to zero to ensue proper destruction if an exception is raised

    pScene->mNumMaterials = 0;
    pScene->mMaterials = new aiMaterial *[hdr.num_meshes]();

    // Swap vertex arrays beforehand...
    for( auto array = reinterpret_cast<iqmvertexarray*>( data + hdr.ofs_vertexarrays ), end = array + hdr.num_vertexarrays; array != end; ++array ) {
        swap_block( &array->type, sizeof( iqmvertexarray ) );
    }

    // Read all surfaces from the file
    for( auto imesh = reinterpret_cast<iqmmesh*>( data + hdr.ofs_meshes ), end_ = imesh + hdr.num_meshes; imesh != end_; ++imesh ) {
        swap_block( &imesh->name, sizeof( iqmmesh ) );
        // The mesh references sub-ranges of the shared triangle/text tables;
        // reject anything that reaches past their validated bounds.
        if (imesh->material >= hdr.num_text || (uint64_t)imesh->first_triangle + imesh->num_triangles > hdr.num_triangles) {
            throw DeadlyImportError("Corrupt mesh entry in IQM file ", file, ".");
        }
        // Allocate output mesh & material
        auto mesh = pScene->mMeshes[pScene->mNumMeshes++] = new aiMesh();
        mesh->mMaterialIndex = pScene->mNumMaterials;
        auto mat = pScene->mMaterials[pScene->mNumMaterials++] = new aiMaterial();
        {
            auto text = reinterpret_cast<char*>( data + hdr.ofs_text );
            aiString name( text + imesh->material );
            mat->AddProperty( &name, AI_MATKEY_NAME );
            mat->AddProperty( &name, AI_MATKEY_TEXTURE_DIFFUSE(0) );
        }

        // Fill mesh information
        mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
        mesh->mNumFaces = 0;
        mesh->mFaces = new aiFace[imesh->num_triangles];

        // Fill in all triangles
        for( auto tri = reinterpret_cast<iqmtriangle*>( data + hdr.ofs_triangles ) + imesh->first_triangle, end = tri + imesh->num_triangles; tri != end; ++tri ) {
            swap_block( tri->vertex, sizeof( tri->vertex ) );
            // Indices are rebased onto this mesh by subtracting first_vertex, so a
            // triangle pointing outside [first_vertex, first_vertex + num_vertexes)
            // would produce an out-of-range vertex index for later stages.
            for (unsigned int v : tri->vertex) {
                if (v < imesh->first_vertex || v - imesh->first_vertex >= imesh->num_vertexes) {
                    throw DeadlyImportError("Corrupt triangle in IQM file ", file, ".");
                }
            }
            auto& face = mesh->mFaces[mesh->mNumFaces++];
            face.mNumIndices = 3;
            face.mIndices = new unsigned int[3]{ tri->vertex[0] - imesh->first_vertex,
                                                 tri->vertex[2] - imesh->first_vertex,
                                                 tri->vertex[1] - imesh->first_vertex };
        }

        // Fill in all vertices
        for( auto array = reinterpret_cast<const iqmvertexarray*>( data + hdr.ofs_vertexarrays ), end__ = array + hdr.num_vertexarrays; array != end__; ++array ) {
            const unsigned int nVerts = imesh->num_vertexes;
            const unsigned int step = array->size;

            // Bound the source region [offset, offset + (first_vertex + nVerts)*step*elemSize)
            // against the file before any element of this array is read.
            static const unsigned int kFormatSize[] = { 1, 1, 2, 2, 4, 4, 2, 4, 8 };
            const unsigned int elemSize = array->format < (sizeof(kFormatSize) / sizeof(*kFormatSize))
                    ? kFormatSize[array->format] : 0;
            // Every factor is a 32-bit header field, so the product can exceed
            // 64 bits; saturate on overflow so the bound check rejects it.
            const uint64_t count = (uint64_t)imesh->first_vertex + nVerts;
            const uint64_t stride = (uint64_t)step * elemSize;
            const uint64_t maxU64 = ~(uint64_t)0;
            const uint64_t region = (count != 0 && stride > maxU64 / count) ? maxU64 : count * stride;
            if (elemSize && !rangeInFile(array->offset, region)) {
                throw DeadlyImportError("Corrupt vertex array in IQM file ", file, ".");
            }

            switch ( array->type ) {
            case IQM_POSITION:
                if( array->format == IQM_FLOAT && step >= 3 ){
                    mesh->mNumVertices = nVerts;
                    auto v = mesh->mVertices = new aiVector3D[nVerts];
                    for( auto f = reinterpret_cast<const float*>( data + array->offset ) + imesh->first_vertex * step,
                            end = f + nVerts * step; f != end; f += step, ++v ) {
                        *v = { AI_BE( f[0] ),
                               AI_BE( f[1] ),
                               AI_BE( f[2] ) };
                    }
                }
                break;
            case IQM_TEXCOORD:
                if( array->format == IQM_FLOAT && step >= 2) {
                    auto v = mesh->mTextureCoords[0] = new aiVector3D[nVerts];
                    mesh->mNumUVComponents[0] = 2;
                    for( auto f = reinterpret_cast<const float*>( data + array->offset ) + imesh->first_vertex * step,
                            end = f + nVerts * step; f != end; f += step, ++v ) {
                        *v = { AI_BE( f[0] ),
                           1 - AI_BE( f[1] ), 0 };
                    }
                }
                break;
            case IQM_NORMAL:
                if (array->format == IQM_FLOAT && step >= 3) {
                    auto v = mesh->mNormals = new aiVector3D[nVerts];
                    for( auto f = reinterpret_cast<const float*>( data + array->offset ) + imesh->first_vertex * step,
                            end = f + nVerts * step; f != end; f += step, ++v ) {
                        *v = { AI_BE( f[0] ),
                               AI_BE( f[1] ),
                               AI_BE( f[2] ) };
                    }
                }
                break;
            case IQM_COLOR:
                if (array->format == IQM_UBYTE && step >= 3) {
                    auto v = mesh->mColors[0] = new aiColor4D[nVerts];
                    for( auto f = ( data + array->offset ) + imesh->first_vertex * step,
                            end = f + nVerts * step; f != end; f += step, ++v ) {
                        *v = { ( f[0] ) / 255.f,
                               ( f[1] ) / 255.f,
                               ( f[2] ) / 255.f,
                               step == 3? 1 : ( f[3] ) / 255.f };
                    }
                } else if (array->format == IQM_FLOAT && step >= 3) {
                    auto v = mesh->mColors[0] = new aiColor4D[nVerts];
                    for( auto f = reinterpret_cast<const float*>( data + array->offset ) + imesh->first_vertex * step,
                            end = f + nVerts * step; f != end; f += step, ++v ) {
                        *v = { AI_BE( f[0] ),
                               AI_BE( f[1] ),
                               AI_BE( f[2] ),
                               step == 3? 1 : AI_BE( f[3] ) };
                    }
                }
                break;
            case IQM_TANGENT:
#if 0
                if (array->format == IQM_FLOAT && step >= 3) {
                    auto v = mesh->mTangents = new aiVector3D[nVerts];
                    for( auto f = reinterpret_cast<const float*>( data + array->offset ) + imesh->first_vertex * step,
                            end = f + nVerts * step; f != end; f += step, ++v ) {
                        *v = { AI_BE( f[0] ),
                               AI_BE( f[1] ),
                               AI_BE( f[2] ) };
                    }
                }
#endif
                break;
            case IQM_BLENDINDEXES:
            case IQM_BLENDWEIGHTS:
            case IQM_CUSTOM:
                break; // these attributes are not relevant.

            default:
                break;
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------

} // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_IQM_IMPORTER
