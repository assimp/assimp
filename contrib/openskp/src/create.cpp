// C++ port of packages/python/src/openskp/create.py - see the module docstring at the top of
// include/openskp/create.hpp for the overall approach (blank-scaffold splicing) and scope.
//
// This file additionally bakes in, as compile-time constants, the handful of scaffold-derived
// *positions* that Python's `SkpBuilder.__init__` computes at runtime by walking the scaffold
// file with the generic MFC archive reader (openskp.legacy._Archive + _probe_layer_anchor_bases).
// Since the scaffold (scaffold_blank_v17.hpp's kScaffoldBlankV17) is a fixed, version-controlled
// byte array - identical to the Python copy, sha256-verified below - those walked-to positions
// are themselves fixed for as long as the scaffold is. Re-deriving them generically here would
// mean porting a large fraction of the generic archive reader (entity preambles, drawbase
// records, definition/instance decoding) purely to recompute numbers that never change; baking
// them in avoids that duplication with no behavioral difference.
//
// The constants below were produced by running, against the *exact* bundled scaffold file
// (packages/python/src/openskp/_scaffold/blank_v17.skp, sha256
// 809a1ab73a20a192ab13aaff197afb1c67d0e9352f6a353a9cd8030919f8a6c3):
//
//   import openskp
//   b = sys.modules['openskp.create'].SkpBuilder()
//   # then print each of: b._material_insert_pos, b._base, b._layer_count_pos,
//   # b._orig_layer_count, b._layer_insert_pos, b._layer_writer_base, b._def_count_pos,
//   # b._orig_def_count, b._root_count_pos, b._orig_root_count, b._tail_pos,
//   # b._scaffold_next_slot, b._scaffold_class_slot
//
// If the bundled scaffold is ever swapped for a different blank document, re-run that recipe
// against the new file and update scaffold_blank_v17.hpp (see its own regeneration comment) AND
// every constant below together. Unlike Python's `_load_scaffold` (which hashes the scaffold at
// *runtime* because it loads it from a separate file on disk via importlib.resources, a resource
// that could drift from the offsets independently), this port has no equivalent runtime check:
// the byte array here is a compile-time constant reviewed and compiled together with the offset
// constants below, not a mutable external resource that could go stale between builds.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iterator>
#include <random>
#include <utility>

#include <openskp/create.hpp>

#include "scaffold_blank_v17.hpp"

