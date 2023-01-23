/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

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

----------------------------------------------------------------------
*/

#include "AssetLib/glTF/glTFCommon.h"

#include <assimp/MemoryIOWrapper.h>
#include <assimp/StringUtils.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Base64.hpp>

// clang-format off
#ifdef ASSIMP_ENABLE_DRACO

// Google draco library headers spew many warnings. Bad Google, no cookie
#   if _MSC_VER
#       pragma warning(push)
#       pragma warning(disable : 4018) // Signed/unsigned mismatch
#       pragma warning(disable : 4804) // Unsafe use of type 'bool'
#   elif defined(__clang__)
#       pragma clang diagnostic push
#       pragma clang diagnostic ignored "-Wsign-compare"
#   elif defined(__GNUC__)
#       pragma GCC diagnostic push
#       if (__GNUC__ > 4)
#           pragma GCC diagnostic ignored "-Wbool-compare"
#       endif
#   pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#include "draco/compression/decode.h"
#include "draco/core/decoder_buffer.h"

#if _MSC_VER
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
#ifndef DRACO_MESH_COMPRESSION_SUPPORTED
#   error glTF: KHR_draco_mesh_compression: draco library must have DRACO_MESH_COMPRESSION_SUPPORTED
#endif
#endif
// clang-format on

using namespace Assimp;

namespace glTF2 {
using glTFCommon::FindStringInContext;
using glTFCommon::FindNumberInContext;
using glTFCommon::FindUIntInContext;
using glTFCommon::FindArrayInContext;
using glTFCommon::FindObjectInContext;
using glTFCommon::FindExtensionInContext;
using glTFCommon::MemberOrDefault;
using glTFCommon::ReadMember;
using glTFCommon::FindMember;
using glTFCommon::FindObject;
using glTFCommon::FindUInt;
using glTFCommon::FindArray;
using glTFCommon::FindArray;

namespace {

//
// JSON Value reading helpers
//
inline CustomExtension ReadExtensions(const char *name, Value &obj) {
    CustomExtension ret;
    ret.name = name;
    if (obj.IsObject()) {
        ret.mValues.isPresent = true;
        for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
            auto &val = it->value;
            ret.mValues.value.push_back(ReadExtensions(it->name.GetString(), val));
        }
    } else if (obj.IsArray()) {
        ret.mValues.value.reserve(obj.Size());
        ret.mValues.isPresent = true;
        for (unsigned int i = 0; i < obj.Size(); ++i) {
            ret.mValues.value.push_back(ReadExtensions(name, obj[i]));
        }
    } else if (obj.IsNumber()) {
        if (obj.IsUint64()) {
            ret.mUint64Value.value = obj.GetUint64();
            ret.mUint64Value.isPresent = true;
        } else if (obj.IsInt64()) {
            ret.mInt64Value.value = obj.GetInt64();
            ret.mInt64Value.isPresent = true;
        } else if (obj.IsDouble()) {
            ret.mDoubleValue.value = obj.GetDouble();
            ret.mDoubleValue.isPresent = true;
        }
    } else if (obj.IsString()) {
        ReadValue(obj, ret.mStringValue);
        ret.mStringValue.isPresent = true;
    } else if (obj.IsBool()) {
        ret.mBoolValue.value = obj.GetBool();
        ret.mBoolValue.isPresent = true;
    }
    return ret;
}

inline void CopyData(size_t count, const uint8_t *src, size_t src_stride,
        uint8_t *dst, size_t dst_stride) {
    if (src_stride == dst_stride) {
        memcpy(dst, src, count * src_stride);
        return;
    }

    size_t sz = std::min(src_stride, dst_stride);
    for (size_t i = 0; i < count; ++i) {
        memcpy(dst, src, sz);
        if (sz < dst_stride) {
            memset(dst + sz, 0, dst_stride - sz);
        }
        src += src_stride;
        dst += dst_stride;
    }
}

void SetVector(vec4 &v, const float (&in)[4]) {
    v[0] = in[0];
    v[1] = in[1];
    v[2] = in[2];
    v[3] = in[3];
}

void SetVector(vec3 &v, const float (&in)[3]) {
    v[0] = in[0];
    v[1] = in[1];
    v[2] = in[2];
}

template <int N>
inline int Compare(const char *attr, const char (&str)[N]) {
    return (strncmp(attr, str, N - 1) == 0) ? N - 1 : 0;
}

#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4706)
#endif // _MSC_VER

inline bool GetAttribVector(Mesh::Primitive &p, const char *attr, Mesh::AccessorList *&v, int &pos) {
    if ((pos = Compare(attr, "POSITION"))) {
        v = &(p.attributes.position);
    } else if ((pos = Compare(attr, "NORMAL"))) {
        v = &(p.attributes.normal);
    } else if ((pos = Compare(attr, "TANGENT"))) {
        v = &(p.attributes.tangent);
    } else if ((pos = Compare(attr, "TEXCOORD"))) {
        v = &(p.attributes.texcoord);
    } else if ((pos = Compare(attr, "COLOR"))) {
        v = &(p.attributes.color);
    } else if ((pos = Compare(attr, "JOINTS"))) {
        v = &(p.attributes.joint);
    } else if ((pos = Compare(attr, "JOINTMATRIX"))) {
        v = &(p.attributes.jointmatrix);
    } else if ((pos = Compare(attr, "WEIGHTS"))) {
        v = &(p.attributes.weight);
    } else
        return false;
    return true;
}

inline bool GetAttribTargetVector(Mesh::Primitive &p, const int targetIndex, const char *attr, Mesh::AccessorList *&v, int &pos) {
    if ((pos = Compare(attr, "POSITION"))) {
        v = &(p.targets[targetIndex].position);
    } else if ((pos = Compare(attr, "NORMAL"))) {
        v = &(p.targets[targetIndex].normal);
    } else if ((pos = Compare(attr, "TANGENT"))) {
        v = &(p.targets[targetIndex].tangent);
    } else
        return false;
    return true;
}

} // namespace

inline Value *Object::FindString(Value &val, const char *memberId) {
    return FindStringInContext(val, memberId, id.c_str(), name.c_str());
}

inline Value *Object::FindNumber(Value &val, const char *memberId) {
    return FindNumberInContext(val, memberId, id.c_str(), name.c_str());
}

inline Value *Object::FindUInt(Value &val, const char *memberId) {
    return FindUIntInContext(val, memberId, id.c_str(), name.c_str());
}

inline Value *Object::FindArray(Value &val, const char *memberId) {
    return FindArrayInContext(val, memberId, id.c_str(), name.c_str());
}

inline Value *Object::FindObject(Value &val, const char *memberId) {
    return FindObjectInContext(val, memberId, id.c_str(), name.c_str());
}

inline Value *Object::FindExtension(Value &val, const char *extensionId) {
    return FindExtensionInContext(val, extensionId, id.c_str(), name.c_str());
}

inline void Object::ReadExtensions(Value &val) {
    if (Value *curExtensions = FindObject(val, "extensions")) {
        this->customExtensions = glTF2::ReadExtensions("extensions", *curExtensions);
    }
}

inline void Object::ReadExtras(Value &val) {
    if (Value *curExtras = FindObject(val, "extras")) {
        this->extras = glTF2::ReadExtensions("extras", *curExtras);
    }
}

#ifdef ASSIMP_ENABLE_DRACO

template <typename T>
inline void CopyFaceIndex_Draco(Buffer &decodedIndexBuffer, const draco::Mesh &draco_mesh) {
    const size_t faceStride = sizeof(T) * 3;
    for (draco::FaceIndex f(0); f < draco_mesh.num_faces(); ++f) {
        const draco::Mesh::Face &face = draco_mesh.face(f);
        T indices[3] = { static_cast<T>(face[0].value()), static_cast<T>(face[1].value()), static_cast<T>(face[2].value()) };
        memcpy(decodedIndexBuffer.GetPointer() + (f.value() * faceStride), &indices[0], faceStride);
    }
}

inline void SetDecodedIndexBuffer_Draco(const draco::Mesh &dracoMesh, Mesh::Primitive &prim) {
    if (!prim.indices || dracoMesh.num_faces() == 0)
        return;

    // Create a decoded Index buffer (if there is one)
    size_t componentBytes = prim.indices->GetBytesPerComponent();

    std::unique_ptr<Buffer> decodedIndexBuffer(new Buffer());
    decodedIndexBuffer->Grow(dracoMesh.num_faces() * 3 * componentBytes);

    // If accessor uses the same size as draco implementation, copy the draco buffer directly

    // Usually uint32_t but shouldn't assume
    if (sizeof(dracoMesh.face(draco::FaceIndex(0))[0]) == componentBytes) {
        memcpy(decodedIndexBuffer->GetPointer(), &dracoMesh.face(draco::FaceIndex(0))[0], decodedIndexBuffer->byteLength);
        return;
    }

    // Not same size, convert
    switch (componentBytes) {
    case sizeof(uint32_t):
        CopyFaceIndex_Draco<uint32_t>(*decodedIndexBuffer, dracoMesh);
        break;
    case sizeof(uint16_t):
        CopyFaceIndex_Draco<uint16_t>(*decodedIndexBuffer, dracoMesh);
        break;
    case sizeof(uint8_t):
        CopyFaceIndex_Draco<uint8_t>(*decodedIndexBuffer, dracoMesh);
        break;
    default:
        ai_assert(false);
        break;
    }

    // Assign this alternate data buffer to the accessor
    prim.indices->decodedBuffer.swap(decodedIndexBuffer);
}

template <typename T>
static bool GetAttributeForAllPoints_Draco(const draco::Mesh &dracoMesh,
        const draco::PointAttribute &dracoAttribute,
        Buffer &outBuffer) {
    size_t byteOffset = 0;
    T values[4] = { 0, 0, 0, 0 };
    for (draco::PointIndex i(0); i < dracoMesh.num_points(); ++i) {
        const draco::AttributeValueIndex val_index = dracoAttribute.mapped_index(i);
        if (!dracoAttribute.ConvertValue<T>(val_index, dracoAttribute.num_components(), values)) {
            return false;
        }

        memcpy(outBuffer.GetPointer() + byteOffset, &values[0], sizeof(T) * dracoAttribute.num_components());
        byteOffset += sizeof(T) * dracoAttribute.num_components();
    }

    return true;
}

