/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------
Copyright (c) 2006-2026, assimp team
----------------------------------------------------------------------
*/
/** @file BlenderMeshAttributes.cpp */

#include "BlenderMeshAttributes.h"
#include "BlenderCustomData.h"
#include "BlenderDNA.h"
#include "BlenderScene.h"

#include <assimp/ByteSwapper.h>
#include <assimp/LogAux.h>
#include <assimp/mesh.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace Assimp {
namespace Blender {
namespace {

/** True if block is DATA owned by the active ID datamap (Blender readfile datamap). */
bool BlockInActiveIdScope(const FileBlockHead &e, const FileDatabase &db) {
    if (!db.active_id_scope.val) {
        return true;
    }
    return e.owner_id.val == db.active_id_scope.val;
}

const FileBlockHead *FindBlockExactAddress(const Pointer &ptr, const FileDatabase &db, size_t expectedSize) {
    if (!ptr.val) {
        return nullptr;
    }
    // Blender 5 generated `old` IDs collide across ME datablocks (MarblesForever:
    // dozens of AttributeArray at the same address). Resolve like Blender: only
    // DATA belonging to the active ID (owner_id / active_id_scope).
    std::vector<const FileBlockHead *> exact;
    exact.reserve(4);
    for (const FileBlockHead &e : db.entries) {
        if (e.address.val != ptr.val) {
            continue;
        }
        if (!BlockInActiveIdScope(e, db)) {
            continue;
        }
        exact.push_back(&e);
    }
    if (exact.empty()) {
        return nullptr;
    }
    if (expectedSize > 0) {
        for (const FileBlockHead *b : exact) {
            if (b->size == expectedSize) {
                return b;
            }
        }
        for (const FileBlockHead *b : exact) {
            if (b->num > 0 && static_cast<size_t>(b->num) * (b->size / b->num) == expectedSize) {
                return b;
            }
        }
        return nullptr;
    }
    return exact.front();
}

bool ReadBlockBytes(const FileBlockHead &block, const FileDatabase &db, std::vector<uint8_t> &out) {
    out.resize(block.size);
    if (block.size == 0) {
        return true;
    }
    const StreamReaderAny::pos pold = db.reader->GetCurrentPos();
    db.reader->SetCurrentPos(block.start);
    db.reader->CopyAndAdvance(out.data(), block.size);
    db.reader->SetCurrentPos(pold);
    return true;
}

bool ReadIntArrayAtPointer(const Pointer &ptr, size_t count, const FileDatabase &db, std::vector<int> &out) {
    const size_t bytes = count * sizeof(int);
    const FileBlockHead *block = FindBlockExactAddress(ptr, db, bytes);
    if (block == nullptr) {
        for (const FileBlockHead &e : db.entries) {
            if (e.address.val != ptr.val || e.size != bytes) {
                continue;
            }
            if (!BlockInActiveIdScope(e, db)) {
                continue;
            }
            block = &e;
            break;
        }
    }
    if (block == nullptr || block->size < bytes) {
        return false;
    }
    std::vector<uint8_t> raw;
    if (!ReadBlockBytes(*block, db, raw)) {
        return false;
    }
    out.resize(count);
    for (size_t i = 0; i < count; ++i) {
        int v = 0;
        std::memcpy(&v, raw.data() + i * 4, 4);
        if (!db.little) {
            ByteSwap::Swap4(&v);
        }
        out[i] = v;
    }
    return true;
}

struct RawAttr {
    std::string name;
    BlendAttrType data_type = BlendAttrType::Bool;
    BlendAttrDomain domain = BlendAttrDomain::Point;
    BlendAttrStorageType storage = BlendAttrStorageType::Array;
    int64_t size = 0;
    std::vector<uint8_t> data;
};

/**
 * Read blender::AttributeArray (DNA_attribute_types.h).
 * Attribute.data points at an AttributeArray file block; .data inside that
 * points at the typed payload. Generated `old` addresses collide across IDs.
 */
bool LoadAttributeArrayPayload(const Pointer &arrayPtr, BlendAttrType type, const FileDatabase &db, RawAttr &out,
        int64_t preferredSize = -1) {
    std::vector<const FileBlockHead *> aaBlocks;
    for (const FileBlockHead &e : db.entries) {
        if (e.address.val != arrayPtr.val || e.size < kBlendAttrArrayDnaSizeMin) {
            continue;
        }
        if (!BlockInActiveIdScope(e, db)) {
            continue;
        }
        aaBlocks.push_back(&e);
    }
    if (aaBlocks.empty()) {
        return false;
    }

    const size_t elem = BlendAttrTypeSize(type);
    if (elem == 0) {
        return false;
    }

    const FileBlockHead *bestData = nullptr;
    int64_t bestSize = 0;
    bool bestIsSingle = false;

    for (const FileBlockHead *aaBlock : aaBlocks) {
        std::vector<uint8_t> aaRaw;
        if (!ReadBlockBytes(*aaBlock, db, aaRaw) || aaRaw.size() < kBlendAttrArrayDnaSizeMin) {
            continue;
        }
        // DNA_attribute_types.h AttributeArray layout
        Pointer dataPtr;
        int64_t size = 0;
        int8_t is_single = 0;
        std::memcpy(&dataPtr.val, aaRaw.data() + 0, 8);
        std::memcpy(&size, aaRaw.data() + 16, 8);
        if (aaRaw.size() >= kBlendAttrArrayDnaSize) {
            is_single = static_cast<int8_t>(aaRaw[24]);
        }
        if (!db.little) {
            ByteSwap::Swap8(&dataPtr.val);
            ByteSwap::Swap8(&size);
        }
        if (size <= 0 || !dataPtr.val) {
            continue;
        }
        // Strict domain-size match (Blender ArrayData.size == domain length),
        // unless is_single marks 5.0-compat expanded single storage.
        if (preferredSize >= 0 && size != preferredSize && !is_single) {
            continue;
        }
        const size_t want = static_cast<size_t>(size) * elem;
        const FileBlockHead *dataBlock = nullptr;
        for (const FileBlockHead &e : db.entries) {
            if (e.address.val != dataPtr.val || e.size != want) {
                continue;
            }
            if (!BlockInActiveIdScope(e, db)) {
                continue;
            }
            dataBlock = &e;
            break;
        }
        if (dataBlock == nullptr) {
            continue;
        }
        bestData = dataBlock;
        bestSize = size;
        bestIsSingle = is_single != 0;
        break;
    }
    if (bestData == nullptr) {
        return false;
    }
    if (!ReadBlockBytes(*bestData, db, out.data)) {
        return false;
    }
    out.size = bestSize;
    (void)bestIsSingle; // payload already expanded to domain size when written with is_single
    return true;
}

/** Read blender::AttributeSingle (DNA_attribute_types.h) - one element. */
bool LoadAttributeSinglePayload(const Pointer &singlePtr, BlendAttrType type, const FileDatabase &db, RawAttr &out) {
    const size_t elem = BlendAttrTypeSize(type);
    if (elem == 0) {
        return false;
    }
    const FileBlockHead *block = nullptr;
    for (const FileBlockHead &e : db.entries) {
        if (e.address.val != singlePtr.val || e.size < kBlendAttrSingleDnaSize) {
            continue;
        }
        if (!BlockInActiveIdScope(e, db)) {
            continue;
        }
        block = &e;
        break;
    }
    if (block == nullptr) {
        return false;
    }
    std::vector<uint8_t> raw;
    if (!ReadBlockBytes(*block, db, raw) || raw.size() < kBlendAttrSingleDnaSize) {
        return false;
    }
    Pointer dataPtr;
    std::memcpy(&dataPtr.val, raw.data(), 8);
    if (!db.little) {
        ByteSwap::Swap8(&dataPtr.val);
    }
    if (!dataPtr.val) {
        return false;
    }
    const FileBlockHead *dataBlock = FindBlockExactAddress(dataPtr, db, elem);
    if (dataBlock == nullptr) {
        for (const FileBlockHead &e : db.entries) {
            if (e.address.val != dataPtr.val || e.size < elem) {
                continue;
            }
            if (!BlockInActiveIdScope(e, db)) {
                continue;
            }
            dataBlock = &e;
            break;
        }
    }
    if (dataBlock == nullptr) {
        return false;
    }
    std::vector<uint8_t> payload;
    if (!ReadBlockBytes(*dataBlock, db, payload) || payload.size() < elem) {
        return false;
    }
    out.data.assign(payload.begin(), payload.begin() + static_cast<std::ptrdiff_t>(elem));
    out.size = 1;
    return true;
}

bool LoadAttributesFromStorage(const Mesh &mesh, const FileDatabase &db, std::vector<RawAttr> &out) {
    // attribute_storage is embedded; re-read from the Mesh file block via DNA offsets
    // already converted into mesh.attribute_* fields filled by Convert<Mesh>.
    if (mesh.attribute_dna_num <= 0 || !mesh.attribute_dna_ptr.val) {
        return false;
    }
    const size_t attrStride = kBlendAttrDnaSize; // blender::Attribute (DNA_attribute_types.h)
    const size_t want = static_cast<size_t>(mesh.attribute_dna_num) * attrStride;
    const FileBlockHead *attrsBlock = FindBlockExactAddress(mesh.attribute_dna_ptr, db, want);
    if (attrsBlock == nullptr) {
        // also try matching by num in block header
        for (const FileBlockHead &e : db.entries) {
            if (e.address.val != mesh.attribute_dna_ptr.val ||
                    e.num != static_cast<size_t>(mesh.attribute_dna_num)) {
                continue;
            }
            if (!BlockInActiveIdScope(e, db)) {
                continue;
            }
            attrsBlock = &e;
            break;
        }
    }
    if (attrsBlock == nullptr) {
        return false;
    }
    std::vector<uint8_t> raw;
    if (!ReadBlockBytes(*attrsBlock, db, raw)) {
        return false;
    }
    const size_t n = std::min(static_cast<size_t>(mesh.attribute_dna_num), raw.size() / attrStride);
    out.clear();
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        const uint8_t *base = raw.data() + i * attrStride;
        Pointer namePtr, dataPtr;
        int16_t dataType = 0;
        std::memcpy(&namePtr.val, base + 0, 8);
        std::memcpy(&dataType, base + 8, 2);
        const int8_t domain = static_cast<int8_t>(base[10]);
        const int8_t storage = static_cast<int8_t>(base[11]);
        std::memcpy(&dataPtr.val, base + 16, 8);
        if (!db.little) {
            ByteSwap::Swap8(&namePtr.val);
            ByteSwap::Swap2(&dataType);
            ByteSwap::Swap8(&dataPtr.val);
        }

        RawAttr attr;
        attr.data_type = static_cast<BlendAttrType>(dataType);
        attr.domain = static_cast<BlendAttrDomain>(domain);
        attr.storage = static_cast<BlendAttrStorageType>(storage);

        // name is a DATA string block
        for (const FileBlockHead &e : db.entries) {
            if (e.address.val != namePtr.val || e.size == 0) {
                continue;
            }
            if (!BlockInActiveIdScope(e, db)) {
                continue;
            }
            std::vector<uint8_t> nb;
            if (ReadBlockBytes(e, db, nb)) {
                size_t len = 0;
                while (len < nb.size() && nb[len] != 0) {
                    ++len;
                }
                attr.name.assign(reinterpret_cast<const char *>(nb.data()), len);
            }
            break;
        }

        if (!dataPtr.val) {
            continue;
        }
        int64_t preferred = -1;
        if (attr.domain == BlendAttrDomain::Point) {
            preferred = mesh.totvert;
        } else if (attr.domain == BlendAttrDomain::Corner) {
            preferred = mesh.totloop;
        } else if (attr.domain == BlendAttrDomain::Face) {
            preferred = mesh.totpoly;
        } else if (attr.domain == BlendAttrDomain::Edge) {
            preferred = mesh.totedge;
        }
        if (attr.storage == BlendAttrStorageType::Array) {
            if (!LoadAttributeArrayPayload(dataPtr, attr.data_type, db, attr, preferred)) {
                continue;
            }
        } else if (attr.storage == BlendAttrStorageType::Single) {
            if (!LoadAttributeSinglePayload(dataPtr, attr.data_type, db, attr)) {
                continue;
            }
        } else {
            continue;
        }
        out.push_back(std::move(attr));
    }
    return !out.empty();
}

const RawAttr *FindAttr(const std::vector<RawAttr> &attrs, const char *name, BlendAttrDomain domain, BlendAttrType type) {
    for (const RawAttr &a : attrs) {
        if (a.domain == domain && a.data_type == type && a.name == name) {
            return &a;
        }
    }
    return nullptr;
}

} // namespace