namespace openskp {
namespace detail {
namespace {

constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------------------------------------
// Scaffold-derived constants (see the file-level comment above for how these were produced).
// ---------------------------------------------------------------------------------------------

// Absolute byte offset of the material-manager insertion point (the position right before the
// "layer list marker" that a zero-material scaffold starts with) - Python's `_material_insert_pos`
// / `start`.
constexpr std::size_t kMaterialInsertPos = 3400;
// The scaffold-derived starting archive slot for anything spliced in after the (empty) material
// section - Python's `_base`.
constexpr int kBase = 9;
// Absolute offset of the u32 layer-count field that precedes the layer list.
constexpr std::size_t kLayerCountPos = 3405;
constexpr int kOrigLayerCount = 1;
// Absolute offset right after the scaffold's own existing layer records (Layer0) - where new
// layers splice in.
constexpr std::size_t kLayerInsertPos = 3505;
// Scaffold-derived starting archive slot for a freshly-created layer writer, before accounting
// for any material shift - Python's `_layer_writer_base` (local var `layer_writer_base`).
constexpr int kLayerWriterBase = 11;
// Absolute offset of the u32 component-definition-count field.
constexpr std::size_t kDefCountPos = 3507;
constexpr int kOrigDefCount = 0;
// Absolute offset of the u32 root-entity-count field.
constexpr std::size_t kRootCountPos = 3511;
constexpr int kOrigRootCount = 0;
// Absolute offset where the document "tail" (the undecoded style/font-manager region) begins.
constexpr std::size_t kTailPos = 3515;
// The scaffold-derived starting slot for anything written after the (always byte-for-byte-
// copied) layer/definition/root-entity region - Python's `_scaffold_next_slot`.
constexpr int kScaffoldNextSlot = 11;

// Absolute offset (BEFORE the material insertion point, so only its value needs correction, not
// its position) of a u16 "next available pid" counter. Increments by exactly the material count
// (one pid consumed per material object).
constexpr std::size_t kPidCounterPos = 1987;

// Offsets (relative to the start of the document "tail") of internal references that must be
// renumbered by the same amount as the number of new archive slots inserted before them. See
// create.hpp's module docstring / the Python module's own `_TAIL_REF_POSITIONS` comment for how
// these were found.
constexpr std::size_t kTailRefPositions[] = {409, 468, 477, 479, 1383, 1385};

// Offset (relative to the material-manager insertion point) of the active-layer anchor - a
// back-reference to the model's first layer (Layer0). Moves only when materials shift Layer0's
// own slot.
constexpr std::size_t kActiveLayerAnchorRel = 0;

constexpr int kLayerSchema = 3;
constexpr int kMaterialSchema = 12;
constexpr int kDibSchema = 3;
constexpr int kDefinitionSchema = 11;
constexpr int kInstanceSchema = 6;
constexpr int kGroupSchema = 1;
// UNVERIFIED - unlike every other schema constant here, not calibrated against a real
// SketchUp-authored file: no sample containing a CImage entity (File > Import > Image) was
// available (ported from the Python writer, same caveat there - see create.py's own comment).
// legacy.cpp's image reader never branches on schema the way instance/group reading does, so
// this project's own reader round-trips correctly regardless of the exact value - this only
// affects whether real SketchUp accepts the file. Chosen to match kInstanceSchema for the same
// reason as Python's _IMAGE_SCHEMA: CImage's read path always expects the trailing GUID
// unconditionally, the same "always present" shape CComponentInstance has.
constexpr int kImageSchema = 6;
constexpr int kThumbnailSchema = 1;
constexpr int kFtcSchema = 4;
constexpr int kArcCurveSchema = 3;
constexpr int kCCurveSchema = 4;

constexpr int kSectionPlaneSchema = 3;
constexpr int kDimensionLinearSchema = 6;
constexpr int kSkFontSchema = 1;
constexpr int kTextSchema = 9;
constexpr int kConstructionLineSchema = 1;
constexpr int kConstructionPointSchema = 0;

// Sentinel a CConstructionLine's start/end distance-parameter carries when unbounded in that
// direction - real SketchUp's own value, ground-truth verified against
// Sketchup::ConstructionLine#start/#end returning nil for that side.
constexpr double kClineInfinite = 1e30;

// CCamera's class is declared inside the scaffold's own style/scene-manager prefix - ground-truth
// confirmed fixed at slot 7 for this exact bundled scaffold file.
constexpr int kCCameraSlot = 7;
// CAttributeContainer's class, likewise pre-declared in the scaffold's own prefix.
constexpr int kAttrContainerSlot = 3;
// CAttributeNamed's class, likewise pre-declared in the scaffold's own prefix.
constexpr int kAttributeNamedSlot = 5;

constexpr std::uint8_t kAttrTypeInt32 = 0x04;
constexpr std::uint8_t kAttrTypeDouble = 0x06;
constexpr std::uint8_t kAttrTypeString = 0x0A;

// Byte pattern found in one SDK-authored textured-material sample's "applied height" field,
// decoding to ~1.29e-231 as an f64 - not a meaningful height value. Confirmed 2026-08-27 via
// real SketchUp screenshots that a material written with this exact value renders as a
// corrupted, vertically-smeared texture, so it was never a genuine "never scaled" default -
// almost certainly uninitialized memory in the one sample file this was calibrated against,
// given write_textured_material's applied WIDTH is unconditionally 1.0 (a clearly deliberate
// value) with no equivalent garbage pattern. No longer used as this project's own default (see
// write_textured_material, which now defaults to 1.0 instead) - kept only as a documented
// historical artifact of what real SketchUp can apparently write here.
[[maybe_unused]] constexpr std::uint8_t kTextureHSentinel[8] = {240, 255, 255, 255,
                                                                255, 255, 255, 15};

// The definition record's 22-byte "base block" (immediately after its own preamble, before the
// embedded layer list) - all zero except offsets 3-4 (the same 1,1 padding convention drawbase
// uses).
constexpr std::uint8_t kDefinitionBaseBlock[22] = {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
                                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// The 176 bytes (everything after CCamera's 2-byte class-ref tag) real SketchUp writes for a
// definition's default thumbnail camera - copied verbatim, not decoded.
constexpr std::uint8_t kCameraTemplate[176] = {
    0,  0, 0,   0,  0, 0, 0, 0, 0, 0,   0,   0,   0,  0, 0, 0, 0, 0,  0,  0, 0, 0, 240, 63, 0, 0,
    0,  0, 0,   0,  0, 0, 0, 0, 0, 0,   0,   0,   0,  0, 0, 0, 0, 0,  0,  0, 0, 0, 0,   64, 0, 0,
    0,  0, 0,   0,  0, 0, 0, 0, 0, 0,   0,   240, 63, 0, 0, 0, 0, 0,  0,  0, 0, 0, 0,   0,  0, 0,
    0,  0, 0,   0,  0, 0, 0, 0, 0, 0,   0,   1,   0,  0, 0, 0, 0, 62, 64, 0, 0, 0, 0,   0,  0, 240,
    63, 0, 0,   0,  0, 0, 0, 0, 0, 0,   0,   0,   0,  0, 0, 0, 0, 0,  0,  0, 0, 0, 0,   0,  0, 0,
    0,  0, 0,   0,  0, 0, 0, 1, 0, 255, 254, 255, 0,  0, 0, 0, 0, 0,  0,  0, 0, 0, 0,   0,  0, 0,
    0,  0, 240, 63, 0, 0, 0, 0, 0, 0,   0,   0,   0,  0, 0, 0, 0, 0,  0,  0,
};

// The blank scaffold ships with SketchUp's own arbitrary default camera; every file this writer
// produces instead patches it to the standard "Iso" view. Found by diffing two SDK-authored blank
// documents that differ only in an explicit SUCameraSetOrientation + SUCameraSetPerspective(False)
// call before saving - copied verbatim rather than decoded.
constexpr std::size_t kIsoCameraPrefixOffset = 2993;
constexpr std::uint8_t kIsoCameraPrefixPatch[98] = {
    89,  64,  0,   0,  0,   0,   0,   0,  89,  192, 0,  0,  0,   0,   0,   0,   89,  64, 0,  0,
    0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,  0,  0,   0,   0,   0,   0,   0,  0,  0,
    0,   0,   63,  44, 12,  112, 189, 32, 218, 191, 63, 44, 12,  112, 189, 32,  218, 63, 63, 44,
    12,  112, 189, 32, 234, 63,  0,   0,  0,   0,   0,  0,  240, 63,  0,   0,   0,   0,  0,  64,
    143, 64,  0,   0,  0,   0,   0,   0,  0,   62,  64, 42, 223, 39,  44,  128, 52,  87,
};

const std::uint8_t kIsoCameraTailPatch1[16] = {208, 168, 105, 97,  60, 68,  45,  71,
                                               153, 164, 102, 125, 26, 223, 168, 54};
const std::uint8_t kIsoCameraTailPatch2[15] = {78,  83,  200, 68, 119, 2,   146, 70,
                                               187, 169, 88,  39, 187, 167, 226};

struct TailPatch {
  std::size_t pos;
  const std::uint8_t* data;
  std::size_t size;
};

const TailPatch kIsoCameraTailPatches[2] = {
    {509, kIsoCameraTailPatch1, sizeof(kIsoCameraTailPatch1)},
    {1390, kIsoCameraTailPatch2, sizeof(kIsoCameraTailPatch2)},
};

// A face with no explicit texture positioning stores no CFaceTextureCoords at all, so this
// identity is only ever used to fill the *other* side's slot when just one of front/back is
// explicitly positioned.
constexpr Matrix3x3 kIdentityUvMatrix = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};

// ---------------------------------------------------------------------------------------------
// Low-level byte helpers.
// ---------------------------------------------------------------------------------------------

void append_u16(ByteBuffer& buf, std::uint32_t v) {
  buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void append_u32(ByteBuffer& buf, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) buf.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
}

void append_i32(ByteBuffer& buf, std::int32_t v) { append_u32(buf, static_cast<std::uint32_t>(v)); }

void append_f64(ByteBuffer& buf, double v) {
  std::uint64_t bits = 0;
  std::memcpy(&bits, &v, sizeof(bits));
  for (int i = 0; i < 8; ++i) buf.push_back(static_cast<std::uint8_t>((bits >> (8 * i)) & 0xFF));
}

void append_bytes(ByteBuffer& buf, const std::uint8_t* data, std::size_t n) {
  buf.insert(buf.end(), data, data + n);
}

void append_zeros(ByteBuffer& buf, std::size_t n) { buf.insert(buf.end(), n, 0); }

// Parses a hex string into a ByteBuffer - used only for the handful of byte-exact templates
// below, ported verbatim (same hex text) from create.py's own bytes.fromhex(...) constants.
ByteBuffer from_hex(const std::string& hex) {
  ByteBuffer out(hex.size() / 2);
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint8_t>(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
  }
  return out;
}

// Byte-exact templates harvested from a real SketchUp 2017 file (28 dimensions, capilla quiroz
// corpus model); see docs/dimension-record-notes.md for the full layout. Ported verbatim from
// create.py's own _DIM_FONT_PAYLOAD/_TEXT_DELIM.
const ByteBuffer kDimFontPayload = from_hex(
    "000000"                            // preamble: null attrs + pid mask 0
    "fffeff065400610068006f006d006100"  // "Tahoma"
    "0000"
    "08000000"
    "00"
    "ecf57abd5eaf2340");  // height f64

// Leader-text delimiter block: [u32 1][u8 flag=1][u8 0][u32 ARROW=3 closed][u8 1]
const ByteBuffer kTextDelim = from_hex("0100000001000300000001");

std::uint16_t read_u16_le(const ByteBuffer& buf, std::size_t pos) {
  return static_cast<std::uint16_t>(buf[pos]) | static_cast<std::uint16_t>(buf[pos + 1] << 8);
}

void write_u16_at(ByteBuffer& buf, std::size_t pos, std::uint16_t v) {
  buf[pos] = static_cast<std::uint8_t>(v & 0xFF);
  buf[pos + 1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
}

void write_u32_at(ByteBuffer& buf, std::size_t pos, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) buf[pos + i] = static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF);
}

// UTF-8 (as held by std::string, this project's convention) -> UTF-16LE code units, matching
// Python's `str.encode('utf-16-le')` (astral characters become surrogate pairs). Used by
// ArchiveWriter::write_str, the exact inverse of Python's own `_write_str`.
std::u16string utf8_to_utf16(const std::string& s) {
  std::u16string out;
  std::size_t i = 0, n = s.size();
  while (i < n) {
    unsigned char c0 = static_cast<unsigned char>(s[i]);
    std::uint32_t cp;
    int len;
    if (c0 < 0x80) {
      cp = c0;
      len = 1;
    } else if ((c0 & 0xE0) == 0xC0) {
      cp = c0 & 0x1F;
      len = 2;
    } else if ((c0 & 0xF0) == 0xE0) {
      cp = c0 & 0x0F;
      len = 3;
    } else if ((c0 & 0xF8) == 0xF0) {
      cp = c0 & 0x07;
      len = 4;
    } else {
      throw SkpWriteError("invalid UTF-8 string passed to openskp::create");
    }
    if (i + static_cast<std::size_t>(len) > n) {
      throw SkpWriteError("invalid (truncated) UTF-8 string passed to openskp::create");
    }
    for (int k = 1; k < len; ++k) {
      unsigned char ck = static_cast<unsigned char>(s[i + k]);
      if ((ck & 0xC0) != 0x80)
        throw SkpWriteError("invalid UTF-8 string passed to openskp::create");
      cp = (cp << 6) | (ck & 0x3F);
    }
    i += static_cast<std::size_t>(len);
    if (cp <= 0xFFFF) {
      out.push_back(static_cast<char16_t>(cp));
    } else {
      cp -= 0x10000;
      out.push_back(static_cast<char16_t>(0xD800 + (cp >> 10)));
      out.push_back(static_cast<char16_t>(0xDC00 + (cp & 0x3FF)));
    }
  }
  return out;
}

std::array<std::uint8_t, 16> random_uuid_bytes() {
  static thread_local std::mt19937_64 rng{std::random_device{}()};
  std::uniform_int_distribution<int> dist(0, 255);
  std::array<std::uint8_t, 16> b{};
  for (auto& x : b) x = static_cast<std::uint8_t>(dist(rng));
  b[6] = static_cast<std::uint8_t>((b[6] & 0x0F) | 0x40);  // RFC 4122 version 4
  b[8] = static_cast<std::uint8_t>((b[8] & 0x3F) | 0x80);  // RFC 4122 variant 10
  return b;
}

// Renumber the u16 archive slot-reference at `pos` (within `buf`) by `shift`, preserving the
// 0x8000 class-ref tag bit if the reference carries one. Widens to the 6-byte escape form (same
// encoding ArchiveWriter::new_of_known_class/backref use, and the same `< 0x7FFF` boundary) if
// the shifted slot would land at or past 0x7FFF - see create.hpp's module docstring for why that
// boundary is exact (0x7FFF is the archive's own big-tag escape marker, and 0x8000|0x7FFF ==
// 0xFFFF is its new-class-declaration marker; neither can be reused as an ordinary slot number).
// Returns the number of bytes the field grew by (0 or 4).
std::size_t shift_ref(ByteBuffer& buf, std::size_t pos, int shift) {
  std::uint16_t u16 = read_u16_le(buf, pos);
  std::uint16_t tag_bit = u16 & 0x8000;
  int slot = u16 & 0x7FFF;
  int new_slot = slot + shift;
  if (new_slot < 0x7FFF) {
    write_u16_at(buf, pos,
                 static_cast<std::uint16_t>(tag_bit | static_cast<std::uint16_t>(new_slot)));
    return 0;
  }
  std::uint32_t val = tag_bit ? (0x80000000u | static_cast<std::uint32_t>(new_slot))
                              : static_cast<std::uint32_t>(new_slot);
  ByteBuffer replacement(6);
  replacement[0] = 0xFF;
  replacement[1] = 0x7F;
  for (int i = 0; i < 4; ++i)
    replacement[2 + i] = static_cast<std::uint8_t>((val >> (8 * i)) & 0xFF);
  buf.erase(buf.begin() + static_cast<std::ptrdiff_t>(pos),
            buf.begin() + static_cast<std::ptrdiff_t>(pos + 2));
  buf.insert(buf.begin() + static_cast<std::ptrdiff_t>(pos), replacement.begin(),
             replacement.end());
  return 4;
}

int detect_image_subtype(const ByteBuffer& image_bytes) {
  static const std::uint8_t png_sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  if (image_bytes.size() >= 8 && std::equal(png_sig, png_sig + 8, image_bytes.begin())) return 4;
  if (image_bytes.size() >= 3 && image_bytes[0] == 0xFF && image_bytes[1] == 0xD8 &&
      image_bytes[2] == 0xFF) {
    return 1;
  }
  throw SkpWriteError(
      "unrecognized image format - only PNG and JPEG textures are supported for now "
      "(detected from the file's own magic bytes, not its extension)");
}

// ---------------------------------------------------------------------------------------------
// Small geometry/linear-algebra helpers.
// ---------------------------------------------------------------------------------------------

double det3(const std::array<std::array<double, 3>, 3>& m) {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
         m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

std::array<double, 3> solve3x3(const std::array<std::array<double, 3>, 3>& a,
                               const std::array<double, 3>& b) {
  double d = det3(a);
  if (std::abs(d) < 1e-9) {
    throw SkpWriteError(
        "the 3 texture-positioning points map to collinear (u, v) coordinates - "
        "cannot determine a texture mapping from them");
  }
  std::array<double, 3> cols{};
  for (int col = 0; col < 3; ++col) {
    auto ai = a;
    for (int r = 0; r < 3; ++r) ai[r][col] = b[r];
    cols[col] = det3(ai) / d;
  }
  return cols;
}

Point3 cross3(Point3 a, Point3 b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

Point3 normalize3(Point3 v) {
  double length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (length < 1e-9) {
    throw SkpWriteError(
        "cannot determine a texture-positioning basis: the face's first edge is degenerate");
  }
  return {v[0] / length, v[1] / length, v[2] / length};
}

// The row-major 3x3 rotation matrix for rotating by `angle` radians (right-hand rule) around
// `axis` (need not be a unit vector) - Rodrigues' rotation formula.
Matrix3x3 rotation_matrix3x3(Point3 axis, double angle) {
  double length = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
  if (length < 1e-9) throw SkpWriteError("rotation axis must not be the zero vector");
  double x = axis[0] / length, y = axis[1] / length, z = axis[2] / length;
  double c = std::cos(angle), s = std::sin(angle), t = 1.0 - c;
  return {
      t * x * x + c,     t * x * y - s * z, t * x * z + s * y, t * x * y + s * z, t * y * y + c,
      t * y * z - s * x, t * x * z - s * y, t * y * z + s * x, t * z * z + c,
  };
}

// Shared by every add_instance/add_group/add_group_instance overload - `matrix3x3` and
// `rotation` are alternate ways to specify the same underlying transform field, not two separate
// ones, so exactly one (or neither, for identity) may be given.
std::optional<Matrix3x3> resolve_matrix3x3(std::optional<Matrix3x3> matrix3x3,
                                           std::optional<Rotation> rotation) {
  if (matrix3x3 && rotation) {
    throw SkpWriteError(
        "pass at most one of matrix3x3/rotation - rotation is just a convenience for matrix3x3");
  }
  if (rotation) return rotation_matrix3x3(rotation->first, rotation->second);
  return matrix3x3;
}

// The in-plane 2D basis (U, W) real SketchUp uses to parameterize a face's texture mapping, for
// a face of ANY orientation - the face's own first edge direction (points[1] - points[0],
// normalized) as U, and the plane normal crossed with that as W.
std::pair<Point3, Point3> face_uv_basis(const std::vector<Point3>& points, Point3 normal) {
  Point3 u = normalize3(
      {points[1][0] - points[0][0], points[1][1] - points[0][1], points[1][2] - points[0][2]});
  Point3 w = normalize3(cross3(normal, u));
  return {u, w};
}

// An arbitrary orthonormal in-plane basis (U, W) for a circle/arc's plane, given only its
// normal - pick whichever of world +Z/+X is less parallel to `normal` as a seed and
// Gram-Schmidt it against `normal` to get U, then W = normal x U.
std::pair<Point3, Point3> circle_basis(Point3 normal) {
  Point3 seed = std::abs(normal[2]) < 0.9 ? Point3{0.0, 0.0, 1.0} : Point3{1.0, 0.0, 0.0};
  double dot = seed[0] * normal[0] + seed[1] * normal[1] + seed[2] * normal[2];
  Point3 u_raw = {seed[0] - dot * normal[0], seed[1] - dot * normal[1], seed[2] - dot * normal[2]};
  Point3 u = normalize3(u_raw);
  Point3 w = normalize3(cross3(normal, u));
  return {u, w};
}

std::vector<Point3> circle_points(Point3 center, Point3 /*normal*/, double radius, int num_segments,
                                  Point3 u, Point3 w) {
  std::vector<Point3> pts;
  pts.reserve(static_cast<std::size_t>(num_segments));
  for (int i = 0; i < num_segments; ++i) {
    double angle = 2.0 * kPi * i / num_segments;
    double c = std::cos(angle), s = std::sin(angle);
    pts.push_back({center[0] + radius * (c * u[0] + s * w[0]),
                   center[1] + radius * (c * u[1] + s * w[1]),
                   center[2] + radius * (c * u[2] + s * w[2])});
  }
  return pts;
}

// The `num_segments + 1` points (both endpoints included) tracing a PARTIAL arc from
// `start_angle` to `end_angle`.
std::vector<Point3> arc_points(Point3 center, Point3 /*normal*/, double radius, int num_segments,
                               Point3 u, Point3 w, double start_angle, double end_angle) {
  std::vector<Point3> pts;
  pts.reserve(static_cast<std::size_t>(num_segments) + 1);
  for (int i = 0; i <= num_segments; ++i) {
    double angle = start_angle + (end_angle - start_angle) * i / num_segments;
    double c = std::cos(angle), s = std::sin(angle);
    pts.push_back({center[0] + radius * (c * u[0] + s * w[0]),
                   center[1] + radius * (c * u[1] + s * w[1]),
                   center[2] + radius * (c * u[2] + s * w[2])});
  }
  return pts;
}

// Fit the 3x3 UV-to-world affine matrix ground truth shows real SketchUp stores for a
// positioned texture, from exactly 3 (world point, (u, v)) correspondences.
Matrix3x3 solve_uv_matrix(const UvCorrespondence& pairs, const std::pair<Point3, Point3>& basis) {
  if (pairs.size() != 3)
    throw SkpWriteError("texture positioning needs exactly 3 (point, uv) pairs");
  const auto& u_axis = basis.first;
  const auto& w_axis = basis.second;
  std::array<std::array<double, 3>, 3> a{};
  std::array<double, 3> bx{}, by{};
  for (int i = 0; i < 3; ++i) {
    const Point3& pt = pairs[static_cast<std::size_t>(i)].first;
    const auto& uv = pairs[static_cast<std::size_t>(i)].second;
    a[static_cast<std::size_t>(i)] = {uv[0], uv[1], 1.0};
    bx[static_cast<std::size_t>(i)] = pt[0] * u_axis[0] + pt[1] * u_axis[1] + pt[2] * u_axis[2];
    by[static_cast<std::size_t>(i)] = pt[0] * w_axis[0] + pt[1] * w_axis[1] + pt[2] * w_axis[2];
  }
  auto col_x = solve3x3(a, bx);
  auto col_y = solve3x3(a, by);
  double a0 = col_x[0], c0 = col_x[1], e0 = col_x[2];
  double b0 = col_y[0], d0 = col_y[1], f0 = col_y[2];
  return {a0, b0, 0.0, c0, d0, 0.0, e0, f0, 1.0};
}

Matrix3x3 uv_matrix_for_face(const std::vector<Point3>& points, const UvCorrespondence& pairs,
                             Point3 normal) {
  return solve_uv_matrix(pairs, face_uv_basis(points, normal));
}

double polygon_span(const std::vector<Point3>& points) {
  double span = 0.0;
  for (int i = 0; i < 3; ++i) {
    double mn = points[0][static_cast<std::size_t>(i)];
    double mx = mn;
    for (const auto& p : points) {
      mn = std::min(mn, p[static_cast<std::size_t>(i)]);
      mx = std::max(mx, p[static_cast<std::size_t>(i)]);
    }
    span = std::max(span, mx - mn);
  }
  return span;
}

struct PlaneResult {
  double nx, ny, nz, d;
};

// Newell's method: sums a cross-product-like term over every edge rather than reading the
// normal off just the first 3 points - correct for concave polygons too, as long as the
// polygon is planar and simple.
PlaneResult newell_normal(const std::vector<Point3>& points) {
  std::size_t n = points.size();
  double nx = 0, ny = 0, nz = 0;
  for (std::size_t i = 0; i < n; ++i) {
    const auto& p0 = points[i];
    const auto& p1 = points[(i + 1) % n];
    nx += (p0[1] - p1[1]) * (p0[2] + p1[2]);
    ny += (p0[2] - p1[2]) * (p0[0] + p1[0]);
    nz += (p0[0] - p1[0]) * (p0[1] + p1[1]);
  }
  double length = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (length < 1e-9)
    throw SkpWriteError("face points are collinear or degenerate; cannot compute a plane");
  nx /= length;
  ny /= length;
  nz /= length;
  double cx = 0, cy = 0, cz = 0;
  for (const auto& p : points) {
    cx += p[0];
    cy += p[1];
    cz += p[2];
  }
  cx /= static_cast<double>(n);
  cy /= static_cast<double>(n);
  cz /= static_cast<double>(n);
  double d = nx * cx + ny * cy + nz * cz;
  return {nx, ny, nz, d};
}

// Every point must actually lie on the fitted plane - tolerance scales with the face's own size.
PlaneResult plane_from_polygon(const std::vector<Point3>& points) {
  PlaneResult pr = newell_normal(points);
  double tol = std::max(polygon_span(points), 1.0) * 1e-6;
  for (const auto& p : points) {
    double dist = pr.nx * p[0] + pr.ny * p[1] + pr.nz * p[2] - pr.d;
    if (std::abs(dist) > tol) {
      throw SkpWriteError(
          "face points are not coplanar - openskp::create only supports planar faces "
          "(pass auto_triangulate=true in FaceOptions to fan-split a non-planar polygon instead)");
    }
  }
  return pr;
}

// Same fit/tolerance as plane_from_polygon, but returns a bool for "not coplanar" instead of
// raising - used by add_face's auto_triangulate to decide whether fan-triangulation is needed.
// Still raises for a collinear/degenerate input.
bool is_coplanar(const std::vector<Point3>& points) {
  PlaneResult pr = newell_normal(points);
  double tol = std::max(polygon_span(points), 1.0) * 1e-6;
  for (const auto& p : points) {
    if (std::abs(pr.nx * p[0] + pr.ny * p[1] + pr.nz * p[2] - pr.d) > tol) return false;
  }
  return true;
}

// ---------------------------------------------------------------------------------------------
// ArchiveWriter - write-side mirror of legacy._Archive's slot/class-ref bookkeeping.
// ---------------------------------------------------------------------------------------------

// Curve parameters shared by every edge a circle/arc's chain declares - see
// ArchiveWriter::write_arc_curve.
struct ArcCurveParams {
  Point3 center;
  Point3 normal;
  Point3 xaxis;
  double start_angle;
  double end_angle;
  double radius;
  int num_segments;
};

using AttributeDictList = std::vector<std::pair<std::string, AttributeDict>>;
using EdgeRegistry = std::map<std::pair<int, int>, std::pair<int, int>>;

struct EdgeChainResult {
  std::vector<int> edge_slots;
  std::vector<int> edge_senses;
  int new_entities = 0;
};

class ArchiveWriter {
 public:
  int next_slot;
  std::map<std::string, int> class_slot;
  std::uint64_t next_pid;
  ByteBuffer buf;

  // One CSkFont per file, serialized inline at the first dimension/text's font field (exactly as
  // SketchUp writes it) and re-used by back-ref afterwards - see write_dimension/write_text.
  std::optional<int> dim_font_slot_;

  explicit ArchiveWriter(int next_slot_, std::map<std::string, int> class_slot_ = {},
                         std::uint64_t next_pid_ = 1)
      : next_slot(next_slot_), class_slot(std::move(class_slot_)), next_pid(next_pid_) {}

  int alloc() { return next_slot++; }

  std::uint64_t alloc_pid() { return next_pid++; }

  // Declares (on first use) or short-class-refs (on repeat use) `class_name`, and always
  // allocates a fresh object slot for the value that follows. `slot == 0x7FFF` is deliberately
  // excluded from the short form (0x8000 | 0x7FFF == 0xFFFF, the new-class-declaration marker) -
  // see create.hpp's module docstring.
  int new_of_known_class(const std::string& class_name, std::optional<int> schema = std::nullopt) {
    auto it = class_slot.find(class_name);
    if (it == class_slot.end()) {
      if (!schema) throw SkpWriteError(class_name + " not yet declared and no schema given");
      append_u16(buf, 0xFFFF);
      append_u16(buf, static_cast<std::uint32_t>(*schema));
      append_u16(buf, static_cast<std::uint32_t>(class_name.size()));
      buf.insert(buf.end(), class_name.begin(), class_name.end());
      class_slot[class_name] = alloc();
      return alloc();
    }
    int slot = it->second;
    if (slot < 0x7FFF) {
      append_u16(buf, 0x8000u | static_cast<std::uint32_t>(slot));
    } else {
      append_u16(buf, 0x7FFF);
      append_u32(buf, 0x80000000u | static_cast<std::uint32_t>(slot));
    }
    return alloc();
  }

  void write_null() { append_u16(buf, 0); }

  // Same 0x7FFF exclusion as new_of_known_class, for the plain (no class-ref bit) case.
  void backref(int slot) {
    if (slot < 0x7FFF) {
      append_u16(buf, static_cast<std::uint32_t>(slot));
    } else {
      append_u16(buf, 0x7FFF);
      append_u32(buf, static_cast<std::uint32_t>(slot));
    }
  }

  ByteBuffer encode_pid(std::uint64_t pid) {
    std::uint8_t mask = 0;
    ByteBuffer pid_bytes;
    for (int bit = 0; bit < 8; ++bit) {
      std::uint8_t byte_val = static_cast<std::uint8_t>((pid >> (8 * bit)) & 0xFF);
      if (byte_val) {
        mask = static_cast<std::uint8_t>(mask | (1u << bit));
        pid_bytes.push_back(byte_val);
      }
    }
    ByteBuffer out;
    out.push_back(mask);
    out.insert(out.end(), pid_bytes.begin(), pid_bytes.end());
    return out;
  }

  void preamble(std::optional<std::uint64_t> pid = std::nullopt, bool real_attrs = false) {
    if (real_attrs) {
      // Ground truth: CComponentDefinition and CComponentInstance both reference a real (but
      // childless) CAttributeContainer here instead of the null pointer every other entity uses.
      append_u16(buf, 0x8000u | static_cast<std::uint32_t>(kAttrContainerSlot));
      alloc();  // a class-ref always allocates a new object slot, even a bookkeeping-only one
      append_bytes(buf, reinterpret_cast<const std::uint8_t*>("\0\0\0"), 3);
      append_u16(buf, 0);  // empty children-list terminator
    } else {
      write_null();  // no CAttributeContainer
    }
    std::uint64_t p = pid ? *pid : alloc_pid();
    auto enc = encode_pid(p);
    buf.insert(buf.end(), enc.begin(), enc.end());
  }

  void write_face_texture_coords(std::optional<Matrix3x3> front_matrix,
                                 std::optional<Matrix3x3> back_matrix);
  void write_attribute_dict(const std::string& dict_name, const AttributeDict& entries);

  // Like preamble(real_attrs=true), but the attribute container's children list holds real
  // content instead of closing immediately: an optional CFaceTextureCoords followed by zero or
  // more named CAttributeNamed dictionaries.
  void preamble_with_real_attrs(std::optional<Matrix3x3> front_matrix,
                                std::optional<Matrix3x3> back_matrix,
                                const AttributeDictList& attribute_dicts,
                                std::optional<std::uint64_t> pid = std::nullopt) {
    append_u16(buf, 0x8000u | static_cast<std::uint32_t>(kAttrContainerSlot));
    alloc();
    append_bytes(buf, reinterpret_cast<const std::uint8_t*>("\0\0\0"), 3);
    if (front_matrix || back_matrix) write_face_texture_coords(front_matrix, back_matrix);
    for (const auto& [dict_name, entries] : attribute_dicts)
      write_attribute_dict(dict_name, entries);
    write_null();  // children-list terminator
    std::uint64_t p = pid ? *pid : alloc_pid();
    auto enc = encode_pid(p);
    buf.insert(buf.end(), enc.begin(), enc.end());
  }

  void drawbase(int mat = 0, int layer = 0, bool hidden = false, bool soft = false,
                bool smooth = false) {
    ByteBuffer b(10, 0);
    write_u16_at(b, 0, static_cast<std::uint16_t>(mat));
    b[2] = hidden ? 1 : 0;
    // offsets 3-4: unused padding per the reader, but real SketchUp silently drops any entity
    // whose drawbase has them zeroed - must be 1, 1.
    b[3] = 1;
    b[4] = 1;
    b[5] = soft ? 1 : 0;
    b[6] = smooth ? 1 : 0;
    write_u16_at(b, 8, static_cast<std::uint16_t>(layer));
    buf.insert(buf.end(), b.begin(), b.end());
  }

  int write_vertex(Point3 point) {
    int slot = new_of_known_class("CVertex", 0);
    preamble();
    append_f64(buf, point[0]);
    append_f64(buf, point[1]);
    append_f64(buf, point[2]);
    return slot;
  }

  // Write one CArcCurve record and return its slot - the shared geometric-parameter object a
  // circle/arc's straight CEdge segments each carry a backref to. `xaxis` is the arc's own fixed
  // 0-angle reference direction (a unit vector times radius); start_angle/end_angle are offsets
  // from it. Two of the 14 stored values (ground truth offsets 11 and 13) were 0 in every sample
  // tested and are written as 0 here too.
  int write_arc_curve(Point3 center, Point3 normal, Point3 xaxis, double start_angle,
                      double end_angle, double radius, int num_segments) {
    if (num_segments < 0 || num_segments > 0xFF) {
      throw SkpWriteError("num_segments must be between 0 and 255, got " +
                          std::to_string(num_segments));
    }
    int slot = new_of_known_class("CArcCurve", kArcCurveSchema);
    preamble();
    buf.push_back(0);
    buf.push_back(static_cast<std::uint8_t>(num_segments));
    append_bytes(buf, reinterpret_cast<const std::uint8_t*>("\0\0\0"), 3);
    const double vals[14] = {center[0], center[1], center[2], normal[0], normal[1],
                             normal[2], xaxis[0],  xaxis[1],  xaxis[2],  start_angle,
                             end_angle, 0.0,       radius,    0.0};
    for (double v : vals) append_f64(buf, v);
    return slot;
  }

  // Write one CCurve record and return its slot - a freeform polyline curve grouping, as opposed
  // to CArcCurve's arc geometry. Ground truth: a constant 1-byte type tag followed by the edge
  // count as a u32.
  int write_curve(int num_edges) {
    int slot = new_of_known_class("CCurve", kCCurveSchema);
    preamble();
    buf.push_back(1);
    append_u32(buf, static_cast<std::uint32_t>(num_edges));
    return slot;
  }

  void write_str(const std::string& s) {
    std::u16string encoded = utf8_to_utf16(s);
    std::size_t n = encoded.size();
    if (n >= 0xFF) throw SkpWriteError("string too long to encode (255 char limit)");
    buf.push_back(0xFF);
    buf.push_back(0xFE);
    buf.push_back(0xFF);
    buf.push_back(static_cast<std::uint8_t>(n));
    for (char16_t c : encoded) {
      buf.push_back(static_cast<std::uint8_t>(c & 0xFF));
      buf.push_back(static_cast<std::uint8_t>((c >> 8) & 0xFF));
    }
  }

  // Write the first dimension/text's CSkFont record inline, or back-ref the one already written
  // earlier in this file - shared 1-per-file state, same as create.py's own `_dim_font_slot`.
  void write_dim_font_ref() {
    if (!dim_font_slot_) {
      dim_font_slot_ = new_of_known_class("CSkFont", kSkFontSchema);
      buf.insert(buf.end(), kDimFontPayload.begin(), kDimFontPayload.end());
    } else {
      backref(*dim_font_slot_);
    }
  }

  // Add a FREE linear dimension between two explicit points (inches, world space). `offset` is
  // the dimension line's offset from the measured segment, in inches (signed).
  //
  // The record layout is the byte-exact one the real SketchUp SDK writes for free dimensions
  // (generated via SketchUpAPI and harvested - see docs/dimension-record-notes.md): connection
  // type 1 with the point stored inline in each connection block and null object refs. Free
  // dimensions render in any orientation; anchored (type 2) dimensions are a future refinement.
  void write_dimension(Point3 p1, Point3 p2, double offset = 10.0) {
    if (p1 == p2) throw SkpWriteError("add_dimension endpoints coincide");
    new_of_known_class("CDimensionLinear", kDimensionLinearSchema);
    preamble();
    drawbase();
    write_str("");  // auto-computed measurement text
    write_dim_font_ref();
    // connection 1: [u8 0][u32 0][u32 type=1][u32 4][point A], null ref
    append_zeros(buf, 5);
    append_u32(buf, 1);
    append_u32(buf, 4);
    append_f64(buf, p1[0]);
    append_f64(buf, p1[1]);
    append_f64(buf, p1[2]);
    append_u16(buf, 0);
    // connection 2: [u16 0][f64 0][u32 type=1][u32 4][point B], null ref
    append_zeros(buf, 10);
    append_u32(buf, 1);
    append_u32(buf, 4);
    append_f64(buf, p2[0]);
    append_f64(buf, p2[1]);
    append_f64(buf, p2[2]);
    append_u16(buf, 0);
    // placement block: SDK free-dimension defaults + our offset
    append_zeros(buf, 2);
    for (double v : {0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0}) append_f64(buf, v);
    append_u32(buf, 0);
    append_f64(buf, offset);
    append_f64(buf, 0.0);
    append_u32(buf, 1);
  }

  // Add a leader text (SketchUp's Text tool) anchored at `point` (inches, world space), with the
  // label floating at `point + leader` and a leader line joining them.
  //
  // The record mirrors human-drawn leader texts harvested from real files (the SDK's own create
  // only produces SCREEN texts - its two 0.5 doubles are screen fractions): screen slot zeroed,
  // the free-connection block dimensions use ([u32 1][u32 4][point3d]), the label's world
  // position in the placement tail, and leader type 2 (pushpin) before the arrow delimiter.
  void write_text(const std::string& text, Point3 point, Point3 leader = {15.0, 15.0, 15.0}) {
    Point3 lb = {point[0] + leader[0], point[1] + leader[1], point[2] + leader[2]};
    new_of_known_class("CText", kTextSchema);
    preamble();
    drawbase();
    write_dim_font_ref();
    append_f64(buf, 0.0);
    append_f64(buf, 0.0);  // screen-fraction slot (unused)
    append_u32(buf, 1);
    append_u32(buf, 4);  // free connection + constant
    append_f64(buf, point[0]);
    append_f64(buf, point[1]);
    append_f64(buf, point[2]);
    append_zeros(buf, 12);
    append_f64(buf, lb[0]);
    append_f64(buf, lb[1]);
    append_f64(buf, lb[2]);  // label position
    append_zeros(buf, 16);
    append_f64(buf, 1.0);
    append_u32(buf, 2);  // leader type: pushpin
    buf.insert(buf.end(), kTextDelim.begin(), kTextDelim.end());
    write_str(text);
    append_zeros(buf, 5);
  }

  // Add a construction/guide line (SketchUp's Construction Line tool). Pass exactly one of
  // `point2` (a bounded segment between `point` and `point2`) or `direction` (an unbounded guide
  // line through `point`).
  //
  // Ground truth (real SketchUp 2025, SDK/Ruby cross-checked against both a v2020-downgrade save
  // and a genuinely v17-native save): the record stores a point + normalized direction + two
  // signed distance parameters along that direction marking the visible segment's start/end. An
  // unbounded direction is written as the real +/-1e30 sentinel SketchUp itself uses.
  void write_construction_line(Point3 point, std::optional<Point3> point2 = std::nullopt,
                               std::optional<Point3> direction = std::nullopt) {
    if (point2.has_value() == direction.has_value()) {
      throw SkpWriteError("add_construction_line: pass exactly one of point2 or direction");
    }
    double dx, dy, dz, start_param, end_param;
    if (point2) {
      dx = (*point2)[0] - point[0];
      dy = (*point2)[1] - point[1];
      dz = (*point2)[2] - point[2];
      double length = std::sqrt(dx * dx + dy * dy + dz * dz);
      if (length == 0.0) throw SkpWriteError("add_construction_line: point and point2 coincide");
      dx /= length;
      dy /= length;
      dz /= length;
      start_param = 0.0;
      end_param = length;
    } else {
      const Point3& d = *direction;
      double dlen = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
      if (dlen == 0.0) throw SkpWriteError("add_construction_line: direction must be nonzero");
      dx = d[0] / dlen;
      dy = d[1] / dlen;
      dz = d[2] / dlen;
      start_param = -kClineInfinite;
      end_param = kClineInfinite;
    }
    new_of_known_class("CConstructionLine", kConstructionLineSchema);
    preamble();
    drawbase();
    for (double v : {point[0], point[1], point[2], dx, dy, dz, start_param, end_param})
      append_f64(buf, v);
    append_zeros(buf, 4);  // trailer
  }

  // Add a construction/guide point (SketchUp's Construction Point tool) at `position` (inches,
  // world space).
  //
  // Ground truth (real SketchUp 2025, SDK/Ruby cross-checked): a second, always-zero 3-double
  // block and a trailing zero byte follow the position - reserved/unused, written as zero to
  // match every real-file sample seen.
  void write_construction_point(Point3 position) {
    new_of_known_class("CConstructionPoint", kConstructionPointSchema);
    preamble();
    drawbase();
    for (double v : {position[0], position[1], position[2], 0.0, 0.0, 0.0}) append_f64(buf, v);
    append_zeros(buf, 1);
  }

  // Add a section plane (SketchUp's Section Plane tool) through `point` with the given `normal`
  // (need not be unit length), matching Entities#add_section_plane([point, normal]).
  //
  // Ground truth (real SketchUp 2025, SDK/Ruby cross-checked against a genuinely v17-native
  // save): the record is preamble + drawbase + the plane as 4 doubles (a, b, c, d) satisfying
  // a*x + b*y + c*z + d = 0 for every point on the plane - the same implicit form
  // Sketchup::SectionPlane#get_plane returns (normal normalized, d = -(normal . point)). A
  // name/short-label pair can follow on v18+ saves per the reader (legacy.cpp's
  // read_section_plane) - omitted here since this writer only ever produces v17-tagged files,
  // same scope as write_dimension/write_text/write_construction_line.
  void write_section_plane(Point3 point, Point3 normal) {
    double nlen = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (nlen == 0.0) throw SkpWriteError("add_section_plane: normal must be nonzero");
    double a = normal[0] / nlen, b = normal[1] / nlen, c = normal[2] / nlen;
    double d = -(a * point[0] + b * point[1] + c * point[2]);
    new_of_known_class("CSectionPlane", kSectionPlaneSchema);
    preamble();
    drawbase();
    append_f64(buf, a);
    append_f64(buf, b);
    append_f64(buf, c);
    append_f64(buf, d);
  }

  int write_material(const std::string& name, Color4 rgba,
                     std::optional<double> opacity = std::nullopt) {
    int slot = new_of_known_class("CMaterial", kMaterialSchema);
    preamble();
    write_str(name);
    append_u16(buf, 0);  // texflag: solid color, no texture
    buf.insert(buf.end(), rgba.begin(), rgba.end());
    write_str("");  // texture path (empty - no texture)
    buf.insert(buf.end(), 8, 0);
    // Stored TRANSPARENCY (0 = opaque); see write_textured_material.
    append_f64(buf, opacity.has_value() ? 1.0 - *opacity : 1.0);
    buf.push_back(opacity.has_value() ? 1 : 0);  // use_opacity gates it
    return slot;
  }

  // `subtype` is CDib's image format tag (4 for PNG, 1 for JPEG - see detect_image_subtype).
  //
  // `applied_width`/`applied_height` both default to 1.0. Pass the material's real-world tile
  // size for a textured material used with default (unpositioned) projection, to make the
  // texture repeat at a specific size instead of every 1 inch (real SketchUp writes the
  // material's own size here - a file authored in SketchUp Web carries 8.0 x 16.0 for a brick) -
  // the reader's own ground-truth-derived UV formula divides a face's final UV by the material's
  // applied width/height, for a default-projected face exactly as much as a positioned
  // (front_uv/back_uv) one. Until 2026-08-28 this defaulted to a corrupted sentinel byte pattern
  // instead (see kTextureHSentinel's own comment) - confirmed via real SketchUp screenshots to
  // render as a streaky, vertically-smeared texture regardless of projection mode.
  //
  // `applied_width` and `opacity` are appended after `applied_height` (rather than sitting next
  // to it) so an existing positional call passing `applied_height` as the 5th argument keeps
  // meaning what it always meant.
  int write_textured_material(const std::string& name, const ByteBuffer& image_bytes,
                              const std::string& texture_path, int subtype,
                              std::optional<double> applied_height = std::nullopt,
                              std::optional<double> applied_width = std::nullopt,
                              std::optional<double> opacity = std::nullopt) {
    int slot = new_of_known_class("CMaterial", kMaterialSchema);
    preamble();
    write_str(name);
    append_u16(buf, 1);  // texflag: textured
    buf.insert(buf.end(), 2, 0);
    new_of_known_class("CDib", kDibSchema);
    append_u32(buf, static_cast<std::uint32_t>(subtype));
    append_u32(buf, static_cast<std::uint32_t>(image_bytes.size()));
    buf.insert(buf.end(), image_bytes.begin(), image_bytes.end());
    if (subtype == 1) {
      // JPEG only: ground-truth confirmed constant 90 regardless of the source JPEG's own actual
      // encoded quality.
      append_u32(buf, 90);
    }
    // Applied size: how much MODEL SPACE one tile of the image covers, in inches - two plain
    // f64s. For a texture applied WITHOUT positioning it is the only thing that says how big the
    // image is - such faces carry no per-face UV record at all, so a wrong size here is the whole
    // mapping wrong.
    append_f64(buf, applied_width.value_or(1.0));
    append_f64(buf, applied_height.value_or(1.0));
    write_str(texture_path);
    // avg color: neutral near-opaque white. Alpha is 254, not fully-opaque 255 - the reader
    // treats alpha=255 here as one of its two "this material is colorized" signals; a plain
    // texture's placeholder must not trip that.
    const std::uint8_t avg[9] = {255, 255, 255, 254, 0, 255, 255, 255, 254};
    append_bytes(buf, avg, sizeof(avg));
    write_str("");  // second name field - empty in ground truth
    append_u32(buf, 1);
    append_u32(buf, 0);  // blob (colorize-related, ground truth: 1, 0)
    // Opacity, and the u8 that GATES it. The stored f64 is TRANSPARENCY (0 = opaque) - the reader
    // turns it into the opacity factor exposed with `1.0 - stored`, and only when the flag is
    // set - so an `opacity` argument here is written inverted and round-trips as itself.
    // Hardcoding 1.0/false meant a translucent material came out solid: a pool's water at 0.6
    // exported as an opaque slab.
    append_f64(buf, opacity.has_value() ? 1.0 - *opacity : 1.0);
    buf.push_back(opacity.has_value() ? 1 : 0);
    return slot;
  }

  // Ground truth shows each top-level layer record contains a second, embedded pid after the
  // visible name, so each layer consumes 2 pids, not 1. `with_pids=false` (used only for the
  // layer a component definition embeds internally) omits both.
  int write_layer(const std::string& name, bool with_pids = true, bool hidden = false,
                  std::optional<Color4> rgba = std::nullopt) {
    int slot = new_of_known_class("CLayer", kLayerSchema);
    preamble(with_pids ? std::optional<std::uint64_t>(std::nullopt)
                       : std::optional<std::uint64_t>(0));
    write_str(name);
    std::uint64_t pid2 = with_pids ? alloc_pid() : 0;
    buf.push_back(hidden ? 1 : 0);
    buf.push_back(0);
    buf.push_back(0);
    auto enc = encode_pid(pid2);
    buf.insert(buf.end(), enc.begin(), enc.end());
    write_str("Layer_" + name);
    append_u16(buf, 256);  // ground truth is a constant 256 here
    if (rgba) {
      buf.insert(buf.end(), rgba->begin(), rgba->end());
    } else {
      buf.insert(buf.end(), 4, 0);
    }
    write_str("");  // second name field - empty in ground truth
    buf.insert(buf.end(), 8, 0);
    append_f64(buf, 0.5);  // 21-byte tail, opacity-like f64=0.5
    buf.insert(buf.end(), 5, 0);
    return slot;
  }

  void write_thumbnail() {
    new_of_known_class("CThumbnail", kThumbnailSchema);
    preamble(0);  // structural container: ground truth carries no pid
    append_u16(buf, 0x8000u | static_cast<std::uint32_t>(kCCameraSlot));
    alloc();
    append_bytes(buf, kCameraTemplate, sizeof(kCameraTemplate));
    write_null();  // no thumbnail image
  }

  // Begin a CComponentDefinition record - everything up to (not including) its internal entity
  // list. Returns (definition_slot, count_patch_pos): the caller writes the definition's
  // geometry, then patches a u32 entity count at count_patch_pos and calls
  // write_definition_tail to close it out.
  std::pair<int, std::size_t> write_definition_header(const AttributeDictList& attribute_dicts) {
    int slot = new_of_known_class("CComponentDefinition", kDefinitionSchema);
    if (!attribute_dicts.empty()) {
      preamble_with_real_attrs(std::nullopt, std::nullopt, attribute_dicts);
    } else {
      preamble(std::nullopt, true);  // ground truth: a real pid and a real (empty) attr container
    }
    append_bytes(buf, kDefinitionBaseBlock, sizeof(kDefinitionBaseBlock));
    append_u32(buf, 1);  // nlayers: always 1, an embedded copy of Layer0
    int embedded_layer_slot = write_layer("Layer0", false);
    backref(embedded_layer_slot);  // "decl": this definition's own active layer
    // Distinct from nested instances (below, like any other entity): ground truth shows this
    // counts CComponentDefinition classes declared inline within this definition's own header -
    // this writer always declares definitions at the top level, so this stays 0.
    append_u32(buf, 0);
    std::size_t count_patch_pos = buf.size();
    append_u32(buf, 0);  // placeholder entity count, patched by the caller
    return {slot, count_patch_pos};
  }

  // Close out a CComponentDefinition record: relationship count, GUID, name, timestamp,
  // behavior flags, and a default thumbnail.
  void write_definition_tail(const std::string& name) {
    append_u32(buf, 0);  // nrel: CRelationship count - always 0, not supported
    append_u16(buf, 0);
    auto guid = random_uuid_bytes();
    buf.insert(buf.end(), guid.begin(), guid.end());
    write_str(name);
    write_str("");  // description - empty in ground truth
    write_str("");  // second name field - empty in ground truth
    append_u32(buf, static_cast<std::uint32_t>(std::time(nullptr)));
    // 43-byte gap; byte -9 carries the always-faces-camera/shadows-face-sun behavior flags, both
    // left off (matching neither being exposed by this writer yet).
    buf.insert(buf.end(), 43, 0);
    write_thumbnail();
  }

  void write_instance_like(const std::string& class_name, int schema, bool real_attrs,
                           int definition_slot, const std::string& name, Point3 translation,
                           std::optional<Matrix3x3> matrix3x3, int mat, int layer,
                           const AttributeDictList& attribute_dicts, bool hidden) {
    new_of_known_class(class_name, schema);
    if (real_attrs && !attribute_dicts.empty()) {
      preamble_with_real_attrs(std::nullopt, std::nullopt, attribute_dicts);
    } else {
      preamble(std::nullopt, real_attrs);
    }
    drawbase(mat, layer, hidden);
    backref(definition_slot);
    Matrix3x3 m = matrix3x3.value_or(Matrix3x3{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0});
    for (double v : m) append_f64(buf, v);
    for (double v : translation) append_f64(buf, v);
    append_f64(buf, 1.0);
    write_str(name);
    auto guid = random_uuid_bytes();
    buf.insert(buf.end(), guid.begin(), guid.end());
  }

  // Places a copy of `definition_slot`; returns how many new root-entity-list slots it consumed
  // (always 1).
  int write_instance(int definition_slot, const std::string& name, Point3 translation,
                     std::optional<Matrix3x3> matrix3x3, int instance_material, int instance_layer,
                     const AttributeDictList& attribute_dicts, bool hidden) {
    // ground truth: instances also carry a real (empty) attr container, unlike CGroup
    write_instance_like("CComponentInstance", kInstanceSchema, true, definition_slot, name,
                        translation, matrix3x3, instance_material, instance_layer, attribute_dicts,
                        hidden);
    return 1;
  }

  // A group is structurally almost identical to a component instance - the two real differences
  // are its class name/schema and its attribute pointer: unlike CComponentInstance (which always
  // carries a real, if often empty, CAttributeContainer regardless of whether attributes are
  // given), a group only gets one when attribute_dicts is actually given - matching write_face's
  // conditional pattern instead. A real production Group WITH attributes (SketchUp 2020 export,
  // ground truth) carries a genuine CAttributeContainer at this exact schema; a never-attributed
  // group still correctly gets a null pointer either way (openskp#261).
  int write_group(int definition_slot, const std::string& name, Point3 translation,
                  std::optional<Matrix3x3> matrix3x3, int group_material, int group_layer,
                  const AttributeDictList& attribute_dicts, bool hidden) {
    write_instance_like("CGroup", kGroupSchema, !attribute_dicts.empty(), definition_slot, name,
                        translation, matrix3x3, group_material, group_layer, attribute_dicts,
                        hidden);
    return 1;
  }

  // Places definition_slot (the quad + texture material add_image built for it); return contract
  // matches write_instance/write_group (always 1).
  //
  // legacy.cpp's image reader treats CImage as "instance-shaped": preamble, drawbase, a
  // definition back-ref, a 3x4 placement, a constant 1.0, a source-path string, and a 16-byte
  // GUID - field-for-field identical in count and order to write_instance's own
  // matrix3x3(9)+translation(3)+1.0(1)=13 f64s, name string, GUID. The source-path string is
  // always empty - ground truth shows real SketchUp writes it empty too. No material argument -
  // an Image entity isn't painted a material the way a face or instance can be; its appearance
  // comes entirely from the definition's own textured face.
  int write_image(int definition_slot, Point3 translation, std::optional<Matrix3x3> matrix3x3,
                  int image_layer, bool hidden) {
    write_instance_like("CImage", kImageSchema, false, definition_slot, "", translation, matrix3x3,
                        0, image_layer, {}, hidden);
    return 1;
  }

  // Write a chain of straight CEdge records connecting `points` in order, sharing
  // vertices/edges via `vertex_slots`/`edge_registry`. `closed=true` also connects the last
  // point back to the first. At most one of curve_params/polyline_num_edges should be given -
  // both describe the same first-use-inline-declaration pattern (the shared curve object is
  // declared inline as the FIRST newly-declared edge's own "curve" field).
  EdgeChainResult write_edge_chain(const std::vector<Point3>& points,
                                   std::map<Point3, int>& vertex_slots, EdgeRegistry& edge_registry,
                                   bool closed, bool hidden_edges, bool soft_edges,
                                   bool smooth_edges, std::optional<ArcCurveParams> curve_params,
                                   std::optional<int> polyline_num_edges) {
    int n = static_cast<int>(points.size());
    int pair_count = closed ? n : n - 1;
    std::vector<std::optional<int>> point_slots(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
      auto it = vertex_slots.find(points[static_cast<std::size_t>(i)]);
      if (it != vertex_slots.end()) point_slots[static_cast<std::size_t>(i)] = it->second;
    }
    EdgeChainResult result;
    std::optional<int> curve_slot;

    for (int i = 0; i < pair_count; ++i) {
      int v1_idx = i, v2_idx = (i + 1) % n;
      auto v1_known = point_slots[static_cast<std::size_t>(v1_idx)];
      auto v2_known = point_slots[static_cast<std::size_t>(v2_idx)];
      if (v1_known && v2_known) {
        std::pair<int, int> key = std::minmax(*v1_known, *v2_known);
        auto it = edge_registry.find(key);
        if (it != edge_registry.end()) {
          int edge_slot = it->second.first;
          int fwd_v1 = it->second.second;
          result.edge_slots.push_back(edge_slot);
          result.edge_senses.push_back(fwd_v1 == *v1_known ? 0 : 1);
          continue;
        }
      }

      int edge_slot = new_of_known_class("CEdge", 2);
      preamble();
      drawbase(0, 0, hidden_edges, soft_edges, smooth_edges);
      for (int idx : {v1_idx, v2_idx}) {
        auto& slot = point_slots[static_cast<std::size_t>(idx)];
        if (!slot) {
          slot = write_vertex(points[static_cast<std::size_t>(idx)]);
          vertex_slots[points[static_cast<std::size_t>(idx)]] = *slot;
        } else {
          backref(*slot);
        }
      }
      if (curve_slot) {
        backref(*curve_slot);
      } else if (curve_params) {
        curve_slot =
            write_arc_curve(curve_params->center, curve_params->normal, curve_params->xaxis,
                            curve_params->start_angle, curve_params->end_angle,
                            curve_params->radius, curve_params->num_segments);
      } else if (polyline_num_edges) {
        curve_slot = write_curve(*polyline_num_edges);
      } else {
        write_null();  // curve = None
      }
      result.edge_slots.push_back(edge_slot);
      result.edge_senses.push_back(0);
      result.new_entities += 1;
      std::pair<int, int> key2 = std::minmax(*point_slots[static_cast<std::size_t>(v1_idx)],
                                             *point_slots[static_cast<std::size_t>(v2_idx)]);
      edge_registry[key2] = {edge_slot, *point_slots[static_cast<std::size_t>(v1_idx)]};
    }
    return result;
  }

  // Write a partial (open) arc as a chain of straight CEdge records - no face. Returns how many
  // new root-entity-list slots were consumed.
  int write_arc(const std::vector<Point3>& points, std::map<Point3, int>& vertex_slots,
                EdgeRegistry& edge_registry, const ArcCurveParams& curve_params, bool hidden_edges,
                bool soft_edges, bool smooth_edges) {
    auto res = write_edge_chain(points, vertex_slots, edge_registry, false, hidden_edges,
                                soft_edges, smooth_edges, curve_params, std::nullopt);
    return res.new_entities;
  }

  // Write a freeform polyline curve - a chain of straight CEdge records all sharing one CCurve
  // grouping, no face. Returns how many new root-entity-list slots were consumed.
  int write_polyline(const std::vector<Point3>& points, std::map<Point3, int>& vertex_slots,
                     EdgeRegistry& edge_registry, bool closed, bool hidden_edges, bool soft_edges,
                     bool smooth_edges) {
    int n = static_cast<int>(points.size());
    int pair_count = closed ? n : n - 1;
    auto res = write_edge_chain(points, vertex_slots, edge_registry, closed, hidden_edges,
                                soft_edges, smooth_edges, std::nullopt, pair_count);
    return res.new_entities;
  }

  // Write one planar face and return how many new root-entity-list slots it consumed (edges
  // newly declared, plus the face itself, plus every hole's own newly-declared edges).
  int write_face(const std::vector<Point3>& points, std::map<Point3, int>& vertex_slots,
                 EdgeRegistry& edge_registry, int face_material, int face_layer, int back_material,
                 bool hidden, bool soft_edges, bool smooth_edges, bool hidden_edges,
                 const std::optional<UvCorrespondence>& front_uv,
                 const std::optional<UvCorrespondence>& back_uv,
                 const AttributeDictList& attribute_dicts,
                 std::optional<ArcCurveParams> curve_params = std::nullopt,
                 const std::vector<std::vector<Point3>>& holes = {}) {
    // Validate everything that CAN fail before writing a single byte or touching
    // vertex_slots/edge_registry - write_edge_chain mutates both this writer's own buffer AND
    // those caller-owned, shared-across-calls containers as it goes, with no rollback.
    PlaneResult plane = plane_from_polygon(points);
    std::optional<Matrix3x3> front_matrix, back_matrix;
    if (front_uv)
      front_matrix = uv_matrix_for_face(points, *front_uv, {plane.nx, plane.ny, plane.nz});
    if (back_uv) back_matrix = uv_matrix_for_face(points, *back_uv, {plane.nx, plane.ny, plane.nz});
    double tol = std::max(polygon_span(points), 1.0) * 1e-6;
    for (const auto& hole : holes) {
      if (hole.size() < 3) throw SkpWriteError("a hole needs at least 3 points");
      for (const auto& p : hole) {
        double dist = plane.nx * p[0] + plane.ny * p[1] + plane.nz * p[2] - plane.d;
        if (std::abs(dist) > tol) {
          throw SkpWriteError(
              "a hole point is off the face's own plane - a hole must lie on the same plane as "
              "the outer boundary");
        }
      }
    }

    auto chain = write_edge_chain(points, vertex_slots, edge_registry, true, hidden_edges,
                                  soft_edges, smooth_edges, curve_params, std::nullopt);
    int new_entities = chain.new_entities;
    std::vector<std::pair<std::vector<int>, std::vector<int>>> hole_loops;
    for (const auto& hole : holes) {
      auto h_chain = write_edge_chain(hole, vertex_slots, edge_registry, true, hidden_edges,
                                      soft_edges, smooth_edges, std::nullopt, std::nullopt);
      hole_loops.emplace_back(h_chain.edge_slots, h_chain.edge_senses);
      new_entities += h_chain.new_entities;
    }

    new_of_known_class("CFace", 3);
    if (front_uv || back_uv || !attribute_dicts.empty()) {
      preamble_with_real_attrs(front_matrix, back_matrix, attribute_dicts);
    } else {
      preamble();
    }
    drawbase(face_material, face_layer, hidden);
    append_f64(buf, plane.nx);
    append_f64(buf, plane.ny);
    append_f64(buf, plane.nz);
    append_f64(buf, plane.d);
    append_u32(buf, static_cast<std::uint32_t>(1 + holes.size()));  // nloops

    int loop_slot = new_of_known_class("CLoop", 1);
    preamble(0);  // structural object: ground truth uses pid 0
    buf.push_back(1);
    buf.push_back(1);

    for (std::size_t i = 0; i < chain.edge_slots.size(); ++i) {
      new_of_known_class("CEdgeUse", 1);
      preamble(0);
      backref(chain.edge_slots[i]);
      buf.push_back(static_cast<std::uint8_t>(chain.edge_senses[i]));
      backref(loop_slot);
    }
    write_null();  // loop terminator

    for (const auto& [h_edge_slots, h_edge_senses] : hole_loops) {
      int h_loop_slot = new_of_known_class("CLoop", 1);
      preamble(0);
      buf.push_back(0);  // ground truth: 0 marks a hole loop, not the boundary
      buf.push_back(1);
      for (std::size_t i = 0; i < h_edge_slots.size(); ++i) {
        new_of_known_class("CEdgeUse", 1);
        preamble(0);
        backref(h_edge_slots[i]);
        buf.push_back(static_cast<std::uint8_t>(h_edge_senses[i]));
        backref(h_loop_slot);
      }
      write_null();
    }

    append_u16(buf, static_cast<std::uint16_t>(back_material));
    new_entities += 1;  // the face itself
    return new_entities;
  }
};

void ArchiveWriter::write_face_texture_coords(std::optional<Matrix3x3> front_matrix,
                                              std::optional<Matrix3x3> back_matrix) {
  new_of_known_class("CFaceTextureCoords", kFtcSchema);
  preamble(0);
  append_u32(buf, 0);
  std::array<double, 24> ks{};
  Matrix3x3 fm = front_matrix.value_or(kIdentityUvMatrix);
  Matrix3x3 bm = back_matrix.value_or(kIdentityUvMatrix);
  for (int i = 0; i < 9; ++i) ks[static_cast<std::size_t>(i)] = fm[static_cast<std::size_t>(i)];
  for (int i = 0; i < 9; ++i)
    ks[static_cast<std::size_t>(12 + i)] = bm[static_cast<std::size_t>(i)];
  for (double v : ks) append_f64(buf, v);
  append_u32(buf, 0);  // front pin count - this writer always emits a solved matrix
  append_u32(buf, 0);  // back pin count
  append_u32(buf, front_matrix ? 1 : 0);  // fflags bit 0: front painted/positioned
  append_u32(buf, back_matrix ? 1 : 0);   // bflags bit 0: back painted/positioned
}

void ArchiveWriter::write_attribute_dict(const std::string& dict_name,
                                         const AttributeDict& entries) {
  // Unlike every other class this project declares, CAttributeNamed is already pre-declared in
  // the scaffold's own prefix - always a short class-ref, never a fresh 0xFFFF declaration.
  append_u16(buf, 0x8000u | static_cast<std::uint32_t>(kAttributeNamedSlot));
  alloc();
  append_bytes(buf, reinterpret_cast<const std::uint8_t*>("\0\0\0"),
               3);     // null attrs (2) + mask=0 (1), pid=0
  append_u32(buf, 0);  // ground truth: read and discarded by the reader too
  write_str(dict_name);
  // AttributeValue's 3 alternatives (string/int32/double) are exactly the 3 types this writer
  // supports, and std::int32_t is already range-bounded by its type - so unlike Python's
  // runtime _validate_attribute_entries, no extra validation is needed here.
  for (const auto& [key, value] : entries) {
    write_str(key);
    if (const auto* s = std::get_if<std::string>(&value)) {
      buf.push_back(kAttrTypeString);
      write_str(*s);
    } else if (const auto* i = std::get_if<std::int32_t>(&value)) {
      buf.push_back(kAttrTypeInt32);
      append_i32(buf, *i);
    } else {
      buf.push_back(kAttrTypeDouble);
      append_f64(buf, std::get<double>(value));
    }
  }
  write_str("");  // empty-key terminator
  append_u32(buf, 0);
}

// ---------------------------------------------------------------------------------------------
// Shared face-writing helper (SkpBuilder::add_face and ComponentDefinitionBuilder::add_face).
// ---------------------------------------------------------------------------------------------

int write_face_or_triangulate(ArchiveWriter& writer, const std::vector<Point3>& points,
                              std::map<Point3, int>& vertex_slots, EdgeRegistry& edge_registry,
                              int material, int layer, int back_material, bool hidden,
                              bool soft_edges, bool smooth_edges, bool hidden_edges,
                              const std::optional<UvCorrespondence>& front_uv,
                              const std::optional<UvCorrespondence>& back_uv,
                              const AttributeDictList& attribute_dicts, bool auto_triangulate,
                              const std::vector<std::vector<Point3>>& holes) {
  if (!holes.empty() || !auto_triangulate || points.size() == 3 || is_coplanar(points)) {
    return writer.write_face(points, vertex_slots, edge_registry, material, layer, back_material,
                             hidden, soft_edges, smooth_edges, hidden_edges, front_uv, back_uv,
                             attribute_dicts, std::nullopt, holes);
  }
  if (front_uv || back_uv) {
    throw SkpWriteError("auto_triangulate cannot be combined with front_uv/back_uv positioning");
  }
  int total = 0;
  for (std::size_t i = 1; i + 1 < points.size(); ++i) {
    total += writer.write_face({points[0], points[i], points[i + 1]}, vertex_slots, edge_registry,
                               material, layer, back_material, hidden, soft_edges, smooth_edges,
                               hidden_edges, std::nullopt, std::nullopt, attribute_dicts,
                               std::nullopt, {});
  }
  return total;
}

int do_add_circle(ArchiveWriter& writer, std::map<Point3, int>& vertex_slots,
                  EdgeRegistry& edge_registry, Point3 center, Point3 normal, double radius,
                  const CircleOptions& options) {
  if (options.num_segments < 3 || options.num_segments > 255) {
    throw SkpWriteError("num_segments must be between 3 and 255, got " +
                        std::to_string(options.num_segments));
  }
  Point3 n = normalize3(normal);
  auto [u, w] = circle_basis(n);
  Point3 xaxis = {radius * u[0], radius * u[1], radius * u[2]};
  ArcCurveParams curve_params{center, n, xaxis, 0.0, 2.0 * kPi, radius, options.num_segments};
  auto points = circle_points(center, n, radius, options.num_segments, u, w);
  AttributeDictList dicts;
  if (!options.attributes.empty())
    dicts.emplace_back(options.attribute_dict_name, options.attributes);
  return writer.write_face(points, vertex_slots, edge_registry, options.material.value_or(0),
                           options.layer.value_or(0), options.back_material.value_or(0),
                           options.hidden, false, false, false, options.front_uv, options.back_uv,
                           dicts, curve_params, {});
}

int do_add_arc(ArchiveWriter& writer, std::map<Point3, int>& vertex_slots,
               EdgeRegistry& edge_registry, Point3 center, Point3 normal, double radius,
               double start_angle, double end_angle, const ArcOptions& options) {
  if (options.num_segments < 3 || options.num_segments > 255) {
    throw SkpWriteError("num_segments must be between 3 and 255, got " +
                        std::to_string(options.num_segments));
  }
  if (end_angle == start_angle) {
    throw SkpWriteError("start_angle and end_angle must differ - use add_circle for a full circle");
  }
  Point3 n = normalize3(normal);
  auto [u, w] = circle_basis(n);
  Point3 xaxis = {radius * u[0], radius * u[1], radius * u[2]};
  ArcCurveParams curve_params{
      center, n, xaxis, start_angle, end_angle, radius, options.num_segments};
  auto points = arc_points(center, n, radius, options.num_segments, u, w, start_angle, end_angle);
  return writer.write_arc(points, vertex_slots, edge_registry, curve_params, options.hidden_edges,
                          options.soft_edges, options.smooth_edges);
}

int do_add_polyline(ArchiveWriter& writer, std::map<Point3, int>& vertex_slots,
                    EdgeRegistry& edge_registry, const std::vector<Point3>& points,
                    const PolylineOptions& options) {
  if (points.size() < 2) throw SkpWriteError("a polyline needs at least 2 points");
  return writer.write_polyline(points, vertex_slots, edge_registry, options.closed,
                               options.hidden_edges, options.soft_edges, options.smooth_edges);
}

// Reject a material/back_material option that isn't a handle `skp`'s own add_material()/
// add_texture_material() actually returned. Without this, a stray value - most commonly a layer
// handle passed to the wrong option by mistake - gets written straight into the file as a
// material reference: this project's own reader tolerates the dangling reference silently, but
// real SketchUp rejects the whole file as corrupt on open, with no indication of which call
// caused it.
void check_material_handle(const SkpBuilder& skp, const std::optional<int>& value,
                           const std::string& param) {
  if (!value || *value == 0) return;
  for (const auto& [name, slot] : skp.materials_by_name) {
    if (slot == *value) return;
  }
  throw SkpWriteError(
      param + "=" + std::to_string(*value) +
      " is not a handle this builder's add_material()/add_texture_material() returned - "
      "passing an unrelated value (e.g. a layer handle by mistake) would silently write an "
      "invalid material reference that real SketchUp rejects on open");
}

// Reject a layer option that isn't a handle `skp`'s own add_layer() actually returned - see
// check_material_handle for why this matters.
void check_layer_handle(const SkpBuilder& skp, const std::optional<int>& value,
                        const std::string& param = "layer") {
  if (!value || *value == 0) return;
  for (const auto& [name, slot] : skp.layers_by_name) {
    if (slot == *value) return;
  }
  throw SkpWriteError(
      param + "=" + std::to_string(*value) +
      " is not a handle this builder's add_layer() returned - passing an unrelated value "
      "(e.g. a material handle by mistake) would silently write an invalid layer reference "
      "that real SketchUp rejects on open");
}

}  // namespace
}  // namespace detail

// ===============================================================================================
// ComponentDefinitionBuilder::Impl / SkpBuilder::Impl
//
// Both Impl structs are defined here, back to back, before any method body of either public
// class - SkpBuilder::Impl::pending_groups needs ComponentDefinitionBuilder::Impl::GroupPlacement
// to be a complete type, and ComponentDefinitionBuilder::close() (defined further down) needs
// SkpBuilder::Impl to be a complete type to reach into a sibling SkpBuilder's bookkeeping.
// ===============================================================================================

struct ComponentDefinitionBuilder::Impl {
  SkpBuilder* skp = nullptr;
  int slot = 0;
  std::string name;
  std::size_t count_patch_pos = 0;
  detail::ArchiveWriter* writer = nullptr;
  std::map<Point3, int> vertex_slots;
  detail::EdgeRegistry edge_registry;
  int new_entity_count = 0;
  bool closed = false;