inline void SetDecodedAttributeBuffer_Draco(const draco::Mesh &dracoMesh, uint32_t dracoAttribId, Accessor &accessor) {
    // Create decoded buffer
    const draco::PointAttribute *pDracoAttribute = dracoMesh.GetAttributeByUniqueId(dracoAttribId);
    if (pDracoAttribute == nullptr) {
        throw DeadlyImportError("GLTF: Invalid draco attribute id: ", dracoAttribId);
    }

    size_t componentBytes = accessor.GetBytesPerComponent();

    std::unique_ptr<Buffer> decodedAttribBuffer(new Buffer());
    decodedAttribBuffer->Grow(dracoMesh.num_points() * pDracoAttribute->num_components() * componentBytes);

    switch (accessor.componentType) {
    case ComponentType_BYTE:
        GetAttributeForAllPoints_Draco<int8_t>(dracoMesh, *pDracoAttribute, *decodedAttribBuffer);
        break;
    case ComponentType_UNSIGNED_BYTE:
        GetAttributeForAllPoints_Draco<uint8_t>(dracoMesh, *pDracoAttribute, *decodedAttribBuffer);
        break;
    case ComponentType_SHORT:
        GetAttributeForAllPoints_Draco<int16_t>(dracoMesh, *pDracoAttribute, *decodedAttribBuffer);
        break;
    case ComponentType_UNSIGNED_SHORT:
        GetAttributeForAllPoints_Draco<uint16_t>(dracoMesh, *pDracoAttribute, *decodedAttribBuffer);
        break;
    case ComponentType_UNSIGNED_INT:
        GetAttributeForAllPoints_Draco<uint32_t>(dracoMesh, *pDracoAttribute, *decodedAttribBuffer);
        break;
    case ComponentType_FLOAT:
        GetAttributeForAllPoints_Draco<float>(dracoMesh, *pDracoAttribute, *decodedAttribBuffer);
        break;
    default:
        ai_assert(false);
        break;
    }

    // Assign this alternate data buffer to the accessor
    accessor.decodedBuffer.swap(decodedAttribBuffer);
}

#endif // ASSIMP_ENABLE_DRACO

//
// LazyDict methods
//

template <class T>
inline LazyDict<T>::LazyDict(Asset &asset, const char *dictId, const char *extId) :
        mDictId(dictId),
        mExtId(extId),
        mDict(nullptr),
        mAsset(asset) {
    asset.mDicts.push_back(this); // register to the list of dictionaries
}

template <class T>
inline LazyDict<T>::~LazyDict() {
    for (size_t i = 0; i < mObjs.size(); ++i) {
        delete mObjs[i];
    }
}

template <class T>
inline void LazyDict<T>::AttachToDocument(Document &doc) {
    Value *container = nullptr;
    const char *context = nullptr;

    if (mExtId) {
        if (Value *exts = FindObject(doc, "extensions")) {
            container = FindObjectInContext(*exts, mExtId, "extensions");
            context = mExtId;
        }
    } else {
        container = &doc;
        context = "the document";
    }

    if (container) {
        mDict = FindArrayInContext(*container, mDictId, context);
    }
}

template <class T>
inline void LazyDict<T>::DetachFromDocument() {
    mDict = nullptr;
}

template <class T>
unsigned int LazyDict<T>::Remove(const char *id) {
    id = T::TranslateId(mAsset, id);

    typename IdDict::iterator objIt = mObjsById.find(id);

    if (objIt == mObjsById.end()) {
        throw DeadlyExportError("GLTF: Object with id \"" + std::string(id) + "\" is not found");
    }

    const unsigned int index = objIt->second;

    mAsset.mUsedIds[id] = false;
    mObjsById.erase(id);
    mObjsByOIndex.erase(index);
    delete mObjs[index];
    mObjs.erase(mObjs.begin() + index);

    //update index of object in mObjs;
    for (unsigned int i = index; i < mObjs.size(); ++i) {
        T *obj = mObjs[i];

        obj->index = i;
    }

    for (IdDict::iterator it = mObjsById.begin(); it != mObjsById.end(); ++it) {
        if (it->second <= index) {
            continue;
        }

        mObjsById[it->first] = it->second - 1;
    }

    for (Dict::iterator it = mObjsByOIndex.begin(); it != mObjsByOIndex.end(); ++it) {
        if (it->second <= index) {
            continue;
        }

        mObjsByOIndex[it->first] = it->second - 1;
    }

    return index;
}

template <class T>
Ref<T> LazyDict<T>::Retrieve(unsigned int i) {

    typename Dict::iterator it = mObjsByOIndex.find(i);
    if (it != mObjsByOIndex.end()) { // already created?
        return Ref<T>(mObjs, it->second);
    }

    // read it from the JSON object
    if (!mDict) {
        throw DeadlyImportError("GLTF: Missing section \"", mDictId, "\"");
    }

    if (!mDict->IsArray()) {
        throw DeadlyImportError("GLTF: Field \"", mDictId, "\"  is not an array");
    }

    if (i >= mDict->Size()) {
        throw DeadlyImportError("GLTF: Array index ", i, " is out of bounds (", mDict->Size(), ") for \"", mDictId, "\"");
    }

    Value &obj = (*mDict)[i];

    if (!obj.IsObject()) {
        throw DeadlyImportError("GLTF: Object at index ", i, " in array \"", mDictId, "\" is not a JSON object");
    }

    if (mRecursiveReferenceCheck.find(i) != mRecursiveReferenceCheck.end()) {
        throw DeadlyImportError("GLTF: Object at index ", i, " in array \"", mDictId, "\" has recursive reference to itself");
    }
    mRecursiveReferenceCheck.insert(i);

    // Unique ptr prevents memory leak in case of Read throws an exception
    auto inst = std::unique_ptr<T>(new T());
    // Try to make this human readable so it can be used in error messages.
    inst->id = std::string(mDictId) + "[" + ai_to_string(i) + "]";
    inst->oIndex = i;
    ReadMember(obj, "name", inst->name);
    inst->Read(obj, mAsset);
    inst->ReadExtensions(obj);
    inst->ReadExtras(obj);

    Ref<T> result = Add(inst.release());
    mRecursiveReferenceCheck.erase(i);
    return result;
}

template <class T>
Ref<T> LazyDict<T>::Get(unsigned int i) {
    return Ref<T>(mObjs, i);
}

template <class T>
Ref<T> LazyDict<T>::Get(const char *id) {
    id = T::TranslateId(mAsset, id);

    typename IdDict::iterator it = mObjsById.find(id);
    if (it != mObjsById.end()) { // already created?
        return Ref<T>(mObjs, it->second);
    }

    return Ref<T>();
}

template <class T>
Ref<T> LazyDict<T>::Add(T *obj) {
    unsigned int idx = unsigned(mObjs.size());
    mObjs.push_back(obj);
    mObjsByOIndex[obj->oIndex] = idx;
    mObjsById[obj->id] = idx;
    mAsset.mUsedIds[obj->id] = true;
    return Ref<T>(mObjs, idx);
}

template <class T>
Ref<T> LazyDict<T>::Create(const char *id) {
    Asset::IdMap::iterator it = mAsset.mUsedIds.find(id);
    if (it != mAsset.mUsedIds.end()) {
        throw DeadlyImportError("GLTF: two objects with the same ID exist");
    }
    T *inst = new T();
    unsigned int idx = unsigned(mObjs.size());
    inst->id = id;
    inst->index = idx;
    inst->oIndex = idx;
    return Add(inst);
}

//
// glTF dictionary objects methods
//
inline Buffer::Buffer() :
        byteLength(0),
        type(Type_arraybuffer),
        EncodedRegion_Current(nullptr),
        mIsSpecial(false) {}

inline Buffer::~Buffer() {
    for (SEncodedRegion *reg : EncodedRegion_List)
        delete reg;
}

inline const char *Buffer::TranslateId(Asset & /*r*/, const char *id) {
    return id;
}

inline void Buffer::Read(Value &obj, Asset &r) {
    size_t statedLength = MemberOrDefault<size_t>(obj, "byteLength", 0);
    byteLength = statedLength;

    Value *it = FindString(obj, "uri");
    if (!it) {
        if (statedLength > 0) {
            throw DeadlyImportError("GLTF: buffer with non-zero length missing the \"uri\" attribute");
        }
        return;
    }

    const char *uri = it->GetString();

    glTFCommon::Util::DataURI dataURI;
    if (ParseDataURI(uri, it->GetStringLength(), dataURI)) {
        if (dataURI.base64) {
            uint8_t *data = nullptr;
            this->byteLength = Base64::Decode(dataURI.data, dataURI.dataLength, data);
            this->mData.reset(data, std::default_delete<uint8_t[]>());

            if (statedLength > 0 && this->byteLength != statedLength) {
                throw DeadlyImportError("GLTF: buffer \"", id, "\", expected ", ai_to_string(statedLength),
                        " bytes, but found ", ai_to_string(dataURI.dataLength));
            }
        } else { // assume raw data
            if (statedLength != dataURI.dataLength) {
                throw DeadlyImportError("GLTF: buffer \"", id, "\", expected ", ai_to_string(statedLength),
                        " bytes, but found ", ai_to_string(dataURI.dataLength));
            }

            this->mData.reset(new uint8_t[dataURI.dataLength], std::default_delete<uint8_t[]>());
            memcpy(this->mData.get(), dataURI.data, dataURI.dataLength);
        }
    } else { // Local file
        if (byteLength > 0) {
            std::string dir = !r.mCurrentAssetDir.empty() ? (r.mCurrentAssetDir.back() == '/' ? r.mCurrentAssetDir : r.mCurrentAssetDir + '/') : "";

            IOStream *file = r.OpenFile(dir + uri, "rb");
            if (file) {
                bool ok = LoadFromStream(*file, byteLength);
                delete file;

                if (!ok)
                    throw DeadlyImportError("GLTF: error while reading referenced file \"", uri, "\"");
            } else {
                throw DeadlyImportError("GLTF: could not open referenced file \"", uri, "\"");
            }
        }
    }
}