bool HydrateMeshFromAttributeStorage(Mesh &mesh, const FileDatabase &db) {
    if (mesh.totvert <= 0) {
        return false;
    }
    const bool needVerts = mesh.mvert.size() < static_cast<size_t>(mesh.totvert);
    const bool needLoops = mesh.totloop > 0 && mesh.mloop.size() < static_cast<size_t>(mesh.totloop);
    const bool needPolys = mesh.totpoly > 0 && mesh.mpoly.size() < static_cast<size_t>(mesh.totpoly);
    if (!needVerts && !needLoops && !needPolys) {
        return false;
    }

    // ConvertMesh runs after DNA Convert; restore per-ID datamap for DATA lookup.
    struct ActiveIdScopeGuard {
        const FileDatabase &db;
        Pointer prev;
        bool armed;
        ActiveIdScopeGuard(const FileDatabase &d, const Pointer &id) :
                db(d), prev(d.active_id_scope), armed(false) {
            if (id.val) {
                db.active_id_scope = id;
                armed = true;
            }
        }
        ~ActiveIdScopeGuard() {
            if (armed) {
                db.active_id_scope = prev;
            }
        }
    } scope_guard(db, mesh.blend_id_address);

    // Face offsets: Mesh.face_offset_indices (DNA rename of poly_offset_indices), length faces_num+1.
    // See DNA_mesh_types.h and dna_rename_defs.h.
    if (needPolys && mesh.poly_offset_indices.size() < static_cast<size_t>(mesh.totpoly) + 1) {
        if (mesh.poly_offset_ptr.val) {
            std::vector<int> offsets;
            if (ReadIntArrayAtPointer(mesh.poly_offset_ptr, static_cast<size_t>(mesh.totpoly) + 1, db, offsets)) {
                mesh.poly_offset_indices.assign(offsets.begin(), offsets.end());
            }
        }
    }

    std::vector<RawAttr> attrs;
    if (!LoadAttributesFromStorage(mesh, db, attrs)) {
        ASSIMP_LOG_WARN("BLEND: AttributeStorage load failed for mesh (dna_num=",
                mesh.attribute_dna_num, " dna_ptr=", mesh.attribute_dna_ptr.val, ")");
        return false;
    }

    auto clearPartial = [&mesh]() {
        mesh.mvert.clear();
        mesh.mloop.clear();
        mesh.mpoly.clear();
        mesh.poly_offset_indices.clear();
    };

    if (needVerts) {
        const RawAttr *pos = FindAttr(attrs, "position", BlendAttrDomain::Point, BlendAttrType::Float3);
        const bool posSingle = pos && pos->storage == BlendAttrStorageType::Single && pos->size == 1 &&
                pos->data.size() >= 12;
        const bool posArray = pos && pos->size >= mesh.totvert &&
                pos->data.size() >= static_cast<size_t>(mesh.totvert) * 12;
        if (pos == nullptr || (!posSingle && !posArray)) {
            ASSIMP_LOG_WARN("BLEND: missing position attr for hydrate (attrs=", attrs.size(),
                    " pos=", (pos ? pos->size : -1), " totvert=", mesh.totvert, ")");
            clearPartial();
            return false;
        }
        mesh.mvert.resize(static_cast<size_t>(mesh.totvert));
        float singleCo[3] = {0.f, 0.f, 0.f};
        if (posSingle) {
            std::memcpy(singleCo, pos->data.data(), 12);
            if (!db.little) {
                ByteSwap::Swap4(&singleCo[0]);
                ByteSwap::Swap4(&singleCo[1]);
                ByteSwap::Swap4(&singleCo[2]);
            }
        }
        for (int i = 0; i < mesh.totvert; ++i) {
            float co[3];
            if (posSingle) {
                co[0] = singleCo[0];
                co[1] = singleCo[1];
                co[2] = singleCo[2];
            } else {
                std::memcpy(co, pos->data.data() + static_cast<size_t>(i) * 12, 12);
                if (!db.little) {
                    ByteSwap::Swap4(&co[0]);
                    ByteSwap::Swap4(&co[1]);
                    ByteSwap::Swap4(&co[2]);
                }
            }
            mesh.mvert[i].co[0] = co[0];
            mesh.mvert[i].co[1] = co[1];
            mesh.mvert[i].co[2] = co[2];
            mesh.mvert[i].no[0] = 0.f;
            mesh.mvert[i].no[1] = 0.f;
            mesh.mvert[i].no[2] = 1.f;
        }
    }

    if (needLoops && mesh.totloop > 0) {
        // .corner_vert is always an array in Mesh DNA (AttrDomain::Corner); Single is unused.
        const RawAttr *cv = FindAttr(attrs, ".corner_vert", BlendAttrDomain::Corner, BlendAttrType::Int32);
        if (cv == nullptr || cv->size < mesh.totloop || cv->data.size() < static_cast<size_t>(mesh.totloop) * 4) {
            ASSIMP_LOG_WARN("BLEND: missing .corner_vert attr for hydrate (attrs=", attrs.size(),
                    " cv=", (cv ? cv->size : -1), " totloop=", mesh.totloop, ")");
            clearPartial();
            return false;
        }
        mesh.mloop.resize(static_cast<size_t>(mesh.totloop));
        for (int i = 0; i < mesh.totloop; ++i) {
            int v = 0;
            std::memcpy(&v, cv->data.data() + static_cast<size_t>(i) * 4, 4);
            if (!db.little) {
                ByteSwap::Swap4(&v);
            }
            mesh.mloop[i].v = v;
            mesh.mloop[i].e = 0;
        }
    }

    if (needPolys && mesh.totpoly > 0 &&
            mesh.poly_offset_indices.size() >= static_cast<size_t>(mesh.totpoly) + 1) {
        mesh.mpoly.resize(static_cast<size_t>(mesh.totpoly));
        // Optional face material_index (Int32 on Face domain)
        const RawAttr *mat = FindAttr(attrs, "material_index", BlendAttrDomain::Face, BlendAttrType::Int32);
        for (int i = 0; i < mesh.totpoly; ++i) {
            const int start = mesh.poly_offset_indices[static_cast<size_t>(i)];
            const int end = mesh.poly_offset_indices[static_cast<size_t>(i) + 1];
            mesh.mpoly[i].loopstart = start;
            mesh.mpoly[i].totloop = end - start;
            mesh.mpoly[i].mat_nr = 0;
            if (mat != nullptr && mat->size > i && mat->data.size() >= static_cast<size_t>(i + 1) * 4) {
                int mi = 0;
                std::memcpy(&mi, mat->data.data() + static_cast<size_t>(i) * 4, 4);
                if (!db.little) {
                    ByteSwap::Swap4(&mi);
                }
                if (mi >= 0 && (mesh.mat.empty() || static_cast<size_t>(mi) < mesh.mat.size())) {
                    mesh.mpoly[i].mat_nr = static_cast<short>(mi);
                }
            }
        }
    }

    if (mesh.mvert.size() < static_cast<size_t>(mesh.totvert) ||
            (mesh.totloop > 0 && mesh.mloop.size() < static_cast<size_t>(mesh.totloop)) ||
            (mesh.totpoly > 0 && mesh.mpoly.size() < static_cast<size_t>(mesh.totpoly))) {
        clearPartial();
        return false;
    }
    // Reject hydrates whose corner indices are clearly from a twin mesh.
    for (int i = 0; i < mesh.totloop; ++i) {
        if (mesh.mloop[static_cast<size_t>(i)].v < 0 ||
                mesh.mloop[static_cast<size_t>(i)].v >= mesh.totvert) {
            ASSIMP_LOG_WARN("BLEND: hydrated loop index out of range; discarding");
            clearPartial();
            return false;
        }
    }
    return true;
}