  struct GroupPlacement {
    Point3 translation;
    std::optional<Matrix3x3> matrix3x3;
    int material;
    int layer;
    detail::AttributeDictList attribute_dicts;
    bool hidden;
  };

  // Set only when this definition was started via SkpBuilder::add_group - a group places itself
  // immediately on close(), unlike a plain component definition (which needs an explicit later
  // SkpBuilder::add_instance call).
  std::optional<GroupPlacement> group_placement;
};

struct SkpBuilder::Impl {
  detail::ArchiveWriter material_writer;
  int material_count = 0;

  std::optional<detail::ArchiveWriter> layer_writer;
  int layer_writer_start = 0;
  int layer_count = 0;

  std::optional<detail::ArchiveWriter> definition_writer;
  int definition_writer_start = 0;
  int definition_count = 0;

  std::vector<std::unique_ptr<ComponentDefinitionBuilder>> definitions;
  ComponentDefinitionBuilder* open_definition = nullptr;
  std::vector<
      std::pair<ComponentDefinitionBuilder*, ComponentDefinitionBuilder::Impl::GroupPlacement>>
      pending_groups;

  std::optional<detail::ArchiveWriter> geometry_writer;
  std::map<Point3, int> vertex_slots;
  detail::EdgeRegistry edge_registry;
  int new_entity_count = 0;
  int face_count = 0;