inline bool Buffer::LoadFromStream(IOStream &stream, size_t length, size_t baseOffset) {
    byteLength = length ? length : stream.FileSize();

    if (byteLength > stream.FileSize()) {
        throw DeadlyImportError("GLTF: Invalid byteLength exceeds size of actual data.");
    }

    if (baseOffset) {
        stream.Seek(baseOffset, aiOrigin_SET);
    }

    mData.reset(new uint8_t[byteLength], std::default_delete<uint8_t[]>());

    if (stream.Read(mData.get(), byteLength, 1) != 1) {
        return false;
    }
    return true;
}

inline void Buffer::EncodedRegion_Mark(const size_t pOffset, const size_t pEncodedData_Length, uint8_t *pDecodedData, const size_t pDecodedData_Length, const std::string &pID) {
    // Check pointer to data
    if (pDecodedData == nullptr) throw DeadlyImportError("GLTF: for marking encoded region pointer to decoded data must be provided.");

    // Check offset
    if (pOffset > byteLength) {
        const uint8_t val_size = 32;

        char val[val_size];

        ai_snprintf(val, val_size, AI_SIZEFMT, pOffset);
        throw DeadlyImportError("GLTF: incorrect offset value (", val, ") for marking encoded region.");
    }

    // Check length
    if ((pOffset + pEncodedData_Length) > byteLength) {
        const uint8_t val_size = 64;

        char val[val_size];

        ai_snprintf(val, val_size, AI_SIZEFMT "/" AI_SIZEFMT, pOffset, pEncodedData_Length);
        throw DeadlyImportError("GLTF: encoded region with offset/length (", val, ") is out of range.");
    }

    // Add new region
    EncodedRegion_List.push_back(new SEncodedRegion(pOffset, pEncodedData_Length, pDecodedData, pDecodedData_Length, pID));
    // And set new value for "byteLength"
    byteLength += (pDecodedData_Length - pEncodedData_Length);
}

inline void Buffer::EncodedRegion_SetCurrent(const std::string &pID) {
    if ((EncodedRegion_Current != nullptr) && (EncodedRegion_Current->ID == pID)) {
        return;
    }

    for (SEncodedRegion *reg : EncodedRegion_List) {
        if (reg->ID == pID) {
            EncodedRegion_Current = reg;
            return;
        }
    }

    throw DeadlyImportError("GLTF: EncodedRegion with ID: \"", pID, "\" not found.");
}

inline bool Buffer::ReplaceData(const size_t pBufferData_Offset, const size_t pBufferData_Count, const uint8_t *pReplace_Data, const size_t pReplace_Count) {

    if ((pBufferData_Count == 0) || (pReplace_Count == 0) || (pReplace_Data == nullptr)) {
        return false;
    }

    const size_t new_data_size = byteLength + pReplace_Count - pBufferData_Count;
    uint8_t *new_data = new uint8_t[new_data_size];
    // Copy data which place before replacing part.
    ::memcpy(new_data, mData.get(), pBufferData_Offset);
    // Copy new data.
    ::memcpy(&new_data[pBufferData_Offset], pReplace_Data, pReplace_Count);
    // Copy data which place after replacing part.
    ::memcpy(&new_data[pBufferData_Offset + pReplace_Count], &mData.get()[pBufferData_Offset + pBufferData_Count], pBufferData_Offset);
    // Apply new data
    mData.reset(new_data, std::default_delete<uint8_t[]>());
    byteLength = new_data_size;

    return true;
}

inline bool Buffer::ReplaceData_joint(const size_t pBufferData_Offset, const size_t pBufferData_Count, const uint8_t *pReplace_Data, const size_t pReplace_Count) {
    if ((pBufferData_Count == 0) || (pReplace_Count == 0) || (pReplace_Data == nullptr)) {
        return false;
    }

    const size_t new_data_size = byteLength + pReplace_Count - pBufferData_Count;
    uint8_t *new_data = new uint8_t[new_data_size];
    // Copy data which place before replacing part.
    memcpy(new_data, mData.get(), pBufferData_Offset);
    // Copy new data.
    memcpy(&new_data[pBufferData_Offset], pReplace_Data, pReplace_Count);
    // Copy data which place after replacing part.
    memcpy(&new_data[pBufferData_Offset + pReplace_Count], &mData.get()[pBufferData_Offset + pBufferData_Count], new_data_size - (pBufferData_Offset + pReplace_Count));
    // Apply new data
    mData.reset(new_data, std::default_delete<uint8_t[]>());
    byteLength = new_data_size;

    return true;
}

inline size_t Buffer::AppendData(uint8_t *data, size_t length) {
    const size_t offset = this->byteLength;

    // Force alignment to 4 bits
    const size_t paddedLength = (length + 3) & ~3;
    Grow(paddedLength);
    memcpy(mData.get() + offset, data, length);
    memset(mData.get() + offset + length, 0, paddedLength - length);
    return offset;
}

inline void Buffer::Grow(size_t amount) {
    if (amount <= 0) {
        return;
    }

    // Capacity is big enough
    if (capacity >= byteLength + amount) {
        byteLength += amount;
        return;
    }

    // Just allocate data which we need
    capacity = byteLength + amount;

    uint8_t *b = new uint8_t[capacity];
    if (nullptr != mData) {
        memcpy(b, mData.get(), byteLength);
    }
    mData.reset(b, std::default_delete<uint8_t[]>());
    byteLength += amount;
}

//
// struct BufferView
//
inline void BufferView::Read(Value &obj, Asset &r) {
    if (Value *bufferVal = FindUInt(obj, "buffer")) {
        buffer = r.buffers.Retrieve(bufferVal->GetUint());
    }

    if (!buffer) {
        throw DeadlyImportError("GLTF: Buffer view without valid buffer.");
    }

    byteOffset = MemberOrDefault(obj, "byteOffset", size_t(0));
    byteLength = MemberOrDefault(obj, "byteLength", size_t(0));
    byteStride = MemberOrDefault(obj, "byteStride", 0u);

    // Check length
    if ((byteOffset + byteLength) > buffer->byteLength) {
        throw DeadlyImportError("GLTF: Buffer view with offset/length (", byteOffset, "/", byteLength, ") is out of range.");
    }
}

inline uint8_t *BufferView::GetPointer(size_t accOffset) {
    if (!buffer) {
        return nullptr;
    }
    uint8_t *basePtr = buffer->GetPointer();
    if (!basePtr) {
        return nullptr;
    }

    size_t offset = accOffset + byteOffset;
    if (buffer->EncodedRegion_Current != nullptr) {
        const size_t begin = buffer->EncodedRegion_Current->Offset;
        const size_t end = begin + buffer->EncodedRegion_Current->DecodedData_Length;
        if ((offset >= begin) && (offset < end)) {
            return &buffer->EncodedRegion_Current->DecodedData[offset - begin];
        }
    }

    return basePtr + offset;
}

//
// struct Accessor
//
inline void Accessor::Sparse::PopulateData(size_t numBytes, uint8_t *bytes) {
    if (bytes) {
        data.assign(bytes, bytes + numBytes);
    } else {
        data.resize(numBytes, 0x00);
    }
}

inline void Accessor::Sparse::PatchData(unsigned int elementSize) {
    uint8_t *pIndices = indices->GetPointer(indicesByteOffset);
    const unsigned int indexSize = int(ComponentTypeSize(indicesType));
    uint8_t *indicesEnd = pIndices + count * indexSize;

    uint8_t *pValues = values->GetPointer(valuesByteOffset);
    while (pIndices != indicesEnd) {
        size_t offset;
        switch (indicesType) {
        case ComponentType_UNSIGNED_BYTE:
            offset = *pIndices;
            break;
        case ComponentType_UNSIGNED_SHORT:
            offset = *reinterpret_cast<uint16_t *>(pIndices);
            break;
        case ComponentType_UNSIGNED_INT:
            offset = *reinterpret_cast<uint32_t *>(pIndices);
            break;
        default:
            // have fun with float and negative values from signed types as indices.
            throw DeadlyImportError("Unsupported component type in index.");
        }

        offset *= elementSize;

        if (offset + elementSize > data.size()) {
            throw DeadlyImportError("Invalid sparse accessor. Byte offset for patching points outside allocated memory.");
        }

        std::memcpy(data.data() + offset, pValues, elementSize);

        pValues += elementSize;
        pIndices += indexSize;
    }
}