namespace {

struct ActiveIdScopeGuard {
    const FileDatabase &db;
    Pointer prev;
    bool armed;
    ActiveIdScopeGuard(const FileDatabase &d, const Pointer &id) :
            db(d), prev(d.active_id_scope), armed(false) {
        if (id.val) {
            db.active_id_scope = id;
            armed = true;
        }
    }
    ~ActiveIdScopeGuard() {
        if (armed) {
            db.active_id_scope = prev;
        }
    }
};

bool LoadRawLayerBytes(const CustomDataLayer &layer, size_t expectCount, size_t stride,
        const FileDatabase &db, std::vector<uint8_t> &out) {
    if (stride == 0 || expectCount == 0) {
        return false;
    }
    if (layer.data) {
        const RawCustomData *raw = static_cast<const RawCustomData *>(layer.data.get());
        if (raw && raw->stride == stride && raw->count >= expectCount &&
                raw->bytes.size() >= expectCount * stride) {
            out.assign(raw->bytes.begin(),
                    raw->bytes.begin() + static_cast<std::ptrdiff_t>(expectCount * stride));
            return true;
        }
    }
    if (!layer.data_ptr.val) {
        return false;
    }
    const size_t want = expectCount * stride;
    const FileBlockHead *block = nullptr;
    for (const FileBlockHead &e : db.entries) {
        if (e.address.val != layer.data_ptr.val) {
            continue;
        }
        if (db.active_id_scope.val && e.owner_id.val != db.active_id_scope.val) {
            continue;
        }
        if (e.size < want) {
            continue;
        }
        block = &e;
        break;
    }
    if (!block) {
        return false;
    }
    out.resize(want);
    const StreamReaderAny::pos pold = db.reader->GetCurrentPos();
    db.reader->SetCurrentPos(block->start);
    db.reader->CopyAndAdvance(out.data(), want);
    db.reader->SetCurrentPos(pold);
    return true;
}

} // namespace

