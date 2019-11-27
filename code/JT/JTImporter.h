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
#ifndef AI_JT_IMPORTER_H_INC
#define AI_JT_IMPORTER_H_INC

#ifndef ASSIMP_BUILD_NO_JT_IMPORTER

#include <assimp/BaseImporter.h>

#include <vector>
#include <cstdint>

namespace Assimp {

namespace JT {
    using uchar = unsigned char;

    using u8  = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using i8  = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    using f32 = float;
    using f64 = double;

    struct CoordF32 {
        f32 x, y, z;
    };

    struct CoordF64 {
        f64 x, y, z;
    };

    struct BBoxF32 {
        CoordF32 min, max;
    };

    struct Guid {
        u32 v1;
        u16 v2[2];
        u8  v3;
    };

    struct HCoordF32 {
        f32 x, y, z, w;
    };

    struct HCoordF64 {
        f64 x, y, z, w;
    };

    struct MbString {
        i32 count;
        u16 *chars;
    };

    struct Mx4F32 {
        f32 m[16];
    };

    struct PlaneF32 {
        f32 a, b, c, d;
    };

    struct Quaternion{
        f32 x, y, z, w;
    };

    struct RGB {
        f32 r, g, b;
    };

    struct RGBA {
        f32 r, g, b, a;
    };

    struct String {
        i32 count;
        u8  *chars;
    };

    struct VecF32 {
        i32 count;
        f32 *data;
    };

    struct VecF64 {
        i32 count;
        f64 *data;
    };

    struct VecI32 {
        i32 count;
        i32 *data;
    };

    struct VecU32 {
        i32 count;
        u32 *data;
    };

    struct JTModel;
    struct JTTOCEntry;
	struct LogicalElementHeaderZLib;
	struct SegmentHeader;
} // namespace JT

class JTImporter : public BaseImporter {
public:
    using DataBuffer = std::vector<char>;

    JTImporter();
    ~JTImporter() override;
    bool CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig ) const override;
    const aiImporterDesc* GetInfo() const override;

protected:
    void InternReadFile(const std::string& pFile,aiScene* pScene,IOSystem* pIOHandler) override;
    void readHeader(DataBuffer& buffer, size_t &offset);
    void readTOCSegment(size_t toc_offset, DataBuffer& buffer, size_t& offset);
    void readDataSegment( JT::JTTOCEntry *entry, DataBuffer &buffer, size_t &offset );
	void readLogicalElementHeaderZLib(JT::LogicalElementHeaderZLib &headerZLib, DataBuffer &buffer, size_t &offset);
	void readLSGSegment(JT::SegmentHeader header, bool isCompressed, DataBuffer &buffer, size_t &offset);

private:
    JT::JTModel* mModel;
};

} //! namespace Assimp

#endif // ASSIMP_BUILD_NO_JT_IMPORTER

#endif // AI_JT_IMPORTER_H_INC

