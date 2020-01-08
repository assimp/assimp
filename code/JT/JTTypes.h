#pragma once

#include <cstdint>

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

} // namespace JT
} // namespace Assimp