inline void Accessor::Read(Value &obj, Asset &r) {
    if (Value *bufferViewVal = FindUInt(obj, "bufferView")) {
        bufferView = r.bufferViews.Retrieve(bufferViewVal->GetUint());
    }

    byteOffset = MemberOrDefault(obj, "byteOffset", size_t(0));
    componentType = MemberOrDefault(obj, "componentType", ComponentType_BYTE);
    {
        const Value *countValue = FindUInt(obj, "count");
        if (!countValue) {
            throw DeadlyImportError("A count value is required, when reading ", id.c_str(), name.empty() ? "" : " (" + name + ")");
        }
        count = countValue->GetUint();
    }

    const char *typestr;
    type = ReadMember(obj, "type", typestr) ? AttribType::FromString(typestr) : AttribType::SCALAR;

    if (bufferView) {
        // Check length
        unsigned long long byteLength = (unsigned long long)GetBytesPerComponent() * (unsigned long long)count;

        // handle integer overflow
        if (byteLength < count) {
            throw DeadlyImportError("GLTF: Accessor with offset/count (", byteOffset, "/", count, ") is out of range.");
        }

        if ((byteOffset + byteLength) > bufferView->byteLength || (bufferView->byteOffset + byteOffset + byteLength) > bufferView->buffer->byteLength) {
            throw DeadlyImportError("GLTF: Accessor with offset/length (", byteOffset, "/", byteLength, ") is out of range.");
        }
    }

    if (Value *sparseValue = FindObject(obj, "sparse")) {
        sparse.reset(new Sparse);
        // count
        ReadMember(*sparseValue, "count", sparse->count);

        // indices
        if (Value *indicesValue = FindObject(*sparseValue, "indices")) {
            //indices bufferView
            Value *indiceViewID = FindUInt(*indicesValue, "bufferView");
            sparse->indices = r.bufferViews.Retrieve(indiceViewID->GetUint());
            //indices byteOffset
            sparse->indicesByteOffset = MemberOrDefault(*indicesValue, "byteOffset", size_t(0));
            //indices componentType
            sparse->indicesType = MemberOrDefault(*indicesValue, "componentType", ComponentType_BYTE);
            //sparse->indices->Read(*indicesValue, r);
        } else {
            // indicesType
            sparse->indicesType = MemberOrDefault(*sparseValue, "componentType", ComponentType_UNSIGNED_SHORT);
        }

        // value
        if (Value *valuesValue = FindObject(*sparseValue, "values")) {
            //value bufferView
            Value *valueViewID = FindUInt(*valuesValue, "bufferView");
            sparse->values = r.bufferViews.Retrieve(valueViewID->GetUint());
            //value byteOffset
            sparse->valuesByteOffset = MemberOrDefault(*valuesValue, "byteOffset", size_t(0));
            //sparse->values->Read(*valuesValue, r);
        }


        const unsigned int elementSize = GetElementSize();
        const size_t dataSize = count * elementSize;
        sparse->PopulateData(dataSize, bufferView ? bufferView->GetPointer(byteOffset) : nullptr);
        sparse->PatchData(elementSize);
    }
}

inline unsigned int Accessor::GetNumComponents() {
    return AttribType::GetNumComponents(type);
}

inline unsigned int Accessor::GetBytesPerComponent() {
    return int(ComponentTypeSize(componentType));
}

inline unsigned int Accessor::GetElementSize() {
    return GetNumComponents() * GetBytesPerComponent();
}

inline uint8_t *Accessor::GetPointer() {
    if (decodedBuffer)
        return decodedBuffer->GetPointer();

    if (sparse)
        return sparse->data.data();

    if (!bufferView || !bufferView->buffer) return nullptr;
    uint8_t *basePtr = bufferView->buffer->GetPointer();
    if (!basePtr) return nullptr;

    size_t offset = byteOffset + bufferView->byteOffset;

    // Check if region is encoded.
    if (bufferView->buffer->EncodedRegion_Current != nullptr) {
        const size_t begin = bufferView->buffer->EncodedRegion_Current->Offset;
        const size_t end = begin + bufferView->buffer->EncodedRegion_Current->DecodedData_Length;

        if ((offset >= begin) && (offset < end))
            return &bufferView->buffer->EncodedRegion_Current->DecodedData[offset - begin];
    }

    return basePtr + offset;
}

inline size_t Accessor::GetStride() {
    // Decoded buffer is always packed
    if (decodedBuffer)
        return GetElementSize();

    // Sparse and normal bufferView
    return (bufferView && bufferView->byteStride ? bufferView->byteStride : GetElementSize());
}

inline size_t Accessor::GetMaxByteSize() {
    if (decodedBuffer)
        return decodedBuffer->byteLength;

    return (bufferView ? bufferView->byteLength : sparse->data.size());
}

template <class T>
void Accessor::ExtractData(T *&outData) {
    uint8_t *data = GetPointer();
    if (!data) {
        throw DeadlyImportError("GLTF2: data is null when extracting data from ", getContextForErrorMessages(id, name));
    }

    const size_t elemSize = GetElementSize();
    const size_t totalSize = elemSize * count;

    const size_t stride = GetStride();

    const size_t targetElemSize = sizeof(T);

    if (elemSize > targetElemSize) {
        throw DeadlyImportError("GLTF: elemSize ", elemSize, " > targetElemSize ", targetElemSize, " in ", getContextForErrorMessages(id, name));
    }

    const size_t maxSize = GetMaxByteSize();
    if (count * stride > maxSize) {
        throw DeadlyImportError("GLTF: count*stride ", (count * stride), " > maxSize ", maxSize, " in ", getContextForErrorMessages(id, name));
    }

    outData = new T[count];
    if (stride == elemSize && targetElemSize == elemSize) {
        memcpy(outData, data, totalSize);
    } else {
        for (size_t i = 0; i < count; ++i) {
            memcpy(outData + i, data + i * stride, elemSize);
        }
    }
}

inline void Accessor::WriteData(size_t _count, const void *src_buffer, size_t src_stride) {
    uint8_t *buffer_ptr = bufferView->buffer->GetPointer();
    size_t offset = byteOffset + bufferView->byteOffset;

    size_t dst_stride = GetNumComponents() * GetBytesPerComponent();

    const uint8_t *src = reinterpret_cast<const uint8_t *>(src_buffer);
    uint8_t *dst = reinterpret_cast<uint8_t *>(buffer_ptr + offset);

    ai_assert(dst + _count * dst_stride <= buffer_ptr + bufferView->buffer->byteLength);
    CopyData(_count, src, src_stride, dst, dst_stride);
}

inline void Accessor::WriteSparseValues(size_t _count, const void *src_data, size_t src_dataStride) {
    if (!sparse)
        return;

    // values
    uint8_t *value_buffer_ptr = sparse->values->buffer->GetPointer();
    size_t value_offset = sparse->valuesByteOffset + sparse->values->byteOffset;
    size_t value_dst_stride = GetNumComponents() * GetBytesPerComponent();
    const uint8_t *value_src = reinterpret_cast<const uint8_t *>(src_data);
    uint8_t *value_dst = reinterpret_cast<uint8_t *>(value_buffer_ptr + value_offset);
    ai_assert(value_dst + _count * value_dst_stride <= value_buffer_ptr + sparse->values->buffer->byteLength);
    CopyData(_count, value_src, src_dataStride, value_dst, value_dst_stride);
}

inline void Accessor::WriteSparseIndices(size_t _count, const void *src_idx, size_t src_idxStride) {
    if (!sparse)
        return;

    // indices
    uint8_t *indices_buffer_ptr = sparse->indices->buffer->GetPointer();
    size_t indices_offset = sparse->indicesByteOffset + sparse->indices->byteOffset;
    size_t indices_dst_stride = 1 * sizeof(unsigned short);
    const uint8_t *indices_src = reinterpret_cast<const uint8_t *>(src_idx);
    uint8_t *indices_dst = reinterpret_cast<uint8_t *>(indices_buffer_ptr + indices_offset);
    ai_assert(indices_dst + _count * indices_dst_stride <= indices_buffer_ptr + sparse->indices->buffer->byteLength);
    CopyData(_count, indices_src, src_idxStride, indices_dst, indices_dst_stride);
}

inline Accessor::Indexer::Indexer(Accessor &acc) :
    accessor(acc),
    data(acc.GetPointer()),
    elemSize(acc.GetElementSize()),
    stride(acc.GetStride()) {
}

//! Accesses the i-th value as defined by the accessor
template <class T>
T Accessor::Indexer::GetValue(int i) {
    ai_assert(data);
    if (i * stride >= accessor.GetMaxByteSize()) {
        throw DeadlyImportError("GLTF: Invalid index ", i, ", count out of range for buffer with stride ", stride, " and size ", accessor.GetMaxByteSize(), ".");
    }
    // Ensure that the memcpy doesn't overwrite the local.
    const size_t sizeToCopy = std::min(elemSize, sizeof(T));
    T value = T();
    // Assume platform endianness matches GLTF binary data (which is little-endian).
    memcpy(&value, data + i * stride, sizeToCopy);
    return value;
}

inline Image::Image() :
        width(0),
        height(0),
        mDataLength(0) {
}

inline void Image::Read(Value &obj, Asset &r) {
    //basisu: no need to handle .ktx2, .basis, load as is
    if (!mDataLength) {
        Value *curUri = FindString(obj, "uri");
        if (nullptr != curUri) {
            const char *uristr = curUri->GetString();

            glTFCommon::Util::DataURI dataURI;
            if (ParseDataURI(uristr, curUri->GetStringLength(), dataURI)) {
                mimeType = dataURI.mediaType;
                if (dataURI.base64) {
                    uint8_t *ptr = nullptr;
                    mDataLength = Base64::Decode(dataURI.data, dataURI.dataLength, ptr);
                    mData.reset(ptr);
                }
            } else {
                this->uri = uristr;
            }
        } else if (Value *bufferViewVal = FindUInt(obj, "bufferView")) {
            this->bufferView = r.bufferViews.Retrieve(bufferViewVal->GetUint());
            if (Value *mtype = FindString(obj, "mimeType")) {
                this->mimeType = mtype->GetString();
            }
            if (!this->bufferView || this->mimeType.empty()) {
                throw DeadlyImportError("GLTF2: ", getContextForErrorMessages(id, name), " does not have a URI, so it must have a valid bufferView and mimetype");
            }

            Ref<Buffer> buffer = this->bufferView->buffer;

            this->mDataLength = this->bufferView->byteLength;
            // maybe this memcpy could be avoided if aiTexture does not delete[] pcData at destruction.

            this->mData.reset(new uint8_t[this->mDataLength]);
            memcpy(this->mData.get(), buffer->GetPointer() + this->bufferView->byteOffset, this->mDataLength);
        } else {
            throw DeadlyImportError("GLTF2: ", getContextForErrorMessages(id, name), " should have either a URI of a bufferView and mimetype");
        }
    }
}

inline uint8_t *Image::StealData() {
    mDataLength = 0;
    return mData.release();
}

// Never take over the ownership of data whenever binary or not
inline void Image::SetData(uint8_t *data, size_t length, Asset &r) {
    Ref<Buffer> b = r.GetBodyBuffer();
    if (b) { // binary file: append to body
        std::string bvId = r.FindUniqueID(this->id, "imgdata");
        bufferView = r.bufferViews.Create(bvId);

        bufferView->buffer = b;
        bufferView->byteLength = length;
        bufferView->byteOffset = b->AppendData(data, length);
    } else { // text file: will be stored as a data uri
        uint8_t *temp = new uint8_t[length];
        memcpy(temp, data, length);
        this->mData.reset(temp);
        this->mDataLength = length;
    }
}

