#pragma once
#ifndef AI_JTIMPORTER_H_INC
#define AI_JTIMPORTER_H_INC

#include <assimp/BaseImporter.h>

#include <vector>
#include <map>

#ifndef ASSIMP_BUILD_NO_JT_IMPORTER

namespace Assimp {

using i32 = int;
using u32 = unsigned int;
using i16 = short;
using u16 = unsigned short;
using c8 = char;
using u8 = unsigned char;
using f32 = float;
using f64 = double;

static const size_t VersionLen = 80u;

struct JTHeader {
    char mVersion[VersionLen];
    unsigned char mByteOrder;
    int mReserved;
    int toc_offset;
};

template<class T>
struct TJTCoord {
    T x, y, z;
};

using F32Coord = TJTCoord<f32>;
using F64Coord = TJTCoord<f64>;

struct BBox32 {
    F32Coord min;
    F32Coord max;
};

template<class T>
struct THJTCoord {
    T x, y, z, w;
};

using F32HCoord = THJTCoord<f32>;
using F64HCoord = THJTCoord<f64>;

struct MbString {
    int count;
    char *data;
};

struct String {
    int count;
    char* data;
};

template<class T>
struct TVec {
    int count;
    T *data;
};

using VecF32 = TVec<f32>;
using VecF64 = TVec<f64>;
using VecI32 = TVec<i32>;

struct RGB {
    f32 data[3];
};

struct RGBA {
    f32 data[4];
 };

struct GUID {
    unsigned int id1;
    u16 id2[2];
    u8 id3[8];
};

struct GUIDComparer {
    bool operator() (const GUID& lhs, const GUID& rhs) const {
        if (lhs.id1 < rhs.id1) {
            return true;
        }

        for (size_t i = 0; i < 2; ++i) {
            if (lhs.id2[i] < rhs.id2[i]) {
                return true;
            }
        }

        for (size_t i = 0; i < 8; ++i) {
            if (lhs.id3[i] < rhs.id3[i]) {
                return true;
            }
        }

        return false;
    }
};

struct TokenEntry {
    GUID guid;
    i32 offset;
    i32 length;
    u32 attributes;
};

struct DataSegmentHeader {
    GUID guid;
    i32 type;
    i32 length;
};

struct DataSegment {
    DataSegmentHeader m_DataSegmentHeader;
};

struct ElementHeader {
    GUID obj_type_id;
    u8 obj_base_type;
    i32 obj_id;
};

struct JTModel {
    JTHeader mJTHeader;
    GUID mGUID;
    using TokenEntryMap = std::map<GUID, TokenEntry*, GUIDComparer>;
    TokenEntryMap mTokenEntryMap;
};

class BinReader {
    std::vector<char>& mData;
    size_t mOffset;

public:
    BinReader(std::vector<char>& data)
    : mData(data)
    , mOffset(0u){
        // empty
    }

    void readChars(size_t numChars, char* buffer) {
        ::memcpy(buffer, &mData[mOffset], numChars);
        mOffset += numChars;
    }

    void readUChar(unsigned char &c) {
        ::memcpy(&c, &mData[mOffset], sizeof(unsigned char));
        mOffset += sizeof(unsigned char);
    }

    void readI32(int32_t& v) {
        ::memcpy(&v, &mData[mOffset], sizeof(int32_t));
        mOffset += sizeof(int32_t);
    }

    void readU32(uint32_t& v) {
        ::memcpy(&v, &mData[mOffset], sizeof(uint32_t));
        mOffset += sizeof(uint32_t);
    }

    void readGUID(GUID &guid) {
        ::memcpy(&guid.id1, &mData[mOffset], sizeof(i32));
        mOffset += sizeof(i32);
        ::memcpy(guid.id2, &mData[mOffset], sizeof(u16) * 2);
        mOffset += sizeof(u16)*2;
        ::memcpy(guid.id3, &mData[mOffset], sizeof(u8) * 8);
        mOffset += sizeof(u8) * 8;
    }
};

class JTImporter : public BaseImporter {
public:
    JTImporter();
    ~JTImporter();
    bool CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig ) const override;
    const aiImporterDesc* GetInfo() const override;

protected:
    void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler ) override;
    void ReadHeader(BinReader &reader);
    void ReadTokenSegment(BinReader& reader);
    void ReadTokenEntry(BinReader& reader);
    TokenEntry* FindTokenEntryByGuid(GUID& id);
    void ReadDataSegment(BinReader& reader);
    void ReadDataSegmentHeader(BinReader& reader);

private:
    JTModel mJTModel;
    std::vector<DataSegment*> mDataSegments;
    DataSegment *mCurrentDataSegment;

};

} // end namespace Assimp

#endif // ASSIMP_BUILD_NO_JT_IMPORTER

#endif // AI_JTIMPORTER_H_INC
