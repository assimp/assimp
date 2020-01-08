#pragma once

#include <cstdint>
#include <vector>

namespace Assimp {
namespace JT {

using uchar = unsigned char;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
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
	u8 v3;
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

struct Quaternion {
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
	u8 *chars;
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

using DataBuffer = std::vector<char>;

struct JTMemoryReader {
	DataBuffer mBuffer;
	size_t mOffset;

    JTMemoryReader( DataBuffer &buffer )
    : mBuffer(buffer)
    , mOffset(0) {
		// empty
    }

    ~JTMemoryReader() {
		mOffset = 0;
    }

    i8 readI8() {
		i8 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(i8));
		mOffset += sizeof(i8);
		return val;
	}

    i16 readI16() {
		i16 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(i16));
		mOffset += sizeof(i16);
		return val;
    }

    i32 readI32() {
	    i32 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(i32));
		mOffset += sizeof(i32);
		return val;
	}

    u8 readU8() {
		u8 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(u8));
		mOffset += sizeof(u8);
		return val;
	}

	u16 readU16() {
		u16 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(u16));
		mOffset += sizeof(u16);
		return val;
	}

	u32 readU32() {
		u32 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(u32));
		mOffset += sizeof(u32);
		return val;
	}

    f32 readF32() {
		f32 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(f32));
		mOffset += sizeof(f32);

        return val;
	}

    f64 readF64() {
		f64 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(f64));
		mOffset += sizeof(f64);

        return val;
	}

    CoordF32 readCoordF32() {
		CoordF32 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(CoordF32));
		mOffset += sizeof(CoordF32);

        return val;
    }

    CoordF64 readCoordF64() {
		CoordF64 val;
		::memcpy(&val, &mBuffer[mOffset], sizeof(CoordF64));
		mOffset += sizeof(CoordF64);

        return val;
	}

    BBoxF32 readBBoxF32() {
		BBoxF32 bbox;
		bbox.min = readCoordF32();
		bbox.max = readCoordF32();

        return bbox;
    }

    Guid readGuid() {
		Guid val;
		memcpy(&val, &mBuffer[mOffset], sizeof(Guid));
		mOffset += sizeof(Guid);

        return val;
    }

    HCoordF32 readHCoordF32() {
		HCoordF32 val;
		memcpy(&val, &mBuffer[mOffset], sizeof(HCoordF32));
		mOffset += sizeof(HCoordF32);

        return val;
	}

    HCoordF64 readHCoordF64() {
		HCoordF64 val;
		memcpy(&val, &mBuffer[mOffset], sizeof(HCoordF64));
		mOffset += sizeof(HCoordF64);

		return val;
	}
};

} // namespace JT
} // namespace Assimp