inline void Sampler::Read(Value &obj, Asset & /*r*/) {
    SetDefaults();

    ReadMember(obj, "name", name);
    ReadMember(obj, "magFilter", magFilter);
    ReadMember(obj, "minFilter", minFilter);
    ReadMember(obj, "wrapS", wrapS);
    ReadMember(obj, "wrapT", wrapT);
}

inline void Sampler::SetDefaults() {
    //only wrapping modes have defaults
    wrapS = SamplerWrap::Repeat;
    wrapT = SamplerWrap::Repeat;
    magFilter = SamplerMagFilter::UNSET;
    minFilter = SamplerMinFilter::UNSET;
}

inline void Texture::Read(Value &obj, Asset &r) {
    if (Value *sourceVal = FindUInt(obj, "source")) {
        source = r.images.Retrieve(sourceVal->GetUint());
    }

    if (Value *samplerVal = FindUInt(obj, "sampler")) {
        sampler = r.samplers.Retrieve(samplerVal->GetUint());
    }
}

void Material::SetTextureProperties(Asset &r, Value *prop, TextureInfo &out) {
    if (r.extensionsUsed.KHR_texture_transform) {
        if (Value *pKHR_texture_transform = FindExtension(*prop, "KHR_texture_transform")) {
            out.textureTransformSupported = true;
            if (Value *array = FindArray(*pKHR_texture_transform, "offset")) {
                out.TextureTransformExt_t.offset[0] = (*array)[0].GetFloat();
                out.TextureTransformExt_t.offset[1] = (*array)[1].GetFloat();
            } else {
                out.TextureTransformExt_t.offset[0] = 0;
                out.TextureTransformExt_t.offset[1] = 0;
            }

            if (!ReadMember(*pKHR_texture_transform, "rotation", out.TextureTransformExt_t.rotation)) {
                out.TextureTransformExt_t.rotation = 0;
            }

            if (Value *array = FindArray(*pKHR_texture_transform, "scale")) {
                out.TextureTransformExt_t.scale[0] = (*array)[0].GetFloat();
                out.TextureTransformExt_t.scale[1] = (*array)[1].GetFloat();
            } else {
                out.TextureTransformExt_t.scale[0] = 1;
                out.TextureTransformExt_t.scale[1] = 1;
            }
        }
    }

    if (Value *indexProp = FindUInt(*prop, "index")) {
        out.texture = r.textures.Retrieve(indexProp->GetUint());
    }

    if (Value *texcoord = FindUInt(*prop, "texCoord")) {
        out.texCoord = texcoord->GetUint();
    }
}

inline void Material::ReadTextureProperty(Asset &r, Value &vals, const char *propName, TextureInfo &out) {
    if (Value *prop = FindMember(vals, propName)) {
        SetTextureProperties(r, prop, out);
    }
}

inline void Material::ReadTextureProperty(Asset &r, Value &vals, const char *propName, NormalTextureInfo &out) {
    if (Value *prop = FindMember(vals, propName)) {
        SetTextureProperties(r, prop, out);

        if (Value *scale = FindNumber(*prop, "scale")) {
            out.scale = static_cast<float>(scale->GetDouble());
        }
    }
}

inline void Material::ReadTextureProperty(Asset &r, Value &vals, const char *propName, OcclusionTextureInfo &out) {
    if (Value *prop = FindMember(vals, propName)) {
        SetTextureProperties(r, prop, out);

        if (Value *strength = FindNumber(*prop, "strength")) {
            out.strength = static_cast<float>(strength->GetDouble());
        }
    }
}

inline void Material::Read(Value &material, Asset &r) {
    SetDefaults();

    if (Value *curPbrMetallicRoughness = FindObject(material, "pbrMetallicRoughness")) {
        ReadMember(*curPbrMetallicRoughness, "baseColorFactor", this->pbrMetallicRoughness.baseColorFactor);
        ReadTextureProperty(r, *curPbrMetallicRoughness, "baseColorTexture", this->pbrMetallicRoughness.baseColorTexture);
        ReadTextureProperty(r, *curPbrMetallicRoughness, "metallicRoughnessTexture", this->pbrMetallicRoughness.metallicRoughnessTexture);
        ReadMember(*curPbrMetallicRoughness, "metallicFactor", this->pbrMetallicRoughness.metallicFactor);
        ReadMember(*curPbrMetallicRoughness, "roughnessFactor", this->pbrMetallicRoughness.roughnessFactor);
    }

    ReadTextureProperty(r, material, "normalTexture", this->normalTexture);
    ReadTextureProperty(r, material, "occlusionTexture", this->occlusionTexture);
    ReadTextureProperty(r, material, "emissiveTexture", this->emissiveTexture);
    ReadMember(material, "emissiveFactor", this->emissiveFactor);

    ReadMember(material, "doubleSided", this->doubleSided);
    ReadMember(material, "alphaMode", this->alphaMode);
    ReadMember(material, "alphaCutoff", this->alphaCutoff);

    if (Value *extensions = FindObject(material, "extensions")) {
        if (r.extensionsUsed.KHR_materials_pbrSpecularGlossiness) {
            if (Value *curPbrSpecularGlossiness = FindObject(*extensions, "KHR_materials_pbrSpecularGlossiness")) {
                PbrSpecularGlossiness pbrSG;

                ReadMember(*curPbrSpecularGlossiness, "diffuseFactor", pbrSG.diffuseFactor);
                ReadTextureProperty(r, *curPbrSpecularGlossiness, "diffuseTexture", pbrSG.diffuseTexture);
                ReadTextureProperty(r, *curPbrSpecularGlossiness, "specularGlossinessTexture", pbrSG.specularGlossinessTexture);
                ReadMember(*curPbrSpecularGlossiness, "specularFactor", pbrSG.specularFactor);
                ReadMember(*curPbrSpecularGlossiness, "glossinessFactor", pbrSG.glossinessFactor);

                this->pbrSpecularGlossiness = Nullable<PbrSpecularGlossiness>(pbrSG);
            }
        }

        // Extension KHR_texture_transform is handled in ReadTextureProperty

        if (r.extensionsUsed.KHR_materials_sheen) {
            if (Value *curMaterialSheen = FindObject(*extensions, "KHR_materials_sheen")) {
                MaterialSheen sheen;

                ReadMember(*curMaterialSheen, "sheenColorFactor", sheen.sheenColorFactor);
                ReadTextureProperty(r, *curMaterialSheen, "sheenColorTexture", sheen.sheenColorTexture);
                ReadMember(*curMaterialSheen, "sheenRoughnessFactor", sheen.sheenRoughnessFactor);
                ReadTextureProperty(r, *curMaterialSheen, "sheenRoughnessTexture", sheen.sheenRoughnessTexture);

                this->materialSheen = Nullable<MaterialSheen>(sheen);
            }
        }

        if (r.extensionsUsed.KHR_materials_clearcoat) {
            if (Value *curMaterialClearcoat = FindObject(*extensions, "KHR_materials_clearcoat")) {
                MaterialClearcoat clearcoat;

                ReadMember(*curMaterialClearcoat, "clearcoatFactor", clearcoat.clearcoatFactor);
                ReadTextureProperty(r, *curMaterialClearcoat, "clearcoatTexture", clearcoat.clearcoatTexture);
                ReadMember(*curMaterialClearcoat, "clearcoatRoughnessFactor", clearcoat.clearcoatRoughnessFactor);
                ReadTextureProperty(r, *curMaterialClearcoat, "clearcoatRoughnessTexture", clearcoat.clearcoatRoughnessTexture);
                ReadTextureProperty(r, *curMaterialClearcoat, "clearcoatNormalTexture", clearcoat.clearcoatNormalTexture);

                this->materialClearcoat = Nullable<MaterialClearcoat>(clearcoat);
            }
        }

        if (r.extensionsUsed.KHR_materials_transmission) {
            if (Value *curMaterialTransmission = FindObject(*extensions, "KHR_materials_transmission")) {
                MaterialTransmission transmission;

                ReadMember(*curMaterialTransmission, "transmissionFactor", transmission.transmissionFactor);
                ReadTextureProperty(r, *curMaterialTransmission, "transmissionTexture", transmission.transmissionTexture);

                this->materialTransmission = Nullable<MaterialTransmission>(transmission);
            }
        }

        if (r.extensionsUsed.KHR_materials_volume) {
            if (Value *curMaterialVolume = FindObject(*extensions, "KHR_materials_volume")) {
                MaterialVolume volume;

                ReadMember(*curMaterialVolume, "thicknessFactor", volume.thicknessFactor);
                ReadTextureProperty(r, *curMaterialVolume, "thicknessTexture", volume.thicknessTexture);
                ReadMember(*curMaterialVolume, "attenuationDistance", volume.attenuationDistance);
                ReadMember(*curMaterialVolume, "attenuationColor", volume.attenuationColor);

                this->materialVolume = Nullable<MaterialVolume>(volume);
            }
        }

        if (r.extensionsUsed.KHR_materials_ior) {
            if (Value *curMaterialIOR = FindObject(*extensions, "KHR_materials_ior")) {
                MaterialIOR ior;

                ReadMember(*curMaterialIOR, "ior", ior.ior);

                this->materialIOR = Nullable<MaterialIOR>(ior);
            }
        }

        if (r.extensionsUsed.KHR_materials_emissive_strength) {
            if (Value *curMaterialEmissiveStrength = FindObject(*extensions, "KHR_materials_emissive_strength")) {
                MaterialEmissiveStrength emissiveStrength;

                ReadMember(*curMaterialEmissiveStrength, "emissiveStrength", emissiveStrength.emissiveStrength);

                this->materialEmissiveStrength = Nullable<MaterialEmissiveStrength>(emissiveStrength);
            }
        }

        unlit = nullptr != FindObject(*extensions, "KHR_materials_unlit");
    }
}

