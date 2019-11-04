/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team

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

#ifndef ASSIMP_BUILD_NO_JT_IMPORTER

#include "JTImporter.h"

#include <assimp/DefaultIOSystem.h>
#include <assimp/ByteSwapper.h>

#include <memory>

namespace Assimp {
namespace JT {

enum class SegmentType {
    LogicalScenegraph = 1,
    JT_BRep,
    PMIData,
    MetaData,
    Shape,
    ShapeLOD0,
    ShapeLOD1,
    ShapeLOD2,
    ShapeLOD3,
    ShapeLOD4,
    ShapeLOD5,
    ShapeLOD6,
    ShapeLOD7,
    ShapeLOD8,
    ShapeLOD9,
    XT_BRep,
    WireframeRepresentation,
    ULP,
    LWPA
};

static const char ZLibCompressionEnabled[] = {
    1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1
};

bool isZLibCompressionEnabled( SegmentType type ) {
    const size_t index = ( size_t )type - 1;
    return 1 == ZLibCompressionEnabled[ index ];
}

#pragma pack(1)
struct JTHeader {
    uchar version[80];
    uchar byte_order;
    i32   reserved;
    i32   toc_offset;
    union {
        Guid LSG_segment_id;
        Guid reserved_field;
    };

    JTHeader()
    : byte_order( 0 )
    , reserved( 0 )
    , toc_offset( 0 ) {
        ::memset(version, ' ', sizeof(uchar) * 80);
    }
};

struct JTTOCEntry {
    Guid segment_id;
    i32 segment_offset;
    i32 segment_lenght;
    u32 segment_attributes;
};

struct JTTOCSegment {
    i32 num_entries;
    JTTOCEntry* entries;

    JTTOCSegment()
    : num_entries(0)
    , entries(nullptr) {
        // empty
    }

    ~JTTOCSegment() {
        delete [] entries;
    }

    void allocEntries(i32 numEntries) {
        num_entries = num_entries;
        entries = new JTTOCEntry[num_entries];
    }

    JTTOCEntry* getEntryAt(size_t index) {
        if (index > num_entries) {
            return nullptr;
        }

        return &entries[index];
    }
};

struct SegmentHeader {
    Guid SegmentID;
    i32 SegmentType;
    i32 SegmentLength;
};

struct LogicalElementHeader {
    i32 length;
};

struct ElementHeader {
    Guid ObjectTypeID;
    uchar ObjectBaseType;
};

struct JTModel {
    JTHeader     header;
    JTTOCSegment toc_segment;
    JTModel() {
        // empty
    }
};

} //! namespace JT

static const aiImporterDesc desc = {
    "JT File Format from Siemens",
    "",
    "",
    "",
    aiImporterFlags_SupportTextFlavour,
    0,
    0,
    0,
    0,
    "jt"
};

using namespace ::Assimp::JT;

JTImporter::JTImporter()
: BaseImporter()
, mModel( nullptr ) {
    // empty
}

JTImporter::~JTImporter() {
    delete mModel;
    mModel = nullptr;
}

bool JTImporter::CanRead(const std::string& file, IOSystem* pIOHandler, bool checkSig) const {
    if (!checkSig) {
        //Check File Extension
        return SimpleExtensionCheck(file, "jt");
    }

    // Check file Header

    return false;
}

const aiImporterDesc* JTImporter::GetInfo() const {
    return &desc;
}

void JTImporter::InternReadFile(const std::string& file, aiScene* scene, IOSystem* ioHandler) {
    ai_assert(nullptr != scene);
    ai_assert(nullptr != ioHandler);

    static const std::string mode = "rb";
    std::unique_ptr<IOStream> fileStream(ioHandler->Open(file, mode));
    if (!fileStream.get()) {
        throw DeadlyImportError("Failed to open file " + file + ".");
    }

    // Get the file-size and validate it, throwing an exception when fails
    size_t fileSize = fileStream->FileSize();
    if (fileSize < sizeof(JTHeader)) {
        throw DeadlyImportError("JT-file is too small.");
    }

    mModel = new JTModel;
    DataBuffer buffer;
    buffer.resize(fileSize);
    fileStream->Read(&buffer[0], sizeof(char), fileSize);

    size_t offset(0);
    readHeader(buffer, offset);
    readTOCSegment(mModel->header.toc_offset, buffer, offset);

    for (size_t i = 0; i < mModel->toc_segment.num_entries; ++i) {

    }
}

void JTImporter::readHeader( DataBuffer& buffer, size_t& offset ) {
    ::memcpy(&mModel->header, &buffer[offset], sizeof(JTHeader));
    offset += sizeof(JTHeader);
}

void JTImporter::readTOCSegment(size_t toc_offset, DataBuffer& buffer, size_t& offset ) {
    i32 num_entries(0);
    ::memcpy(&num_entries, &buffer[toc_offset], sizeof(i32));
    offset = toc_offset + sizeof(i32);
    mModel->toc_segment.allocEntries(num_entries);
    for (i32 i = 0; i < num_entries; ++i) {
        JTTOCEntry* entry = mModel->toc_segment.getEntryAt(i);
        if (nullptr == entry) {
            continue;
        }

        ::memcpy( entry, &buffer[ offset ], sizeof( JTTOCEntry ) );
    }
}

void JTImporter::readDataSegment( JTTOCEntry *entry, DataBuffer &buffer, size_t &offset ) {
    if (nullptr == entry) {
        return;
    }
    SegmentHeader header;
    ::memcpy( &header, &buffer[ entry->segment_offset ], entry->segment_lenght );

    switch (header.SegmentType) {
        case SegmentType::LogicalScenegraph:
            break;
        case SegmentType::JT_BRep:
            break;
        case SegmentType::PMIData:
            break;
        case SegmentType::MetaData:
            break;
        case SegmentType::Shape:
            break;
        case SegmentType::ShapeLOD0:
            break;
        case SegmentType::ShapeLOD1:
            break;
        case SegmentType::ShapeLOD2:
            break;
        case SegmentType::ShapeLOD3:
            break;
        case SegmentType::ShapeLOD4:
            break;
        case SegmentType::ShapeLOD5:
            break;
        case SegmentType::ShapeLOD6:
            break;
        case SegmentType::ShapeLOD7:
            break;
        case SegmentType::ShapeLOD8:
            break;
        case SegmentType::ShapeLOD9:
            break;
        case SegmentType::XT_BRep:
            break;
        case SegmentType::WireframeRepresentation:
            break;
        case SegmentType::ULP:
            break;
        case SegmentType::LWPA:
            break;
        default:
            break;
    }
}


} //! namespace Assimp

#endif // ASSIMP_BUILD_NO_JT_IMPORTER