bool HydrateMeshFromCustomData(Mesh &mesh, const FileDatabase &db) {
    if (mesh.totvert <= 0) {
        return false;
    }
    const bool needVerts = mesh.mvert.size() < static_cast<size_t>(mesh.totvert);
    const bool needLoops = mesh.totloop > 0 && mesh.mloop.size() < static_cast<size_t>(mesh.totloop);
    const bool needPolys = mesh.totpoly > 0 && mesh.mpoly.size() < static_cast<size_t>(mesh.totpoly);
    if (!needVerts && !needLoops && !needPolys) {
        return false;
    }

    ActiveIdScopeGuard scope_guard(db, mesh.blend_id_address);

    // Blender 4.x: position in vert_data / vdata, .corner_vert in corner_data / ldata,
    // face_offset_indices on Mesh (DNA rename from poly_offset_indices).
    if (needPolys && mesh.poly_offset_indices.size() < static_cast<size_t>(mesh.totpoly) + 1) {
        if (mesh.poly_offset_ptr.val) {
            std::vector<int> offsets;
            if (ReadIntArrayAtPointer(mesh.poly_offset_ptr, static_cast<size_t>(mesh.totpoly) + 1, db, offsets)) {
                mesh.poly_offset_indices.assign(offsets.begin(), offsets.end());
            }
        }
    }

    auto clearPartial = [&mesh]() {
        mesh.mvert.clear();
        mesh.mloop.clear();
        mesh.mpoly.clear();
        mesh.poly_offset_indices.clear();
    };

    if (needVerts) {
        std::shared_ptr<CustomDataLayer> posLayer =
                getCustomDataLayer(mesh.vdata, CD_PROP_FLOAT3, "position");
        if (!posLayer) {
            posLayer = getCustomDataLayerByName(mesh.vdata, "position");
        }
        // Legacy: CD_MVERT layer still present on some 2.8/2.9 writes.
        if (!posLayer) {
            posLayer = getCustomDataLayer(mesh.vdata, CD_MVERT, "");
            for (const auto &L : mesh.vdata.layers) {
                if (L && L->type == CD_MVERT) {
                    posLayer = L;
                    break;
                }
            }
        }
        if (!posLayer) {
            ASSIMP_LOG_WARN("BLEND: CustomData missing position layer (file_version=",
                    db.blender_file_version, " totvert=", mesh.totvert, ")");
            return false;
        }

        if (posLayer->type == CD_MVERT && posLayer->data) {
            const MVert *mv = static_cast<const MVert *>(posLayer->data.get());
            mesh.mvert.resize(static_cast<size_t>(mesh.totvert));
            for (int i = 0; i < mesh.totvert; ++i) {
                mesh.mvert[static_cast<size_t>(i)] = mv[i];
            }
        } else {
            std::vector<uint8_t> bytes;
            if (!LoadRawLayerBytes(*posLayer, static_cast<size_t>(mesh.totvert), 12, db, bytes)) {
                ASSIMP_LOG_WARN("BLEND: CustomData position payload unreadable");
                return false;
            }
            mesh.mvert.resize(static_cast<size_t>(mesh.totvert));
            for (int i = 0; i < mesh.totvert; ++i) {
                float co[3];
                std::memcpy(co, bytes.data() + static_cast<size_t>(i) * 12, 12);
                if (!db.little) {
                    ByteSwap::Swap4(&co[0]);
                    ByteSwap::Swap4(&co[1]);
                    ByteSwap::Swap4(&co[2]);
                }
                mesh.mvert[static_cast<size_t>(i)].co[0] = co[0];
                mesh.mvert[static_cast<size_t>(i)].co[1] = co[1];
                mesh.mvert[static_cast<size_t>(i)].co[2] = co[2];
                mesh.mvert[static_cast<size_t>(i)].no[0] = 0.f;
                mesh.mvert[static_cast<size_t>(i)].no[1] = 0.f;
                mesh.mvert[static_cast<size_t>(i)].no[2] = 1.f;
            }
        }
    }

    if (needLoops && mesh.totloop > 0) {
        std::shared_ptr<CustomDataLayer> cv =
                getCustomDataLayer(mesh.ldata, CD_PROP_INT32, ".corner_vert");
        if (!cv) {
            cv = getCustomDataLayerByName(mesh.ldata, ".corner_vert");
        }
        if (!cv) {
            // BMesh-era: CD_MLOOP array
            for (const auto &L : mesh.ldata.layers) {
                if (L && L->type == CD_MLOOP && L->data) {
                    const MLoop *ml = static_cast<const MLoop *>(L->data.get());
                    mesh.mloop.resize(static_cast<size_t>(mesh.totloop));
                    for (int i = 0; i < mesh.totloop; ++i) {
                        mesh.mloop[static_cast<size_t>(i)] = ml[i];
                    }
                    cv.reset();
                    break;
                }
            }
        }
        if (mesh.mloop.size() < static_cast<size_t>(mesh.totloop)) {
            if (!cv) {
                ASSIMP_LOG_WARN("BLEND: CustomData missing .corner_vert (file_version=",
                        db.blender_file_version, " totloop=", mesh.totloop, ")");
                clearPartial();
                return false;
            }
            std::vector<uint8_t> bytes;
            if (!LoadRawLayerBytes(*cv, static_cast<size_t>(mesh.totloop), 4, db, bytes)) {
                ASSIMP_LOG_WARN("BLEND: CustomData .corner_vert payload unreadable");
                clearPartial();
                return false;
            }
            mesh.mloop.resize(static_cast<size_t>(mesh.totloop));
            for (int i = 0; i < mesh.totloop; ++i) {
                int v = 0;
                std::memcpy(&v, bytes.data() + static_cast<size_t>(i) * 4, 4);
                if (!db.little) {
                    ByteSwap::Swap4(&v);
                }
                mesh.mloop[static_cast<size_t>(i)].v = v;
                mesh.mloop[static_cast<size_t>(i)].e = 0;
            }
        }
    }

    if (needPolys && mesh.totpoly > 0 &&
            mesh.poly_offset_indices.size() >= static_cast<size_t>(mesh.totpoly) + 1) {
        mesh.mpoly.resize(static_cast<size_t>(mesh.totpoly));
        for (int i = 0; i < mesh.totpoly; ++i) {
            const int start = mesh.poly_offset_indices[static_cast<size_t>(i)];
            const int end = mesh.poly_offset_indices[static_cast<size_t>(i) + 1];
            mesh.mpoly[static_cast<size_t>(i)].loopstart = start;
            mesh.mpoly[static_cast<size_t>(i)].totloop = end - start;
            mesh.mpoly[static_cast<size_t>(i)].mat_nr = 0;
        }
    } else if (needPolys && mesh.totpoly > 0) {
        // BMesh CD_MPOLY layer
        for (const auto &L : mesh.pdata.layers) {
            if (L && L->type == CD_MPOLY && L->data) {
                const MPoly *mp = static_cast<const MPoly *>(L->data.get());
                mesh.mpoly.resize(static_cast<size_t>(mesh.totpoly));
                for (int i = 0; i < mesh.totpoly; ++i) {
                    mesh.mpoly[static_cast<size_t>(i)] = mp[i];
                }
                break;
            }
        }
    }

    if (mesh.mvert.size() < static_cast<size_t>(mesh.totvert) ||
            (mesh.totloop > 0 && mesh.mloop.size() < static_cast<size_t>(mesh.totloop)) ||
            (mesh.totpoly > 0 && mesh.mpoly.size() < static_cast<size_t>(mesh.totpoly))) {
        clearPartial();
        return false;
    }
    for (int i = 0; i < mesh.totloop; ++i) {
        if (mesh.mloop[static_cast<size_t>(i)].v < 0 ||
                mesh.mloop[static_cast<size_t>(i)].v >= mesh.totvert) {
            ASSIMP_LOG_WARN("BLEND: CustomData loop index out of range; discarding");
            clearPartial();
            return false;
        }
    }
    return true;
}