inline void Material::SetDefaults() {
    //pbr materials
    SetVector(pbrMetallicRoughness.baseColorFactor, defaultBaseColor);
    pbrMetallicRoughness.metallicFactor = 1.0f;
    pbrMetallicRoughness.roughnessFactor = 1.0f;

    SetVector(emissiveFactor, defaultEmissiveFactor);
    alphaMode = "OPAQUE";
    alphaCutoff = 0.5f;
    doubleSided = false;
    unlit = false;
}

inline void PbrSpecularGlossiness::SetDefaults() {
    //pbrSpecularGlossiness properties
    SetVector(diffuseFactor, defaultDiffuseFactor);
    SetVector(specularFactor, defaultSpecularFactor);
    glossinessFactor = 1.0f;
}

inline void MaterialSheen::SetDefaults() {
    //KHR_materials_sheen properties
    SetVector(sheenColorFactor, defaultSheenFactor);
    sheenRoughnessFactor = 0.f;
}

inline void MaterialVolume::SetDefaults() {
    //KHR_materials_volume properties
    thicknessFactor = 0.f;
    attenuationDistance = INFINITY;
    SetVector(attenuationColor, defaultAttenuationColor);
}

inline void MaterialIOR::SetDefaults() {
    //KHR_materials_ior properties
    ior = 1.5f;
}

inline void MaterialEmissiveStrength::SetDefaults() {
    //KHR_materials_emissive_strength properties
    emissiveStrength = 0.f;
}

inline void Mesh::Read(Value &pJSON_Object, Asset &pAsset_Root) {
    Value *curName = FindMember(pJSON_Object, "name");
    if (nullptr != curName && curName->IsString()) {
        name = curName->GetString();
    }

    /****************** Mesh primitives ******************/
    Value *curPrimitives = FindArray(pJSON_Object, "primitives");
    if (nullptr != curPrimitives) {
        this->primitives.resize(curPrimitives->Size());
        for (unsigned int i = 0; i < curPrimitives->Size(); ++i) {
            Value &primitive = (*curPrimitives)[i];

            Primitive &prim = this->primitives[i];
            prim.mode = MemberOrDefault(primitive, "mode", PrimitiveMode_TRIANGLES);

            if (Value *indices = FindUInt(primitive, "indices")) {
                prim.indices = pAsset_Root.accessors.Retrieve(indices->GetUint());
            }

            if (Value *material = FindUInt(primitive, "material")) {
                prim.material = pAsset_Root.materials.Retrieve(material->GetUint());
            }

            if (Value *attrs = FindObject(primitive, "attributes")) {
                for (Value::MemberIterator it = attrs->MemberBegin(); it != attrs->MemberEnd(); ++it) {
                    if (!it->value.IsUint()) continue;
                    const char *attr = it->name.GetString();
                    // Valid attribute semantics include POSITION, NORMAL, TANGENT, TEXCOORD, COLOR, JOINT, JOINTMATRIX,
                    // and WEIGHT.Attribute semantics can be of the form[semantic]_[set_index], e.g., TEXCOORD_0, TEXCOORD_1, etc.

                    int undPos = 0;
                    Mesh::AccessorList *vec = nullptr;
                    if (GetAttribVector(prim, attr, vec, undPos)) {
                        size_t idx = (attr[undPos] == '_') ? atoi(attr + undPos + 1) : 0;
                        if ((*vec).size() != idx) {
                            throw DeadlyImportError("GLTF: Invalid attribute in mesh: ", name, " primitive: ", i, "attrib: ", attr,
                                    ". All indices for indexed attribute semantics must start with 0 and be continuous positive integers: TEXCOORD_0, TEXCOORD_1, etc.");
                        }
                        (*vec).resize(idx + 1);
                        (*vec)[idx] = pAsset_Root.accessors.Retrieve(it->value.GetUint());
                    }
                }
            }

#ifdef ASSIMP_ENABLE_DRACO
            // KHR_draco_mesh_compression spec: Draco can only be used for glTF Triangles or Triangle Strips
            if (pAsset_Root.extensionsUsed.KHR_draco_mesh_compression && (prim.mode == PrimitiveMode_TRIANGLES || prim.mode == PrimitiveMode_TRIANGLE_STRIP)) {
                // Look for draco mesh compression extension and bufferView
                // Skip if any missing
                if (Value *dracoExt = FindExtension(primitive, "KHR_draco_mesh_compression")) {
                    if (Value *bufView = FindUInt(*dracoExt, "bufferView")) {
                        // Attempt to load indices and attributes using draco compression
                        auto bufferView = pAsset_Root.bufferViews.Retrieve(bufView->GetUint());
                        // Attempt to perform the draco decode on the buffer data
                        const char *bufferViewData = reinterpret_cast<const char *>(bufferView->buffer->GetPointer() + bufferView->byteOffset);
                        draco::DecoderBuffer decoderBuffer;
                        decoderBuffer.Init(bufferViewData, bufferView->byteLength);
                        draco::Decoder decoder;
                        auto decodeResult = decoder.DecodeMeshFromBuffer(&decoderBuffer);
                        if (!decodeResult.ok()) {
                            // A corrupt Draco isn't actually fatal if the primitive data is also provided in a standard buffer, but does anyone do that?
                            throw DeadlyImportError("GLTF: Invalid Draco mesh compression in mesh: ", name, " primitive: ", i, ": ", decodeResult.status().error_msg_string());
                        }

                        // Now we have a draco mesh
                        const std::unique_ptr<draco::Mesh> &pDracoMesh = decodeResult.value();

                        // Redirect the accessors to the decoded data

                        // Indices
                        SetDecodedIndexBuffer_Draco(*pDracoMesh, prim);

                        // Vertex attributes
                        if (Value *attrs = FindObject(*dracoExt, "attributes")) {
                            for (Value::MemberIterator it = attrs->MemberBegin(); it != attrs->MemberEnd(); ++it) {
                                if (!it->value.IsUint()) continue;
                                const char *attr = it->name.GetString();

                                int undPos = 0;
                                Mesh::AccessorList *vec = nullptr;
                                if (GetAttribVector(prim, attr, vec, undPos)) {
                                    size_t idx = (attr[undPos] == '_') ? atoi(attr + undPos + 1) : 0;
                                    if (idx >= (*vec).size()) {
                                        throw DeadlyImportError("GLTF: Invalid draco attribute in mesh: ", name, " primitive: ", i, " attrib: ", attr,
                                                ". All indices for indexed attribute semantics must start with 0 and be continuous positive integers: TEXCOORD_0, TEXCOORD_1, etc.");
                                    }

                                    if (!(*vec)[idx]) {
                                        throw DeadlyImportError("GLTF: Invalid draco attribute in mesh: ", name, " primitive: ", i, " attrib: ", attr,
                                                ". All draco-encoded attributes must also define an accessor.");
                                    }

                                    Accessor &attribAccessor = *(*vec)[idx];
                                    if (attribAccessor.count == 0)
                                        throw DeadlyImportError("GLTF: Invalid draco attribute in mesh: ", name, " primitive: ", i, " attrib: ", attr);

                                    // Redirect this accessor to the appropriate Draco vertex attribute data
                                    const uint32_t dracoAttribId = it->value.GetUint();
                                    SetDecodedAttributeBuffer_Draco(*pDracoMesh, dracoAttribId, attribAccessor);
                                }
                            }
                        }
                    }
                }
            }
#endif

            Value *targetsArray = FindArray(primitive, "targets");
            if (nullptr != targetsArray) {
                prim.targets.resize(targetsArray->Size());
                for (unsigned int j = 0; j < targetsArray->Size(); ++j) {
                    Value &target = (*targetsArray)[j];
                    if (!target.IsObject()) {
                        continue;
                    }
                    for (Value::MemberIterator it = target.MemberBegin(); it != target.MemberEnd(); ++it) {
                        if (!it->value.IsUint()) {
                            continue;
                        }
                        const char *attr = it->name.GetString();
                        // Valid attribute semantics include POSITION, NORMAL, TANGENT
                        int undPos = 0;
                        Mesh::AccessorList *vec = nullptr;
                        if (GetAttribTargetVector(prim, j, attr, vec, undPos)) {
                            size_t idx = (attr[undPos] == '_') ? atoi(attr + undPos + 1) : 0;
                            if ((*vec).size() <= idx) {
                                (*vec).resize(idx + 1);
                            }
                            (*vec)[idx] = pAsset_Root.accessors.Retrieve(it->value.GetUint());
                        }
                    }
                }
            }
        }
    }

    Value *curWeights = FindArray(pJSON_Object, "weights");
    if (nullptr != curWeights) {
        this->weights.resize(curWeights->Size());
        for (unsigned int i = 0; i < curWeights->Size(); ++i) {
            Value &weightValue = (*curWeights)[i];
            if (weightValue.IsNumber()) {
                this->weights[i] = weightValue.GetFloat();
            }
        }
    }

    Value *curExtras = FindObject(pJSON_Object, "extras");
    if (nullptr != curExtras) {
        if (Value *curTargetNames = FindArray(*curExtras, "targetNames")) {
            this->targetNames.resize(curTargetNames->Size());
            for (unsigned int i = 0; i < curTargetNames->Size(); ++i) {
                Value &targetNameValue = (*curTargetNames)[i];
                if (targetNameValue.IsString()) {
                    this->targetNames[i] = targetNameValue.GetString();
                }
            }
        }
    }
}