  Impl() : material_writer(detail::kBase, {}) {}

  int material_shift() const { return material_writer.next_slot - detail::kBase; }

  std::map<std::string, int> material_shifted_class_slot() const {
    int shift = material_shift();
    std::map<std::string, int> out;
    out["CLayer"] =
        detail::kBase + shift;  // the scaffold's only pre-declared class (see kScaffoldClassSlot)
    return out;
  }

  int layer_shift() const {
    return layer_writer ? layer_writer->next_slot - layer_writer_start : 0;
  }

  std::map<std::string, int> post_layer_class_slot() const {
    if (layer_writer) return layer_writer->class_slot;
    return material_shifted_class_slot();
  }

  int definition_shift() const {
    return definition_writer ? definition_writer->next_slot - definition_writer_start : 0;
  }

  std::map<std::string, int> post_definition_class_slot() const {
    if (definition_writer) return definition_writer->class_slot;
    return post_layer_class_slot();
  }

  void ensure_geometry_writer() {
    if (geometry_writer) return;
    if (open_definition) {
      // Calling this while a definition/group is still open would lock in the geometry writer's
      // starting slot before that definition finishes growing definition_writer - corrupting
      // every back-reference root-level geometry makes.
      throw SkpWriteError("component definition '" + open_definition->name() +
                          "' is still open - call close() on it before adding root-level geometry");
    }
    int mshift = material_shift();
    geometry_writer.emplace(detail::kScaffoldNextSlot + mshift + layer_shift() + definition_shift(),
                            post_definition_class_slot());
    // Flush any groups that closed earlier, in the order they were created - deferred until now
    // so closing one group doesn't lock in root-level slot numbering before a later
    // add_group/add_component_definition call has had a chance to run.
    for (auto& [comp, gp] : pending_groups) {
      new_entity_count +=
          geometry_writer->write_group(comp->slot(), comp->name(), gp.translation, gp.matrix3x3,
                                       gp.material, gp.layer, gp.attribute_dicts, gp.hidden);
      face_count += 1;
    }
    pending_groups.clear();
  }