bool HydrateMeshGeometry(Mesh &mesh, const FileDatabase &db) {
    if (mesh.totvert <= 0) {
        return false;
    }
    const bool needVerts = mesh.mvert.size() < static_cast<size_t>(mesh.totvert);
    const bool needLoops = mesh.totloop > 0 && mesh.mloop.size() < static_cast<size_t>(mesh.totloop);
    const bool needPolys = mesh.totpoly > 0 && mesh.mpoly.size() < static_cast<size_t>(mesh.totpoly);
    if (!needVerts && !needLoops && !needPolys) {
        return true;
    }

    const int ver = db.blender_file_version;
    // AttributeStorage: DNA from ~4.5 (ver>=450) / 5.x (ver>=500). Also try when
    // Convert already saw attribute_storage fields (dna_num>0).
    const bool tryAttrStorage = (ver >= kBlendVer_AttributeStorage) || (mesh.attribute_dna_num > 0);
    if (tryAttrStorage) {
        if (HydrateMeshFromAttributeStorage(mesh, db)) {
            return true;
        }
    }
    // CustomData generic attrs: CD_PROP_FLOAT3 era (~2.90+) through 4.x, and as
    // fallback when AttributeStorage is absent/empty on newer files.
    if (ver >= kBlendVer_CustomDataGenericAttrs || ver == 0) {
        if (HydrateMeshFromCustomData(mesh, db)) {
            return true;
        }
    }
    return false;
}