inline void Camera::Read(Value &obj, Asset & /*r*/) {
    std::string type_string = std::string(MemberOrDefault(obj, "type", "perspective"));
    if (type_string == "orthographic") {
        type = Camera::Orthographic;
    } else {
        type = Camera::Perspective;
    }

    const char *subobjId = (type == Camera::Orthographic) ? "orthographic" : "perspective";

    Value *it = FindObject(obj, subobjId);
    if (!it) throw DeadlyImportError("GLTF: Camera missing its parameters");

    if (type == Camera::Perspective) {
        cameraProperties.perspective.aspectRatio = MemberOrDefault(*it, "aspectRatio", 0.f);
        cameraProperties.perspective.yfov = MemberOrDefault(*it, "yfov", 3.1415f / 2.f);
        cameraProperties.perspective.zfar = MemberOrDefault(*it, "zfar", 100.f);
        cameraProperties.perspective.znear = MemberOrDefault(*it, "znear", 0.01f);
    } else {
        cameraProperties.ortographic.xmag = MemberOrDefault(*it, "xmag", 1.f);
        cameraProperties.ortographic.ymag = MemberOrDefault(*it, "ymag", 1.f);
        cameraProperties.ortographic.zfar = MemberOrDefault(*it, "zfar", 100.f);
        cameraProperties.ortographic.znear = MemberOrDefault(*it, "znear", 0.01f);
    }
}

inline void Light::Read(Value &obj, Asset & /*r*/) {
#ifndef M_PI
    const float M_PI = 3.14159265358979323846f;
#endif

    std::string type_string;
    ReadMember(obj, "type", type_string);
    if (type_string == "directional")
        type = Light::Directional;
    else if (type_string == "point")
        type = Light::Point;
    else
        type = Light::Spot;

    name = MemberOrDefault(obj, "name", "");

    SetVector(color, vec3{ 1.0f, 1.0f, 1.0f });
    ReadMember(obj, "color", color);

    intensity = MemberOrDefault(obj, "intensity", 1.0f);

    ReadMember(obj, "range", range);

    if (type == Light::Spot) {
        Value *spot = FindObject(obj, "spot");
        if (!spot) throw DeadlyImportError("GLTF: Light missing its spot parameters");
        innerConeAngle = MemberOrDefault(*spot, "innerConeAngle", 0.0f);
        outerConeAngle = MemberOrDefault(*spot, "outerConeAngle", static_cast<float>(M_PI / 4.0f));
    }
}

inline void Node::Read(Value &obj, Asset &r) {
    if (name.empty()) {
        name = id;
    }

    Value *curChildren = FindArray(obj, "children");
    if (nullptr != curChildren) {
        this->children.reserve(curChildren->Size());
        for (unsigned int i = 0; i < curChildren->Size(); ++i) {
            Value &child = (*curChildren)[i];
            if (child.IsUint()) {
                // get/create the child node
                Ref<Node> chn = r.nodes.Retrieve(child.GetUint());
                if (chn) {
                    this->children.push_back(chn);
                }
            }
        }
    }

    Value *curMatrix = FindArray(obj, "matrix");
    if (nullptr != curMatrix) {
        ReadValue(*curMatrix, this->matrix);
    } else {
        ReadMember(obj, "translation", translation);
        ReadMember(obj, "scale", scale);
        ReadMember(obj, "rotation", rotation);
    }

    Value *curMesh = FindUInt(obj, "mesh");
    if (nullptr != curMesh) {
        unsigned int numMeshes = 1;
        this->meshes.reserve(numMeshes);
        Ref<Mesh> meshRef = r.meshes.Retrieve((*curMesh).GetUint());
        if (meshRef) {
            this->meshes.push_back(meshRef);
        }
    }

    // Do not retrieve a skin here, just take a reference, to avoid infinite recursion
    // Skins will be properly loaded later
    Value *curSkin = FindUInt(obj, "skin");
    if (nullptr != curSkin) {
        this->skin = r.skins.Get(curSkin->GetUint());
    }

    Value *curCamera = FindUInt(obj, "camera");
    if (nullptr != curCamera) {
        this->camera = r.cameras.Retrieve(curCamera->GetUint());
        if (this->camera) {
            this->camera->id = this->id;
        }
    }

    Value *curExtensions = FindObject(obj, "extensions");
    if (nullptr != curExtensions) {
        if (r.extensionsUsed.KHR_lights_punctual) {
            if (Value *ext = FindObject(*curExtensions, "KHR_lights_punctual")) {
                Value *curLight = FindUInt(*ext, "light");
                if (nullptr != curLight) {
                    this->light = r.lights.Retrieve(curLight->GetUint());
                    if (this->light) {
                        this->light->id = this->id;
                    }
                }
            }
        }
    }
}

inline void Scene::Read(Value &obj, Asset &r) {
    if (Value *scene_name = FindString(obj, "name")) {
        if (scene_name->IsString()) {
            this->name = scene_name->GetString();
        }
    }
    if (Value *array = FindArray(obj, "nodes")) {
        for (unsigned int i = 0; i < array->Size(); ++i) {
            if (!(*array)[i].IsUint()) continue;
            Ref<Node> node = r.nodes.Retrieve((*array)[i].GetUint());
            if (node)
                this->nodes.push_back(node);
        }
    }
}

inline void Skin::Read(Value &obj, Asset &r) {
    if (Value *matrices = FindUInt(obj, "inverseBindMatrices")) {
        inverseBindMatrices = r.accessors.Retrieve(matrices->GetUint());
    }

    if (Value *joints = FindArray(obj, "joints")) {
        for (unsigned i = 0; i < joints->Size(); ++i) {
            if (!(*joints)[i].IsUint()) continue;
            Ref<Node> node = r.nodes.Retrieve((*joints)[i].GetUint());
            if (node) {
                this->jointNames.push_back(node);
            }
        }
    }
}

inline void Animation::Read(Value &obj, Asset &r) {
    Value *curSamplers = FindArray(obj, "samplers");
    if (nullptr != curSamplers) {
        for (unsigned i = 0; i < curSamplers->Size(); ++i) {
            Value &sampler = (*curSamplers)[i];

            Sampler s;
            if (Value *input = FindUInt(sampler, "input")) {
                s.input = r.accessors.Retrieve(input->GetUint());
            }
            if (Value *output = FindUInt(sampler, "output")) {
                s.output = r.accessors.Retrieve(output->GetUint());
            }
            s.interpolation = Interpolation_LINEAR;
            if (Value *interpolation = FindString(sampler, "interpolation")) {
                const std::string interp = interpolation->GetString();
                if (interp == "LINEAR") {
                    s.interpolation = Interpolation_LINEAR;
                } else if (interp == "STEP") {
                    s.interpolation = Interpolation_STEP;
                } else if (interp == "CUBICSPLINE") {
                    s.interpolation = Interpolation_CUBICSPLINE;
                }
            }
            this->samplers.push_back(s);
        }
    }

    Value *curChannels = FindArray(obj, "channels");
    if (nullptr != curChannels) {
        for (unsigned i = 0; i < curChannels->Size(); ++i) {
            Value &channel = (*curChannels)[i];

            Channel c;
            Value *curSampler = FindUInt(channel, "sampler");
            if (nullptr != curSampler) {
                c.sampler = curSampler->GetUint();
            }

            if (Value *target = FindObject(channel, "target")) {
                if (Value *node = FindUInt(*target, "node")) {
                    c.target.node = r.nodes.Retrieve(node->GetUint());
                }
                if (Value *path = FindString(*target, "path")) {
                    const std::string p = path->GetString();
                    if (p == "translation") {
                        c.target.path = AnimationPath_TRANSLATION;
                    } else if (p == "rotation") {
                        c.target.path = AnimationPath_ROTATION;
                    } else if (p == "scale") {
                        c.target.path = AnimationPath_SCALE;
                    } else if (p == "weights") {
                        c.target.path = AnimationPath_WEIGHTS;
                    }
                }
            }
            this->channels.push_back(c);
        }
    }
}

inline void AssetMetadata::Read(Document &doc) {
    if (Value *obj = FindObject(doc, "asset")) {
        ReadMember(*obj, "copyright", copyright);
        ReadMember(*obj, "generator", generator);

        if (Value *versionString = FindStringInContext(*obj, "version", "\"asset\"")) {
            version = versionString->GetString();
        }
        Value *curProfile = FindObjectInContext(*obj, "profile", "\"asset\"");
        if (nullptr != curProfile) {
            ReadMember(*curProfile, "api", this->profile.api);
            ReadMember(*curProfile, "version", this->profile.version);
        }
    }

    if (version.empty() || version[0] != '2') {
        throw DeadlyImportError("GLTF: Unsupported glTF version: ", version);
    }
}

//
// Asset methods implementation
//

inline void Asset::ReadBinaryHeader(IOStream &stream, std::vector<char> &sceneData) {
    ASSIMP_LOG_DEBUG("Reading GLTF2 binary");
    GLB_Header header;
    if (stream.Read(&header, sizeof(header), 1) != 1) {
        throw DeadlyImportError("GLTF: Unable to read the file header");
    }

    if (strncmp((char *)header.magic, AI_GLB_MAGIC_NUMBER, sizeof(header.magic)) != 0) {
        throw DeadlyImportError("GLTF: Invalid binary glTF file");
    }

    AI_SWAP4(header.version);
    asset.version = ai_to_string(header.version);
    if (header.version != 2) {
        throw DeadlyImportError("GLTF: Unsupported binary glTF version");
    }

    GLB_Chunk chunk;
    if (stream.Read(&chunk, sizeof(chunk), 1) != 1) {
        throw DeadlyImportError("GLTF: Unable to read JSON chunk");
    }

    AI_SWAP4(chunk.chunkLength);
    AI_SWAP4(chunk.chunkType);

    if (chunk.chunkType != ChunkType_JSON) {
        throw DeadlyImportError("GLTF: JSON chunk missing");
    }

    // read the scene data, ensure null termination
    static_assert(std::numeric_limits<uint32_t>::max() <= std::numeric_limits<size_t>::max(), "size_t must be at least 32bits");
    mSceneLength = chunk.chunkLength; // Can't be larger than 4GB (max. uint32_t)
    sceneData.resize(mSceneLength + 1);
    sceneData[mSceneLength] = '\0';

    if (stream.Read(&sceneData[0], 1, mSceneLength) != mSceneLength) {
        throw DeadlyImportError("GLTF: Could not read the file contents");
    }

    uint32_t padding = ((chunk.chunkLength + 3) & ~3) - chunk.chunkLength;
    if (padding > 0) {
        stream.Seek(padding, aiOrigin_CUR);
    }

    AI_SWAP4(header.length);
    mBodyOffset = 12 + 8 + chunk.chunkLength + padding + 8;
    if (header.length >= mBodyOffset) {
        if (stream.Read(&chunk, sizeof(chunk), 1) != 1) {
            throw DeadlyImportError("GLTF: Unable to read BIN chunk");
        }

        AI_SWAP4(chunk.chunkLength);
        AI_SWAP4(chunk.chunkType);

        if (chunk.chunkType != ChunkType_BIN) {
            throw DeadlyImportError("GLTF: BIN chunk missing");
        }

        mBodyLength = chunk.chunkLength;
    } else {
        mBodyOffset = mBodyLength = 0;
    }
}

