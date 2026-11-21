#include "BlenderCustomData.h"
#include "BlenderScene.h"
#include "BlenderSceneGen.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>

namespace Assimp {
namespace Blender {
namespace {

// Structure-backed readers (existing Assimp DNA converters).
#define DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(ty)                                                  \
    CustomDataTypeDescription {                                                                    \
        [](void *out, size_t cnt, const FileDatabase &db) -> bool {                                \
            ty *arr = static_cast<ty *>(out);                                                      \
            const Structure &s = db.dna[#ty];                                                      \
            for (size_t i = 0; i < cnt; ++i) {                                                     \
                s.Convert(arr[i], db);                                                             \
            }                                                                                      \
            return true;                                                                           \
        },                                                                                         \
                [](size_t cnt) -> ElemBase * { return new ty[cnt]; },                              \
                [](ElemBase *p) { delete[] static_cast<ty *>(p); }, #ty                            \
    }

#define DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION                                                 \
    CustomDataTypeDescription {                                                                    \
        nullptr, nullptr, nullptr, nullptr                                                         \
    }

struct CustomDataTypeDescription {
    using PRead = bool (*)(void *out, size_t cnt, const FileDatabase &db);
    using PCreate = ElemBase *(*)(size_t cnt);
    using PDestroy = void (*)(ElemBase *p);
    PRead Read;
    PCreate Create;
    PDestroy Destroy;
    const char *TypeName;
};

size_t RawStrideForType(int cdtype) {
    switch (cdtype) {
    case CD_PROP_BOOL:
    case CD_PROP_INT8: return 1;
    case CD_PROP_FLOAT:
    case CD_PROP_INT32: return 4;
    case CD_PROP_FLOAT2:
    case CD_PROP_INT16_2D: return 8;
    case CD_PROP_FLOAT3: return 12;
    case CD_PROP_FLOAT4:
    case CD_PROP_COLOR:
    case CD_PROP_QUATERNION: return 16;
    case CD_PROP_INT32_2D: return 8;
    case CD_PROP_FLOAT4X4: return 64;
    default: return 0;
    }
}

bool readRawCustomData(void *out, size_t cnt, size_t stride, size_t dataSize, const FileDatabase &db) {
    if (stride == 0 || cnt == 0) {
        return false;
    }
    cnt = std::min(cnt, dataSize / stride);
    if (cnt == 0) {
        return false;
    }
    RawCustomData *raw = static_cast<RawCustomData *>(out);
    raw->stride = stride;
    raw->count = cnt;
    raw->bytes.resize(cnt * stride);
    db.reader->CopyAndAdvance(raw->bytes.data(), raw->bytes.size());
    return true;
}

ElemBase *createRaw(size_t) {
    return new RawCustomData();
}
void destroyRaw(ElemBase *p) {
    delete static_cast<RawCustomData *>(p);
}

#define DECL_RAW_CUSTOMDATATYPEDESCRIPTION                                                         \
    CustomDataTypeDescription {                                                                    \
        nullptr, &createRaw, &destroyRaw, nullptr                                                  \
    }

// Index = eCustomDataType. Keep in sync with BlenderCustomData.h / DNA_customdata_types.h.
static const std::array<CustomDataTypeDescription, CD_NUMTYPES> customDataTypeDescriptions = { {
        /* 0  CD_MVERT */ DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(MVert),
        /* 1 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 2  CD_MDEFORMVERT */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 3  CD_MEDGE */ DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(MEdge),
        /* 4  CD_MFACE */ DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(MFace),
        /* 5  CD_MTFACE */ DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(MTFace),
        /* 6  CD_MCOL */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 7  CD_ORIGINDEX */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 8  CD_NORMAL */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 9 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 10 CD_PROP_FLOAT */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 11 CD_PROP_INT32 */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 12 CD_PROP_STRING */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 13 CD_ORIGSPACE */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 14 CD_ORCO */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 15 CD_MTEXPOLY */ DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(MTexPoly),
        /* 16 CD_MLOOPUV */ DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(MLoopUV),
        /* 17 CD_PROP_BYTE_COLOR / legacy MLOOPCOL */ DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(MLoopCol),
        /* 18 CD_TANGENT */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 19 CD_MDISPS */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 20 CD_PROP_FLOAT4X4 */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 21 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 22 CD_PROP_INT16_2D */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 23 CD_CLOTH_ORCO */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 24 CD_PROP_FLOAT4 */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 25 CD_MPOLY */ DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(MPoly),
        /* 26 CD_MLOOP */ DECL_STRUCT_CUSTOMDATATYPEDESCRIPTION(MLoop),
        /* 27 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 28 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 29 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 30 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 31 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 32 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 33 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 34 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 35 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 36 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 37 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 38 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 39 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 40 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 41 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 42 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 43 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 44 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 45 CD_PROP_INT8 */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 46 CD_PROP_INT32_2D */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 47 CD_PROP_COLOR */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 48 CD_PROP_FLOAT3 */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 49 CD_PROP_FLOAT2 */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 50 CD_PROP_BOOL */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
        /* 51 */ DECL_UNSUPPORTED_CUSTOMDATATYPEDESCRIPTION,
        /* 52 CD_PROP_QUATERNION */ DECL_RAW_CUSTOMDATATYPEDESCRIPTION,
} };

} // namespace

size_t CustomDataRawStride(int cdtype) {
    return RawStrideForType(cdtype);
}

bool isValidCustomDataType(const int cdtype) {
    return cdtype >= 0 && cdtype < CD_NUMTYPES;
}

bool readCustomData(std::shared_ptr<ElemBase> &out, const int cdtype, size_t cnt, const size_t dataSize, const FileDatabase &db) {
    if (!isValidCustomDataType(cdtype)) {
        // Newer Blender may add types beyond our table — skip layer, do not abort Mesh.
        return false;
    }

    const CustomDataTypeDescription &cdtd = customDataTypeDescriptions[cdtype];
    const size_t rawStride = RawStrideForType(cdtype);

    if (rawStride > 0 && cdtd.Create && cdtd.Destroy) {
        out.reset(cdtd.Create(cnt), cdtd.Destroy);
        return readRawCustomData(out.get(), cnt, rawStride, dataSize, db);
    }

    if (cdtd.Read && cdtd.Create && cdtd.Destroy && cnt > 0) {
        const Structure &s = db.dna[cdtd.TypeName];
        if (s.size == 0) {
            return false;
        }
        cnt = std::min(cnt, dataSize / s.size);
        if (cnt == 0) {
            return false;
        }
        out.reset(cdtd.Create(cnt), cdtd.Destroy);
        return cdtd.Read(out.get(), cnt, db);
    }
    return false;
}

std::shared_ptr<CustomDataLayer> getCustomDataLayer(const CustomData &customdata, const CustomDataType cdtype, const std::string &name) {
    for (const auto &layer : customdata.layers) {
        if (layer && layer->type == cdtype && name == layer->name) {
            return layer;
        }
    }
    return nullptr;
}

std::shared_ptr<CustomDataLayer> getCustomDataLayerByName(const CustomData &customdata, const std::string &name) {
    for (const auto &layer : customdata.layers) {
        if (layer && name == layer->name) {
            return layer;
        }
    }
    return nullptr;
}

const ElemBase *getCustomDataLayerData(const CustomData &customdata, const CustomDataType cdtype, const std::string &name) {
    const std::shared_ptr<CustomDataLayer> pLayer = getCustomDataLayer(customdata, cdtype, name);
    return pLayer ? pLayer->data.get() : nullptr;
}

} // namespace Blender
} // namespace Assimp