namespace {

bool IsPublicUvName(const std::string &name) {
    // Blender internal attributes use a leading '.'; public UV maps do not.
    return name.empty() || name[0] != '.';
}

std::string UniqueUvName(const std::string &raw, const std::vector<BlendUVLayer> &have) {
    const std::string base = raw.empty() ? "UVMap" : raw;
    auto taken = [&](const std::string &n) {
        for (const BlendUVLayer &L : have) {
            if (L.name == n) {
                return true;
            }
        }
        return false;
    };
    if (!taken(base)) {
        return base;
    }
    for (unsigned i = 1; i < 1000; ++i) {
        char buf[96];
        std::snprintf(buf, sizeof buf, "%s.%03u", base.c_str(), i);
        if (!taken(buf)) {
            return std::string(buf);
        }
    }
    return base + ".x";
}

bool PushUvLayer(std::vector<BlendUVLayer> &out, const std::string &name, std::vector<float> &&uv,
        size_t expectFloats) {
    if (uv.size() < expectFloats) {
        return false;
    }
    if (uv.size() > expectFloats) {
        uv.resize(expectFloats);
    }
    BlendUVLayer layer;
    layer.name = UniqueUvName(name, out);
    layer.uv = std::move(uv);
    out.push_back(std::move(layer));
    return true;
}

bool ReadFloat2LayerUv(const CustomDataLayer &layer, size_t totloop, const FileDatabase &db,
        std::vector<float> &uv) {
    std::vector<uint8_t> bytes;
    if (!LoadRawLayerBytes(layer, totloop, 8, db, bytes)) {
        return false;
    }
    uv.resize(totloop * 2);
    for (size_t i = 0; i < totloop; ++i) {
        float uv2[2];
        std::memcpy(uv2, bytes.data() + i * 8, 8);
        if (!db.little) {
            ByteSwap::Swap4(&uv2[0]);
            ByteSwap::Swap4(&uv2[1]);
        }
        uv[i * 2 + 0] = uv2[0];
        uv[i * 2 + 1] = uv2[1];
    }
    return true;
}

bool ReadMLoopUvLayer(const CustomDataLayer &layer, size_t totloop, std::vector<float> &uv) {
    if (!layer.data) {
        return false;
    }
    const MLoopUV *arr = dynamic_cast<const MLoopUV *>(layer.data.get());
    if (!arr) {
        // shared_ptr holds MLoopUV[] via ElemBase* - dynamic_cast on first element works
        // when Create allocated MLoopUV[]. Fallback: static_cast if type matches.
        arr = static_cast<const MLoopUV *>(layer.data.get());
    }
    if (!arr) {
        return false;
    }
    uv.resize(totloop * 2);
    for (size_t i = 0; i < totloop; ++i) {
        uv[i * 2 + 0] = arr[i].uv[0];
        uv[i * 2 + 1] = arr[i].uv[1];
    }
    return true;
}

} // namespace

