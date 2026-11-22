/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------
Copyright (c) 2006-2026, assimp team
----------------------------------------------------------------------
*/
/** @file BlenderMeshAttributes.h
 *  Version-aware mesh geometry hydrate for Blender .blend files.
 *
 *  File header version (`FileDatabase::blender_file_version`):
 *    v0: 3 digits (e.g. 403 = Blender 4.03)
 *    v1: 4 digits (e.g. 500 = Blender 5.00)
 *
 *  Geometry on disk (from blender.git DNA / mesh_blend_write):
 *    classic *mvert / *mloop / *mpoly - early / forward-compat pointers
 *    CustomData CD_PROP_FLOAT3
 *      "position" + CD_PROP_INT32 ".corner_vert"
 *      + face_offset_indices          - ~2.90+ through 4.x (no AttributeStorage in 4.3)
 *    AttributeStorage                  - ~4.5+ / 5.x (DNA_attribute_types.h)
 *
 *  DNA refs: DNA_customdata_types.h, DNA_attribute_types.h, DNA_mesh_types.h,
 *  dna_rename_defs.h, BKE_attribute_enums.hh
 */
#pragma once

#include <assimp/mesh.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Assimp {
namespace Blender {

struct Mesh;
class FileDatabase;

/** One named UV map (corner-domain), uv.size() == 2 * totloop. */
struct BlendUVLayer {
    std::string name;
    std::vector<float> uv;
};

/** blender::bke::AttrType (BKE_attribute_enums.hh) */
enum class BlendAttrType : std::int16_t {
    Bool = 0,
    Int8 = 1,
    Int16_2D = 2,
    Int32 = 3,
    Int32_2D = 4,
    Float = 5,
    Float2 = 6,
    Float3 = 7,
    Float4x4 = 8,
    ColorByte = 9,
    ColorFloat = 10,
    Quaternion = 11,
    String = 12,
    Float4 = 13
};

/** blender::bke::AttrDomain (BKE_attribute_enums.hh) */
enum class BlendAttrDomain : std::int8_t {
    Point = 0,
    Edge = 1,
    Face = 2,
    Corner = 3,
    Curve = 4,
    Instance = 5,
    Layer = 6
};

/** blender::bke::AttrStorageType (BKE_attribute_enums.hh) */
enum class BlendAttrStorageType : std::int8_t {
    Array = 0,
    Single = 1
};

static constexpr std::size_t kBlendAttrDnaSize = 24;
static constexpr std::size_t kBlendAttrArrayDnaSizeMin = 24;
static constexpr std::size_t kBlendAttrArrayDnaSize = 32;
static constexpr std::size_t kBlendAttrSingleDnaSize = 16;

/**
 * Blender file-version cutovers for mesh geometry paths.
 * Compared against FileDatabase::blender_file_version (header digits).
 */
enum BlendMeshGeomEra : int {
    /** CD_PROP_FLOAT3 / named generic attrs appear (~Blender 2.90). */
    kBlendVer_CustomDataGenericAttrs = 290,
    /** AttributeStorage DNA on Mesh (~Blender 4.5+; dominant in 5.x). */
    kBlendVer_AttributeStorage = 450
};

inline std::size_t BlendAttrTypeSize(BlendAttrType t) {
    switch (t) {
    case BlendAttrType::Bool:
    case BlendAttrType::Int8: return 1;
    case BlendAttrType::Int16_2D: return 4;
    case BlendAttrType::Int32: return 4;
    case BlendAttrType::Int32_2D: return 8;
    case BlendAttrType::Float: return 4;
    case BlendAttrType::Float2: return 8;
    case BlendAttrType::Float3: return 12;
    case BlendAttrType::Float4:
    case BlendAttrType::ColorFloat:
    case BlendAttrType::Quaternion: return 16;
    case BlendAttrType::ColorByte: return 4;
    case BlendAttrType::Float4x4: return 64;
    case BlendAttrType::String: return 0;
    default: return 0;
    }
}

bool HydrateMeshFromAttributeStorage(Mesh &mesh, const FileDatabase &db);
bool HydrateMeshFromCustomData(Mesh &mesh, const FileDatabase &db);

/**
 * Fill classic mvert/mloop/mpoly when missing, choosing the path by
 * blender_file_version (and DNA presence). Returns true if topology is usable.
 */
bool HydrateMeshGeometry(Mesh &mesh, const FileDatabase &db);

/**
 * Collect named UV maps from CustomData (CD_MLOOPUV / CD_PROP_FLOAT2) and
 * AttributeStorage (corner float2). Caps at maxLayers (default
 * AI_MAX_NUMBER_OF_TEXTURECOORDS).
 * Logs a warning when more than one map exists, and when truncating.
 * Returns number of layers written to out.
 */
unsigned CollectMeshUVLayers(const Mesh &mesh, const FileDatabase &db,
        std::vector<BlendUVLayer> &out,
        unsigned maxLayers = AI_MAX_NUMBER_OF_TEXTURECOORDS);

} // namespace Blender
} // namespace Assimp
