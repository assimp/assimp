#pragma once

/**
 * CustomData layer types — values from blender.git
 * `source/blender/makesdna/DNA_customdata_types.h` (current CD_NUMTYPES=53).
 *
 * Mesh geometry on disk by Blender file version (header digits):
 *   classic *mvert / *mface        - early 2.x
 *   CD_MLOOP / CD_MPOLY / *mloop   - BMesh era
 *   CD_PROP_FLOAT3 "position" +
 *   CD_PROP_INT32 ".corner_vert"   - ~2.90+ / 3.x / 4.x (generic attrs in CustomData)
 *   AttributeStorage               - ~4.5+ / 5.x (separate DNA; see BlenderMeshAttributes.h)
 */

#include "BlenderDNA.h"
#include "BlenderScene.h"
#include <memory>
#include <vector>

namespace Assimp {
namespace Blender {

enum CustomDataType {
    CD_AUTO_FROM_NAME = -1,

    CD_MVERT = 0,
    CD_MSTICKY = 1, /* deprecated */
    CD_MDEFORMVERT = 2,
    CD_MEDGE = 3,
    CD_MFACE = 4,
    CD_MTFACE = 5,
    CD_MCOL = 6,
    CD_ORIGINDEX = 7,
    CD_NORMAL = 8,
    CD_FACEMAP = 9, /* deprecated */
    CD_PROP_FLOAT = 10,
    CD_PROP_INT32 = 11,
    CD_PROP_INT = CD_PROP_INT32, /* Assimp alias */
    CD_PROP_STRING = 12,
    CD_ORIGSPACE = 13,
    CD_ORCO = 14,
    CD_MTEXPOLY = 15, /* deprecated */
    CD_MLOOPUV = 16, /* deprecated */
    CD_PROP_BYTE_COLOR = 17,
    CD_MLOOPCOL = 17, /* legacy name; modern DNA: BYTE_COLOR at same index */
    CD_TANGENT = 18,
    CD_MDISPS = 19,
    CD_PROP_FLOAT4X4 = 20,
    CD_PROP_INT16_2D = 22,
    CD_CLOTH_ORCO = 23,
    CD_PROP_FLOAT4 = 24,
    CD_MPOLY = 25,
    CD_MLOOP = 26,
    CD_SHAPE_KEYINDEX = 27,
    CD_SHAPEKEY = 28,
    CD_BWEIGHT = 29,
    CD_CREASE = 30,
    CD_ORIGSPACE_MLOOP = 31,
    CD_BM_ELEM_PYPTR = 33,
    CD_PAINT_MASK = 34,
    CD_GRID_PAINT_MASK = 35,
    CD_MVERT_SKIN = 36,
    CD_FREESTYLE_EDGE = 37,
    CD_FREESTYLE_FACE = 38,
    CD_MLOOPTANGENT = 39,
    CD_TESSLOOPNORMAL = 40,
    CD_CUSTOMLOOPNORMAL = 41,
    CD_SCULPT_FACE_SETS = 42,
    CD_PROP_INT8 = 45,
    CD_PROP_INT32_2D = 46,
    CD_PROP_COLOR = 47,
    CD_PROP_FLOAT3 = 48,
    CD_PROP_FLOAT2 = 49,
    CD_PROP_BOOL = 50,
    CD_PROP_QUATERNION = 52,

    CD_NUMTYPES = 53
};

/** Raw CustomData layer payload (CD_PROP_FLOAT3 / INT32 / …) — not a DNA struct. */
struct RawCustomData final : ElemBase {
    std::vector<uint8_t> bytes;
    size_t count = 0;
    size_t stride = 0;
};

bool isValidCustomDataType(int cdtype);

/** Element stride for raw CD_PROP_* types; 0 if not a raw array type we hydrate. */
size_t CustomDataRawStride(int cdtype);

std::shared_ptr<CustomDataLayer> getCustomDataLayer(const CustomData &customdata, CustomDataType cdtype, const std::string &name);

const ElemBase *getCustomDataLayerData(const CustomData &customdata, CustomDataType cdtype, const std::string &name);

/** Find layer by name only (any type) — Blender 4.x position / .corner_vert. */
std::shared_ptr<CustomDataLayer> getCustomDataLayerByName(const CustomData &customdata, const std::string &name);

} // namespace Blender
} // namespace Assimp