unsigned CollectMeshUVLayers(const Mesh &mesh, const FileDatabase &db, std::vector<BlendUVLayer> &out,
        unsigned maxLayers) {
    out.clear();
    if (maxLayers == 0 || mesh.totloop <= 0) {
        return 0;
    }
    if (maxLayers > AI_MAX_NUMBER_OF_TEXTURECOORDS) {
        maxLayers = AI_MAX_NUMBER_OF_TEXTURECOORDS;
    }

    ActiveIdScopeGuard scope_guard(db, mesh.blend_id_address);
    std::vector<BlendUVLayer> found;
    const size_t expect = static_cast<size_t>(mesh.totloop) * 2;

    // 1) CustomData corner layers: CD_MLOOPUV (legacy) + CD_PROP_FLOAT2 (modern UV maps)
    // Track first UV-type group's active_rnd so render-active map can be set 0.
    std::vector<size_t> cdUvFoundIdx;
    int cdUvActiveRnd = -1;
    int cdUvTypeGroup = -1;
    for (const auto &layerPtr : mesh.ldata.layers) {
        if (!layerPtr) {
            continue;
        }
        const CustomDataLayer &layer = *layerPtr;
        const std::string lname = layer.name;
        if (!IsPublicUvName(lname)) {
            continue;
        }
        std::vector<float> uv;
        if (layer.type == CD_MLOOPUV) {
            if (!ReadMLoopUvLayer(layer, static_cast<size_t>(mesh.totloop), uv)) {
                continue;
            }
        } else if (layer.type == CD_PROP_FLOAT2) {
            if (!ReadFloat2LayerUv(layer, static_cast<size_t>(mesh.totloop), db, uv)) {
                continue;
            }
        } else {
            continue;
        }
        if (cdUvTypeGroup < 0) {
            cdUvTypeGroup = layer.type;
            cdUvActiveRnd = layer.active_rnd;
        }
        const size_t before = found.size();
        if (PushUvLayer(found, lname, std::move(uv), expect) && layer.type == cdUvTypeGroup) {
            cdUvFoundIdx.push_back(before);
        }
    }

    // 2) AttributeStorage corner float2 (Blender ~4.5+ / 5.x)
    if (mesh.attribute_dna_num > 0 && mesh.attribute_dna_ptr.val) {
        std::vector<RawAttr> attrs;
        if (LoadAttributesFromStorage(mesh, db, attrs)) {
            for (const RawAttr &a : attrs) {
                if (a.domain != BlendAttrDomain::Corner || a.data_type != BlendAttrType::Float2) {
                    continue;
                }
                if (!IsPublicUvName(a.name)) {
                    continue;
                }
                if (a.data.size() < expect) {
                    continue;
                }
                // Skip if name already collected from CustomData
                bool haveName = false;
                for (const BlendUVLayer &L : found) {
                    if (L.name == a.name) {
                        haveName = true;
                        break;
                    }
                }
                if (haveName) {
                    continue;
                }
                std::vector<float> uv(expect);
                for (size_t i = 0; i < static_cast<size_t>(mesh.totloop); ++i) {
                    float uv2[2];
                    std::memcpy(uv2, a.data.data() + i * 8, 8);
                    if (!db.little) {
                        ByteSwap::Swap4(&uv2[0]);
                        ByteSwap::Swap4(&uv2[1]);
                    }
                    uv[i * 2 + 0] = uv2[0];
                    uv[i * 2 + 1] = uv2[1];
                }
                PushUvLayer(found, a.name, std::move(uv), expect);
            }
        }
    }

    // 3) Classic *mloopuv mirror pointer (active/default UV only)
    if (found.empty() && mesh.mloopuv.size() >= static_cast<size_t>(mesh.totloop)) {
        std::vector<float> uv(expect);
        for (int i = 0; i < mesh.totloop; ++i) {
            uv[static_cast<size_t>(i) * 2 + 0] = mesh.mloopuv[static_cast<size_t>(i)].uv[0];
            uv[static_cast<size_t>(i) * 2 + 1] = mesh.mloopuv[static_cast<size_t>(i)].uv[1];
        }
        PushUvLayer(found, "UVMap", std::move(uv), expect);
    }

    // Blender default/render UV (CustomData active_rnd / Mesh::default_uv_map_name) -> Assimp set 0.
    // Empty ShaderNodeUVMap / TexCoord.UV / unlinked Image Vector resolve to this set.
    if (cdUvActiveRnd >= 0 && static_cast<size_t>(cdUvActiveRnd) < cdUvFoundIdx.size()) {
        const size_t want = cdUvFoundIdx[static_cast<size_t>(cdUvActiveRnd)];
        if (want > 0 && want < found.size()) {
            BlendUVLayer active = std::move(found[want]);
            found.erase(found.begin() + static_cast<std::vector<BlendUVLayer>::difference_type>(want));
            found.insert(found.begin(), std::move(active));
        }
    }

    const char *meshName = (mesh.id.name[0] && mesh.id.name[1]) ? mesh.id.name + 2 : mesh.id.name;
    if (found.size() > 1) {
        std::ostringstream names;
        for (size_t i = 0; i < found.size(); ++i) {
            if (i) {
                names << ", ";
            }
            names << '"' << found[i].name << '"';
        }
        ASSIMP_LOG_WARN("BLEND: mesh '", meshName, "' has ", found.size(),
                " UV maps (Assimp delivers up to ", maxLayers, "): ", names.str());
    }
    if (found.size() > maxLayers) {
        ASSIMP_LOG_WARN("BLEND: mesh '", meshName, "' truncating UV maps from ", found.size(),
                " to ", maxLayers);
        found.resize(maxLayers);
    }

    out = std::move(found);
    return static_cast<unsigned>(out.size());
}

} // namespace Blender
} // namespace Assimp