inline rapidjson::Document Asset::ReadDocument(IOStream &stream, bool isBinary, std::vector<char> &sceneData) {
    ASSIMP_LOG_DEBUG("Loading GLTF2 asset");

    // is binary? then read the header
    if (isBinary) {
        SetAsBinary(); // also creates the body buffer
        ReadBinaryHeader(stream, sceneData);
    } else {
        mSceneLength = stream.FileSize();
        mBodyLength = 0;

        // Binary format only supports up to 4GB of JSON, use that as a maximum
        if (mSceneLength >= std::numeric_limits<uint32_t>::max()) {
            throw DeadlyImportError("GLTF: JSON size greater than 4GB");
        }

        // read the scene data, ensure null termination
        sceneData.resize(mSceneLength + 1);
        sceneData[mSceneLength] = '\0';

        if (stream.Read(&sceneData[0], 1, mSceneLength) != mSceneLength) {
            throw DeadlyImportError("GLTF: Could not read the file contents");
        }
    }

    // Smallest legal JSON file is "{}" Smallest loadable glTF file is larger than that but catch it later
    if (mSceneLength < 2) {
        throw DeadlyImportError("GLTF: No JSON file contents");
    }

    // parse the JSON document
    ASSIMP_LOG_DEBUG("Parsing GLTF2 JSON");
    Document doc;
    doc.ParseInsitu(&sceneData[0]);

    if (doc.HasParseError()) {
        char buffer[32];
        ai_snprintf(buffer, 32, "%d", static_cast<int>(doc.GetErrorOffset()));
        throw DeadlyImportError("GLTF: JSON parse error, offset ", buffer, ": ", GetParseError_En(doc.GetParseError()));
    }

    if (!doc.IsObject()) {
        throw DeadlyImportError("GLTF: JSON document root must be a JSON object");
    }

    return doc;
}

inline void Asset::Load(const std::string &pFile, bool isBinary)
{
    mCurrentAssetDir.clear();
    if (0 != strncmp(pFile.c_str(), AI_MEMORYIO_MAGIC_FILENAME, AI_MEMORYIO_MAGIC_FILENAME_LENGTH)) {
        mCurrentAssetDir = glTFCommon::getCurrentAssetDir(pFile);
    }

    shared_ptr<IOStream> stream(OpenFile(pFile.c_str(), "rb", true));
    if (!stream) {
        throw DeadlyImportError("GLTF: Could not open file for reading");
    }

    std::vector<char> sceneData;
    rapidjson::Document doc = ReadDocument(*stream, isBinary, sceneData);

    // If a schemaDocumentProvider is available, see if the glTF schema is present.
    // If so, use it to validate the document.
    if (mSchemaDocumentProvider) {
        if (const rapidjson::SchemaDocument *gltfSchema = mSchemaDocumentProvider->GetRemoteDocument("glTF.schema.json", 16)) {
            // The schemas are found here: https://github.com/KhronosGroup/glTF/tree/main/specification/2.0/schema
            rapidjson::SchemaValidator validator(*gltfSchema);
            if (!doc.Accept(validator)) {
                rapidjson::StringBuffer pathBuffer;
                validator.GetInvalidSchemaPointer().StringifyUriFragment(pathBuffer);
                rapidjson::StringBuffer argumentBuffer;
                validator.GetInvalidDocumentPointer().StringifyUriFragment(argumentBuffer);
                throw DeadlyImportError("GLTF: The JSON document did not satisfy the glTF2 schema. Schema keyword: ", validator.GetInvalidSchemaKeyword(), ", document path: ", pathBuffer.GetString(), ", argument: ", argumentBuffer.GetString());
            }
        }
    }

    // Fill the buffer instance for the current file embedded contents
    if (mBodyLength > 0) {
        if (!mBodyBuffer->LoadFromStream(*stream, mBodyLength, mBodyOffset)) {
            throw DeadlyImportError("GLTF: Unable to read gltf file");
        }
    }

    // Load the metadata
    asset.Read(doc);
    ReadExtensionsUsed(doc);
    ReadExtensionsRequired(doc);

#ifndef ASSIMP_ENABLE_DRACO
    // Is Draco required?
    if (extensionsRequired.KHR_draco_mesh_compression) {
        throw DeadlyImportError("GLTF: Draco mesh compression not supported.");
    }
#endif

    // Prepare the dictionaries
    for (size_t i = 0; i < mDicts.size(); ++i) {
        mDicts[i]->AttachToDocument(doc);
    }

    // Read the "scene" property, which specifies which scene to load
    // and recursively load everything referenced by it
    unsigned int sceneIndex = 0;
    Value *curScene = FindUInt(doc, "scene");
    if (nullptr != curScene) {
        sceneIndex = curScene->GetUint();
    }

    if (Value *scenesArray = FindArray(doc, "scenes")) {
        if (sceneIndex < scenesArray->Size()) {
            this->scene = scenes.Retrieve(sceneIndex);
        }
    }

    if (Value *skinsArray = FindArray(doc, "skins")) {
        for (unsigned int i = 0; i < skinsArray->Size(); ++i) {
            skins.Retrieve(i);
        }
    }

    if (Value *animsArray = FindArray(doc, "animations")) {
        for (unsigned int i = 0; i < animsArray->Size(); ++i) {
            animations.Retrieve(i);
        }
    }

    // Clean up
    for (size_t i = 0; i < mDicts.size(); ++i) {
        mDicts[i]->DetachFromDocument();
    }
}

inline bool Asset::CanRead(const std::string &pFile, bool isBinary) {
    try {
        shared_ptr<IOStream> stream(OpenFile(pFile.c_str(), "rb", true));
        if (!stream) {
            return false;
        }
        std::vector<char> sceneData;
        rapidjson::Document doc = ReadDocument(*stream, isBinary, sceneData);
        asset.Read(doc);
    } catch (...) {
        return false;
    }
    return true;
}

inline void Asset::SetAsBinary() {
    if (!mBodyBuffer) {
        mBodyBuffer = buffers.Create("binary_glTF");
        mBodyBuffer->MarkAsSpecial();
    }
}

// As required extensions are only a concept in glTF 2.0, this is here
// instead of glTFCommon.h
#define CHECK_REQUIRED_EXT(EXT) \
    if (exts.find(#EXT) != exts.end()) extensionsRequired.EXT = true;

inline void Asset::ReadExtensionsRequired(Document &doc) {
    Value *extsRequired = FindArray(doc, "extensionsRequired");
    if (nullptr == extsRequired) {
        return;
    }

    std::gltf_unordered_map<std::string, bool> exts;
    for (unsigned int i = 0; i < extsRequired->Size(); ++i) {
        if ((*extsRequired)[i].IsString()) {
            exts[(*extsRequired)[i].GetString()] = true;
        }
    }

    CHECK_REQUIRED_EXT(KHR_draco_mesh_compression);

#undef CHECK_REQUIRED_EXT
}

inline void Asset::ReadExtensionsUsed(Document &doc) {
    Value *extsUsed = FindArray(doc, "extensionsUsed");
    if (!extsUsed) return;

    std::gltf_unordered_map<std::string, bool> exts;

    for (unsigned int i = 0; i < extsUsed->Size(); ++i) {
        if ((*extsUsed)[i].IsString()) {
            exts[(*extsUsed)[i].GetString()] = true;
        }
    }

    CHECK_EXT(KHR_materials_pbrSpecularGlossiness);
    CHECK_EXT(KHR_materials_unlit);
    CHECK_EXT(KHR_lights_punctual);
    CHECK_EXT(KHR_texture_transform);
    CHECK_EXT(KHR_materials_sheen);
    CHECK_EXT(KHR_materials_clearcoat);
    CHECK_EXT(KHR_materials_transmission);
    CHECK_EXT(KHR_materials_volume);
    CHECK_EXT(KHR_materials_ior);
    CHECK_EXT(KHR_materials_emissive_strength);
    CHECK_EXT(KHR_draco_mesh_compression);
    CHECK_EXT(KHR_texture_basisu);

#undef CHECK_EXT
}

inline IOStream *Asset::OpenFile(const std::string &path, const char *mode, bool /*absolute*/) {
#ifdef ASSIMP_API
    return mIOSystem->Open(path, mode);
#else
    if (path.size() < 2) return nullptr;
    if (!absolute && path[1] != ':' && path[0] != '/') { // relative?
        path = mCurrentAssetDir + path;
    }
    FILE *f = fopen(path.c_str(), mode);
    return f ? new IOStream(f) : nullptr;
#endif
}

inline std::string Asset::FindUniqueID(const std::string &str, const char *suffix) {
    std::string id = str;

    if (!id.empty()) {
        if (mUsedIds.find(id) == mUsedIds.end())
            return id;

        id += "_";
    }

    id += suffix;

    Asset::IdMap::iterator it = mUsedIds.find(id);
    if (it == mUsedIds.end()) {
        return id;
    }

    std::vector<char> buffer;
    buffer.resize(id.size() + 16);
    int offset = ai_snprintf(buffer.data(), buffer.size(), "%s_", id.c_str());
    for (int i = 0; it != mUsedIds.end(); ++i) {
        ai_snprintf(buffer.data() + offset, buffer.size() - offset, "%d", i);
        id = buffer.data();
        it = mUsedIds.find(id);
    }

    return id;
}

#if _MSC_VER
#   pragma warning(pop)
#endif // _MSC_VER

} // namespace glTF2