  // Guard-checks + definition-header write shared by SkpBuilder::add_component_definition/
  // add_group. Deliberately does NOT construct the ComponentDefinitionBuilder itself: its
  // constructor is private with `friend class SkpBuilder;` specifically - that friendship does
  // not extend to this nested Impl struct (a nested class does not inherit its enclosing class's
  // friendships), only to SkpBuilder's own member function bodies. See add_component_definition/
  // add_group below, which do the actual construction.
  std::pair<int, std::size_t> begin_definition_header(
      const char* caller, const detail::AttributeDictList& attribute_dicts) {
    if (geometry_writer) {
      throw SkpWriteError(std::string(caller) +
                          " must be called before any add_face/add_instance calls");
    }
    if (open_definition) {
      throw SkpWriteError("component definition '" + open_definition->name() +
                          "' is still open - call close() on it before starting another");
    }
    if (!definition_writer) {
      definition_writer_start = detail::kScaffoldNextSlot + material_shift() + layer_shift();
      definition_writer.emplace(definition_writer_start, post_layer_class_slot());
    }
    auto header = definition_writer->write_definition_header(attribute_dicts);
    definition_count += 1;
    return header;
  }

  ByteBuffer to_bytes() {
    if (!pending_groups.empty()) {
      // A file with only groups (no add_face/add_instance call) would otherwise never flush
      // them - ensure_geometry_writer is a no-op once already created.
      ensure_geometry_writer();
    }
    if (face_count == 0)
      throw SkpWriteError("no geometry added - call add_face at least once before saving");

    int mshift = material_shift();
    int lshift = layer_shift();
    int dshift = definition_shift();
    int geometry_initial_slot = detail::kScaffoldNextSlot + mshift + lshift + dshift;
    int geometry_shift = geometry_writer->next_slot - geometry_initial_slot;
    int new_root_count = detail::kOrigRootCount + new_entity_count;

    ByteBuffer out;
    const std::uint8_t* data = detail::kScaffoldBlankV17;

    // Each layer's record embeds 2 pids (write_layer); materials use 1 pid each.
    int layer_pids = layer_writer ? static_cast<int>(layer_writer->next_pid - 1) : 0;
    int pid_delta = material_count + layer_pids;

    ByteBuffer prefix(data, data + (detail::kMaterialInsertPos - 4));
    if (pid_delta) {
      std::uint16_t u16 = detail::read_u16_le(prefix, detail::kPidCounterPos);
      detail::write_u16_at(prefix, detail::kPidCounterPos,
                           static_cast<std::uint16_t>(u16 + pid_delta));
    }
    std::copy(std::begin(detail::kIsoCameraPrefixPatch), std::end(detail::kIsoCameraPrefixPatch),
              prefix.begin() + static_cast<std::ptrdiff_t>(detail::kIsoCameraPrefixOffset));
    out.insert(out.end(), prefix.begin(), prefix.end());
    detail::append_u32(out, static_cast<std::uint32_t>(material_count));
    out.insert(out.end(), material_writer.buf.begin(), material_writer.buf.end());

    // material_insert_pos -> layer_insert_pos: Layer0 (and any already-existing layers) plus the
    // layer_count field, unmodified except for that count.
    ByteBuffer middle1(data + detail::kMaterialInsertPos, data + detail::kLayerInsertPos);
    std::size_t layer_count_rel = detail::kLayerCountPos - detail::kMaterialInsertPos;
    detail::write_u32_at(middle1, layer_count_rel,
                         static_cast<std::uint32_t>(detail::kOrigLayerCount + layer_count));
    out.insert(out.end(), middle1.begin(), middle1.end());
    if (layer_writer) out.insert(out.end(), layer_writer->buf.begin(), layer_writer->buf.end());

    // layer_insert_pos -> def_count_pos: just the active-layer anchor, which needs
    // +material_shift (never +layer_shift - Layer0 itself never moves just because more layers
    // are appended after it).
    ByteBuffer middle2a(data + detail::kLayerInsertPos, data + detail::kDefCountPos);
    if (mshift) detail::shift_ref(middle2a, detail::kActiveLayerAnchorRel, mshift);
    out.insert(out.end(), middle2a.begin(), middle2a.end());

    detail::append_u32(out, static_cast<std::uint32_t>(detail::kOrigDefCount + definition_count));
    if (definition_writer)
      out.insert(out.end(), definition_writer->buf.begin(), definition_writer->buf.end());

    // def_count_pos+4 -> root_count_pos: any already-existing definitions (none, in the blank
    // scaffold), unmodified.
    out.insert(out.end(), data + detail::kDefCountPos + 4, data + detail::kRootCountPos);

    detail::append_u32(out, static_cast<std::uint32_t>(new_root_count));
    out.insert(out.end(), data + detail::kRootCountPos + 4, data + detail::kTailPos);
    out.insert(out.end(), geometry_writer->buf.begin(), geometry_writer->buf.end());

    ByteBuffer tail(data + detail::kTailPos, data + detail::kScaffoldBlankV17Size);
    int total_tail_shift = mshift + lshift + dshift + geometry_shift;

    // _TAIL_REF_POSITIONS and the iso-camera tail patches both index into this same tail buffer.
    // A ref-shift that widens to the 6-byte escape form grows the buffer at that point, pushing
    // every later position forward - so every action is applied in ascending original-offset
    // order, tracking that growth, rather than at its original hardcoded offset.
    struct Action {
      std::size_t pos;
      bool is_ref;
      const detail::TailPatch* patch;
    };

    std::vector<Action> actions;
    for (std::size_t pos : detail::kTailRefPositions) actions.push_back({pos, true, nullptr});
    for (const auto& p : detail::kIsoCameraTailPatches) actions.push_back({p.pos, false, &p});
    std::sort(actions.begin(), actions.end(),
              [](const Action& a, const Action& b) { return a.pos < b.pos; });

    std::size_t growth = 0;
    for (const auto& act : actions) {
      std::size_t here = act.pos + growth;
      if (act.is_ref) {
        growth += detail::shift_ref(tail, here, total_tail_shift);
      } else {
        std::copy(act.patch->data, act.patch->data + act.patch->size,
                  tail.begin() + static_cast<std::ptrdiff_t>(here));
      }
    }
    out.insert(out.end(), tail.begin(), tail.end());
    return out;
  }
};

// ===============================================================================================
// ComponentDefinitionBuilder (method bodies)
// ===============================================================================================

ComponentDefinitionBuilder::ComponentDefinitionBuilder(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl)) {}

ComponentDefinitionBuilder::~ComponentDefinitionBuilder() = default;

const std::string& ComponentDefinitionBuilder::name() const noexcept { return impl_->name; }

bool ComponentDefinitionBuilder::closed() const noexcept { return impl_->closed; }

void ComponentDefinitionBuilder::check_writable(const std::string& action) const {
  if (impl_->closed) {
    throw SkpWriteError("component definition '" + impl_->name +
                        "' has already closed (close() was already called) - cannot add more " +
                        action + " to it");
  }
}

void ComponentDefinitionBuilder::add_face(const std::vector<Point3>& points,
                                          const FaceOptions& options) {
  check_writable("faces");
  detail::check_material_handle(*impl_->skp, options.material, "material");
  detail::check_material_handle(*impl_->skp, options.back_material, "back_material");
  detail::check_layer_handle(*impl_->skp, options.layer);
  if (points.size() < 3) throw SkpWriteError("a face needs at least 3 points");
  detail::AttributeDictList dicts;
  if (!options.attributes.empty())
    dicts.emplace_back(options.attribute_dict_name, options.attributes);
  impl_->new_entity_count += detail::write_face_or_triangulate(
      *impl_->writer, points, impl_->vertex_slots, impl_->edge_registry,
      options.material.value_or(0), options.layer.value_or(0), options.back_material.value_or(0),
      options.hidden, options.soft_edges, options.smooth_edges, options.hidden_edges,
      options.front_uv, options.back_uv, dicts, options.auto_triangulate, options.holes);
}

void ComponentDefinitionBuilder::add_circle(Point3 center, Point3 normal, double radius,
                                            const CircleOptions& options) {
  check_writable("faces");
  detail::check_material_handle(*impl_->skp, options.material, "material");
  detail::check_material_handle(*impl_->skp, options.back_material, "back_material");
  detail::check_layer_handle(*impl_->skp, options.layer);
  impl_->new_entity_count += detail::do_add_circle(
      *impl_->writer, impl_->vertex_slots, impl_->edge_registry, center, normal, radius, options);
}

void ComponentDefinitionBuilder::add_arc(Point3 center, Point3 normal, double radius,
                                         double start_angle, double end_angle,
                                         const ArcOptions& options) {
  check_writable("arcs");
  impl_->new_entity_count +=
      detail::do_add_arc(*impl_->writer, impl_->vertex_slots, impl_->edge_registry, center, normal,
                         radius, start_angle, end_angle, options);
}

void ComponentDefinitionBuilder::add_polyline(const std::vector<Point3>& points,
                                              const PolylineOptions& options) {
  check_writable("polylines");
  impl_->new_entity_count += detail::do_add_polyline(*impl_->writer, impl_->vertex_slots,
                                                     impl_->edge_registry, points, options);
}

void ComponentDefinitionBuilder::add_instance(const ComponentDefinitionBuilder& definition,
                                              const InstanceOptions& options) {
  check_writable("instances");
  detail::check_material_handle(*impl_->skp, options.material, "material");
  detail::check_layer_handle(*impl_->skp, options.layer);
  if (definition.impl_->skp != impl_->skp) {
    throw SkpWriteError("component definition '" + definition.name() +
                        "' belongs to a different builder (a different create() call) - "
                        "its slot number is meaningless here");
  }
  if (&definition == this) {
    throw SkpWriteError("component definition '" + impl_->name +
                        "' cannot nest an instance of itself");
  }
  auto matrix = detail::resolve_matrix3x3(options.matrix3x3, options.rotation);
  detail::AttributeDictList dicts;
  if (!options.attributes.empty())
    dicts.emplace_back(options.attribute_dict_name, options.attributes);
  impl_->new_entity_count += impl_->writer->write_instance(
      definition.slot(), options.name.value_or(definition.name()), options.translation, matrix,
      options.material.value_or(0), options.layer.value_or(0), dicts, options.hidden);
}

void ComponentDefinitionBuilder::add_group_instance(const ComponentDefinitionBuilder& definition,
                                                    const GroupInstanceOptions& options) {
  check_writable("groups");
  detail::check_material_handle(*impl_->skp, options.material, "material");
  detail::check_layer_handle(*impl_->skp, options.layer);
  if (definition.impl_->skp != impl_->skp) {
    throw SkpWriteError("component definition '" + definition.name() +
                        "' belongs to a different builder (a different create() call) - "
                        "its slot number is meaningless here");
  }
  if (&definition == this) {
    throw SkpWriteError("component definition '" + impl_->name +
                        "' cannot nest a group instance of itself");
  }
  auto matrix = detail::resolve_matrix3x3(options.matrix3x3, options.rotation);
  detail::AttributeDictList dicts;
  if (!options.attributes.empty())
    dicts.emplace_back(options.attribute_dict_name, options.attributes);
  impl_->new_entity_count += impl_->writer->write_group(
      definition.slot(), options.name.value_or(definition.name()), options.translation, matrix,
      options.material.value_or(0), options.layer.value_or(0), dicts, options.hidden);
}

int ComponentDefinitionBuilder::slot() const noexcept { return impl_->slot; }

void ComponentDefinitionBuilder::close() {
  if (impl_->closed)
    throw SkpWriteError("component definition '" + impl_->name + "' has already closed");
  if (impl_->new_entity_count == 0) {
    throw SkpWriteError("component definition '" + impl_->name +
                        "' has no geometry - add at least one face");
  }
  detail::write_u32_at(impl_->writer->buf, impl_->count_patch_pos,
                       static_cast<std::uint32_t>(impl_->new_entity_count));
  impl_->writer->write_definition_tail(impl_->name);
  impl_->closed = true;
  // ComponentDefinitionBuilder is a friend of SkpBuilder (create.hpp), so this is allowed to
  // reach into a *different* SkpBuilder's private impl_ - access control in C++ is per-class,
  // not per-object.
  impl_->skp->impl_->open_definition = nullptr;
  if (impl_->group_placement) {
    impl_->skp->impl_->pending_groups.emplace_back(this, *impl_->group_placement);
  }
}

// ===============================================================================================
// SkpBuilder (method bodies) - struct SkpBuilder::Impl itself is defined further up, alongside
// ComponentDefinitionBuilder::Impl.
// ===============================================================================================

SkpBuilder::SkpBuilder() : impl_(std::make_unique<Impl>()) {}

SkpBuilder::~SkpBuilder() = default;

int SkpBuilder::add_material(const std::string& name, Color4 rgba, std::optional<double> opacity) {
  if (impl_->geometry_writer)
    throw SkpWriteError("add_material must be called before any add_face calls");
  if (impl_->layer_writer)
    throw SkpWriteError("add_material must be called before any add_layer calls");
  if (impl_->definition_writer) {
    throw SkpWriteError("add_material must be called before any add_component_definition calls");
  }
  auto it = materials_by_name.find(name);
  if (it != materials_by_name.end()) return it->second;
  int slot = impl_->material_writer.write_material(name, rgba, opacity);
  materials_by_name[name] = slot;
  impl_->material_count += 1;
  return slot;
}

int SkpBuilder::add_material(const std::string& name, Color3 rgb, std::optional<double> opacity) {
  return add_material(name, Color4{rgb[0], rgb[1], rgb[2], 255}, opacity);
}

int SkpBuilder::add_texture_material(const std::string& name,
                                     const std::filesystem::path& image_path,
                                     std::optional<double> applied_height,
                                     std::optional<double> applied_width,
                                     std::optional<double> opacity) {
  if (impl_->geometry_writer)
    throw SkpWriteError("add_texture_material must be called before any add_face calls");
  if (impl_->layer_writer)
    throw SkpWriteError("add_texture_material must be called before any add_layer calls");
  if (impl_->definition_writer) {
    throw SkpWriteError(
        "add_texture_material must be called before any add_component_definition calls");
  }
  auto it = materials_by_name.find(name);
  if (it != materials_by_name.end()) return it->second;
  std::ifstream f(image_path, std::ios::binary);
  if (!f) throw SkpWriteError("cannot open texture image file: " + image_path.string());
  ByteBuffer image_bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  int subtype = detail::detect_image_subtype(image_bytes);
  int slot = impl_->material_writer.write_textured_material(
      name, image_bytes, image_path.string(), subtype, applied_height, applied_width, opacity);
  materials_by_name[name] = slot;
  impl_->material_count += 1;
  return slot;
}

int SkpBuilder::add_layer(const std::string& name, const LayerOptions& options) {
  if (impl_->geometry_writer)
    throw SkpWriteError("add_layer must be called before any add_face calls");
  if (impl_->definition_writer) {
    throw SkpWriteError("add_layer must be called before any add_component_definition calls");
  }
  auto it = layers_by_name.find(name);
  if (it != layers_by_name.end()) return it->second;
  if (!impl_->layer_writer) {
    impl_->layer_writer_start = detail::kLayerWriterBase + impl_->material_shift();
    impl_->layer_writer.emplace(impl_->layer_writer_start, impl_->material_shifted_class_slot());
  }
  int slot = impl_->layer_writer->write_layer(name, true, options.hidden, options.color);
  layers_by_name[name] = slot;
  impl_->layer_count += 1;
  return slot;
}

// add_component_definition/add_group below both construct a ComponentDefinitionBuilder directly
// (rather than through a shared free-function helper) because its constructor is private with
// `friend class SkpBuilder;` *specifically* - that friendship covers any genuine SkpBuilder::
// member function (both of these are), but not a nested struct (SkpBuilder::Impl, which is why
// the guard-checks/header-write live in SkpBuilder::Impl::begin_definition_header but the
// construction itself doesn't) and not an ordinary free function either (so it can't be
// factored out further without adding an explicit `friend` for it).

ComponentDefinitionBuilder& SkpBuilder::add_component_definition(const std::string& name,
                                                                 const DefinitionOptions& options) {
  detail::AttributeDictList dicts;
  if (!options.attributes.empty())
    dicts.emplace_back(options.attribute_dict_name, options.attributes);
  auto [slot, count_patch_pos] = impl_->begin_definition_header("add_component_definition", dicts);
  auto cdb_impl = std::make_unique<ComponentDefinitionBuilder::Impl>();
  cdb_impl->skp = this;
  cdb_impl->slot = slot;
  cdb_impl->name = name;
  cdb_impl->count_patch_pos = count_patch_pos;
  cdb_impl->writer = &*impl_->definition_writer;
  impl_->definitions.push_back(std::unique_ptr<ComponentDefinitionBuilder>(
      new ComponentDefinitionBuilder(std::move(cdb_impl))));
  ComponentDefinitionBuilder* raw = impl_->definitions.back().get();
  impl_->open_definition = raw;
  return *raw;
}

ComponentDefinitionBuilder& SkpBuilder::add_group(const GroupOptions& options) {
  detail::check_material_handle(*this, options.material, "material");
  detail::check_layer_handle(*this, options.layer);
  auto matrix = detail::resolve_matrix3x3(options.matrix3x3, options.rotation);
  auto [slot, count_patch_pos] = impl_->begin_definition_header("add_group", {});
  auto cdb_impl = std::make_unique<ComponentDefinitionBuilder::Impl>();
  cdb_impl->skp = this;
  cdb_impl->slot = slot;
  cdb_impl->name = options.name.value_or("Group");
  cdb_impl->count_patch_pos = count_patch_pos;
  cdb_impl->writer = &*impl_->definition_writer;
  detail::AttributeDictList group_dicts;
  if (!options.attributes.empty())
    group_dicts.emplace_back(options.attribute_dict_name, options.attributes);
  cdb_impl->group_placement = ComponentDefinitionBuilder::Impl::GroupPlacement{
      options.translation,       matrix,      options.material.value_or(0),
      options.layer.value_or(0), group_dicts, options.hidden};
  impl_->definitions.push_back(std::unique_ptr<ComponentDefinitionBuilder>(
      new ComponentDefinitionBuilder(std::move(cdb_impl))));
  ComponentDefinitionBuilder* raw = impl_->definitions.back().get();
  impl_->open_definition = raw;
  return *raw;
}

void SkpBuilder::add_instance(const ComponentDefinitionBuilder& definition,
                              const InstanceOptions& options) {
  detail::check_material_handle(*this, options.material, "material");
  detail::check_layer_handle(*this, options.layer);
  if (definition.impl_->skp != this) {
    throw SkpWriteError("component definition '" + definition.name() +
                        "' belongs to a different builder (a different create() call) - "
                        "its slot number is meaningless here");
  }
  if (!definition.closed()) {
    throw SkpWriteError("component definition '" + definition.name() +
                        "' is still open - call close() on it before calling add_instance");
  }
  auto matrix = detail::resolve_matrix3x3(options.matrix3x3, options.rotation);
  impl_->ensure_geometry_writer();
  detail::AttributeDictList dicts;
  if (!options.attributes.empty())
    dicts.emplace_back(options.attribute_dict_name, options.attributes);
  impl_->new_entity_count += impl_->geometry_writer->write_instance(
      definition.slot(), options.name.value_or(definition.name()), options.translation, matrix,
      options.material.value_or(0), options.layer.value_or(0), dicts, options.hidden);
  impl_->face_count += 1;  // reuses the "at least one root entity" check in to_bytes
}

void SkpBuilder::add_image(const std::filesystem::path& image_path, double width, double height,
                           const ImageOptions& options) {
  detail::check_layer_handle(*this, options.layer);
  int mat =
      add_texture_material("__openskp_image_" + std::to_string(impl_->material_count), image_path);
  auto& image_def = add_component_definition("Image" + std::to_string(impl_->definition_count));
  // Standard (0,0)-at-bottom-left, V increasing upward - no vertical flip. Every other UV-related
  // fact in this file is calibrated against real SketchUp output; this one specific sense is NOT
  // (no ground truth available - see add_image's own warning in create.hpp) and could come out
  // upside down in real SketchUp if its texture sampling flips V the other way.
  FaceOptions face_opts;
  face_opts.material = mat;
  face_opts.front_uv = UvCorrespondence{
      UvPoint{Point3{0.0, 0.0, 0.0}, {0.0, 0.0}},
      UvPoint{Point3{width, 0.0, 0.0}, {1.0, 0.0}},
      UvPoint{Point3{0.0, height, 0.0}, {0.0, 1.0}},
  };
  image_def.add_face({{0.0, 0.0, 0.0}, {width, 0.0, 0.0}, {width, height, 0.0}, {0.0, height, 0.0}},
                     face_opts);
  image_def.close();
  auto matrix = detail::resolve_matrix3x3(options.matrix3x3, options.rotation);
  impl_->ensure_geometry_writer();
  impl_->new_entity_count += impl_->geometry_writer->write_image(
      image_def.slot(), options.translation, matrix, options.layer.value_or(0), options.hidden);
  impl_->face_count += 1;  // reuses the "at least one root entity" check in to_bytes
}

void SkpBuilder::add_face(const std::vector<Point3>& points, const FaceOptions& options) {
  detail::check_material_handle(*this, options.material, "material");
  detail::check_material_handle(*this, options.back_material, "back_material");
  detail::check_layer_handle(*this, options.layer);
  if (points.size() < 3) throw SkpWriteError("a face needs at least 3 points");
  impl_->ensure_geometry_writer();
  detail::AttributeDictList dicts;
  if (!options.attributes.empty())
    dicts.emplace_back(options.attribute_dict_name, options.attributes);
  impl_->new_entity_count += detail::write_face_or_triangulate(
      *impl_->geometry_writer, points, impl_->vertex_slots, impl_->edge_registry,
      options.material.value_or(0), options.layer.value_or(0), options.back_material.value_or(0),
      options.hidden, options.soft_edges, options.smooth_edges, options.hidden_edges,
      options.front_uv, options.back_uv, dicts, options.auto_triangulate, options.holes);
  impl_->face_count += 1;
}

void SkpBuilder::add_circle(Point3 center, Point3 normal, double radius,
                            const CircleOptions& options) {
  detail::check_material_handle(*this, options.material, "material");
  detail::check_material_handle(*this, options.back_material, "back_material");
  detail::check_layer_handle(*this, options.layer);
  impl_->ensure_geometry_writer();
  impl_->new_entity_count +=
      detail::do_add_circle(*impl_->geometry_writer, impl_->vertex_slots, impl_->edge_registry,
                            center, normal, radius, options);
  impl_->face_count += 1;
}

void SkpBuilder::add_arc(Point3 center, Point3 normal, double radius, double start_angle,
                         double end_angle, const ArcOptions& options) {
  impl_->ensure_geometry_writer();
  impl_->new_entity_count +=
      detail::do_add_arc(*impl_->geometry_writer, impl_->vertex_slots, impl_->edge_registry, center,
                         normal, radius, start_angle, end_angle, options);
  impl_->face_count += 1;  // reuses the "at least one root entity" check in to_bytes
}

void SkpBuilder::add_polyline(const std::vector<Point3>& points, const PolylineOptions& options) {
  impl_->ensure_geometry_writer();
  impl_->new_entity_count += detail::do_add_polyline(*impl_->geometry_writer, impl_->vertex_slots,
                                                     impl_->edge_registry, points, options);
  impl_->face_count += 1;  // reuses the "at least one root entity" check in to_bytes
}

void SkpBuilder::add_dimension(Point3 p1, Point3 p2, double offset) {
  impl_->ensure_geometry_writer();
  impl_->geometry_writer->write_dimension(p1, p2, offset);
  impl_->new_entity_count += 1;
  impl_->face_count += 1;  // reuses the "at least one root entity" check in to_bytes
}

void SkpBuilder::add_text(const std::string& text, Point3 point, Point3 leader) {
  impl_->ensure_geometry_writer();
  impl_->geometry_writer->write_text(text, point, leader);
  impl_->new_entity_count += 1;
  impl_->face_count += 1;  // reuses the "at least one root entity" check in to_bytes
}

void SkpBuilder::add_construction_line(Point3 point, std::optional<Point3> point2,
                                       std::optional<Point3> direction) {
  impl_->ensure_geometry_writer();
  impl_->geometry_writer->write_construction_line(point, point2, direction);
  impl_->new_entity_count += 1;
  impl_->face_count += 1;  // reuses the "at least one root entity" check in to_bytes
}

void SkpBuilder::add_construction_point(Point3 position) {
  impl_->ensure_geometry_writer();
  impl_->geometry_writer->write_construction_point(position);
  impl_->new_entity_count += 1;
  impl_->face_count += 1;  // reuses the "at least one root entity" check in to_bytes
}

void SkpBuilder::add_section_plane(Point3 point, Point3 normal) {
  impl_->ensure_geometry_writer();
  impl_->geometry_writer->write_section_plane(point, normal);
  impl_->new_entity_count += 1;
  impl_->face_count += 1;  // reuses the "at least one root entity" check in to_bytes
}

ByteBuffer SkpBuilder::to_bytes() { return impl_->to_bytes(); }

void SkpBuilder::save(const std::filesystem::path& path) {
  ByteBuffer bytes = to_bytes();
  std::ofstream f(path, std::ios::binary);
  if (!f) throw SkpWriteError("cannot open output file for writing: " + path.string());
  f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!f) throw SkpWriteError("failed writing output file: " + path.string());
}

std::unique_ptr<SkpBuilder> create() { return std::make_unique<SkpBuilder>(); }

}  // namespace openskp
