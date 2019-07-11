#include "JTImporter.h"

#include <assimp/StreamReader.h>
#include <assimp/MemoryIOWrapper.h>
#include <assimp/Importer.hpp>
#include <assimp/importerdesc.h>
#include <assimp/ai_assert.h>

#ifndef ASSIMP_BUILD_NO_JT_IMPORTER

namespace Assimp {

namespace {

    static const aiImporterDesc desc = {
        "Siemens JF File format importer",
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
}

enum SegmentType {
    LogicalSceneGraph = 1,
    JT_B_Rep,
    PMI_Data,
    MetaData,
    Shape,
    Shape_LOD0,
    Shape_LOD1,
    Shape_LOD2,
    Shape_LOD3,
    Shape_LOD4,
    Shape_LOD5,
    Shape_LOD6,
    Shape_LOD7,
    Shape_LOD8,
    Shape_LOD9,
    XT_B_Rep,
    Wireframe_Rep,
    ULP,
    LWPA
};

JTImporter::JTImporter()
: BaseImporter()
, mJTModel()
, mDataSegments()
, mCurrentDataSegment( nullptr ){
    // empty
}

JTImporter::~JTImporter() {
    // empty
}

bool JTImporter::CanRead(const std::string &file, IOSystem* pIOHandler, bool checkSig) const {
    const std::string& extension = GetExtension(file);
    if (extension == std::string(desc.mFileExtensions)) {
        return true;
    }

    return false;
}

const aiImporterDesc* JTImporter::GetInfo() const {
    return &desc;
}

void JTImporter::InternReadFile(const std::string &file, aiScene* pScene, IOSystem* pIOHandler) {

    std::unique_ptr<IOStream> stream(pIOHandler->Open(file, "rb"));
    if (!stream) {
        throw DeadlyImportError("JT: Cannot open file " + file );
    }

    std::vector<char> contents;
    contents.resize(stream->FileSize() + 1);
    stream->Read(&*contents.begin(), 1, contents.size() - 1);
    contents[contents.size() - 1] = 0;
    BinReader reader(contents);
    ReadHeader(reader);
}

void JTImporter::ReadHeader(BinReader& reader) {
    reader.readChars(VersionLen, mJTModel.mJTHeader.mVersion);
    reader.readUChar(mJTModel.mJTHeader.mByteOrder);
    reader.readI32(mJTModel.mJTHeader.mReserved);

    int toc_offset;
    reader.readI32(toc_offset);
}

void JTImporter::ReadTokenSegment(BinReader& reader) {
    int toc_count;
    reader.readI32(toc_count);
    if (0 == toc_count) {
        return;
    }

    for (int i = 0; i < toc_count; ++i) {
        ReadTokenEntry(reader);
    }
}

void JTImporter::ReadTokenEntry(BinReader& reader) {
    TokenEntry* entry = new TokenEntry;
    reader.readGUID(entry->guid);
    reader.readI32(entry->offset);
    reader.readI32(entry->length);
    reader.readU32(entry->attributes);

    mJTModel.mTokenEntryMap[entry->guid] = entry;
}

TokenEntry* JTImporter::FindTokenEntryByGuid(GUID& guid) {
    if (mJTModel.mTokenEntryMap.empty()) {
        return nullptr;
    }

    JTModel::TokenEntryMap::const_iterator it(mJTModel.mTokenEntryMap.find(guid));
    if (it == mJTModel.mTokenEntryMap.end()) {
        return nullptr;
    }

    return it->second;
}
/*
static bool SupportsZlibCompression(SegmentType type) {
    switch (type) {
        case LogicalSceneGraph:
        case JT_B_Rep:
        case PMI_Data:
        case MetaData:
            return true;
        case Shape:
        case Shape_LOD0:
        case Shape_LOD1:
        case Shape_LOD2:
        case Shape_LOD3:
        case Shape_LOD4:
        case Shape_LOD5:
        case Shape_LOD6:
        case Shape_LOD7:
        case Shape_LOD8:
        case Shape_LOD9:
            return false;
        case XT_B_Rep:
        case Wireframe_Rep:
        case ULP:
        case LWPA:
            return true;
        default:
            ai_assert_entry();
            break;
    }

    return false;
}*/

void JTImporter::ReadDataSegment(BinReader& reader) {
    mCurrentDataSegment = new DataSegment;
    mDataSegments.push_back(mCurrentDataSegment);
    ReadDataSegmentHeader(reader);

}

void JTImporter::ReadDataSegmentHeader(BinReader& reader) {
    if (nullptr == mCurrentDataSegment) {
        return;
    }

    reader.readGUID(mCurrentDataSegment->m_DataSegmentHeader.guid);
    reader.readI32(mCurrentDataSegment->m_DataSegmentHeader.type);
    reader.readI32(mCurrentDataSegment->m_DataSegmentHeader.length);

}

void JTImporter::ReadLogicalElementHeaderZLib(BinReader& reader) {
    i32 compressionFlag;
    reader.readI32(compressionFlag);
    i32 compressedLen;
    reader.readI32(compressedLen);
    u8 compressionAlgo;
    reader.readUChar(compressionAlgo);
}

void JTImporter::ReadSegmentType(BinReader& reader) {
    SegmentType type = static_cast<SegmentType>(mCurrentDataSegment->m_DataSegmentHeader.type);
    switch (type) {
        case LogicalSceneGraph:
            ReadLSG(reader);
            break;
        case JT_B_Rep:
            break;
        case PMI_Data:
            break;
        case MetaData:
            break;
        case Shape:
            break;
        case Shape_LOD0:
            break;
        case Shape_LOD1:
            break;
        case Shape_LOD2:
            break;
        case Shape_LOD3:
            break;
        case Shape_LOD4:
            break;
        case Shape_LOD5:
            break;
        case Shape_LOD6:
            break;
        case Shape_LOD7:
            break;
        case Shape_LOD8:
            break;
        case Shape_LOD9:
            break;
        case XT_B_Rep:
            break;
        case Wireframe_Rep:
            break;
        case ULP:
            break;
        case LWPA:
            break;
        default:
            break;
    }
}

void JTImporter::ReadLSG(BinReader& reader) {
    ReadLogicalElementHeaderZLib(reader);
}

} // end namespace Assimp

#endif // ASSIMP_BUILD_NO_JT_IMPORTER
