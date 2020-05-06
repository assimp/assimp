/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team


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

#include <assimp/StringUtils.h>
#include <assimp/DefaultLogger.hpp>

using namespace Assimp;

namespace glTF2 {

namespace {

//
// JSON Value reading helpers
//

template <class T>
struct ReadHelper {
    static bool Read(Value &val, T &out) {
        return val.IsInt() ? out = static_cast<T>(val.GetInt()), true : false;
    }
};

template <>
struct ReadHelper<bool> {
    static bool Read(Value &val, bool &out) {
        return val.IsBool() ? out = val.GetBool(), true : false;
    }
};

template <>
struct ReadHelper<float> {
    static bool Read(Value &val, float &out) {
        return val.IsNumber() ? out = static_cast<float>(val.GetDouble()), true : false;
    }
};

template <unsigned int N>
struct ReadHelper<float[N]> {
    static bool Read(Value &val, float (&out)[N]) {
        if (!val.IsArray() || val.Size() != N) return false;
        for (unsigned int i = 0; i < N; ++i) {
            if (val[i].IsNumber())
                out[i] = static_cast<float>(val[i].GetDouble());
        }
        return true;
    }
};

template <>
struct ReadHelper<const char *> {
    static bool Read(Value &val, const char *&out) {
        return val.IsString() ? (out = val.GetString(), true) : false;
    }
};

template <>
struct ReadHelper<std::string> {
    static bool Read(Value &val, std::string &out) {
        return val.IsString() ? (out = std::string(val.GetString(), val.GetStringLength()), true) : false;
    }
};

template <>
struct ReadHelper<uint64_t> {
    static bool Read(Value &val, uint64_t &out) {
        return val.IsUint64() ? out = val.GetUint64(), true : false;
    }
};

template <>
struct ReadHelper<int64_t> {
    static bool Read(Value &val, int64_t &out) {
        return val.IsInt64() ? out = val.GetInt64(), true : false;
    }
};

template <class T>
struct ReadHelper<Nullable<T>> {
    static bool Read(Value &val, Nullable<T> &out) {
        return out.isPresent = ReadHelper<T>::Read(val, out.value);
    }
};

template <class T>
inline static bool ReadValue(Value &val, T &out) {
    return ReadHelper<T>::Read(val, out);
}

template <class T>
inline static bool ReadMember(Value &obj, const char *id, T &out) {
    Value::MemberIterator it = obj.FindMember(id);
    if (it != obj.MemberEnd()) {
        return ReadHelper<T>::Read(it->value, out);
    }
    return false;
}

template <class T>
inline static T MemberOrDefault(Value &obj, const char *id, T defaultValue) {
    T out;
    return ReadMember(obj, id, out) ? out : defaultValue;
}

inline Value *FindMember(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd()) ? &it->value : 0;
}

inline Value *FindString(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd() && it->value.IsString()) ? &it->value : 0;
}

inline Value *FindNumber(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd() && it->value.IsNumber()) ? &it->value : 0;
}

inline Value *FindUInt(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd() && it->value.IsUint()) ? &it->value : 0;
}

inline Value *FindArray(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd() && it->value.IsArray()) ? &it->value : 0;
}

inline Value *FindObject(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd() && it->value.IsObject()) ? &it->value : 0;
}
} // namespace

//
// LazyDict methods
//

template <class T>
inline LazyDict<T>::LazyDict(Asset &asset, const char *dictId, const char *extId) :
        mDictId(dictId), mExtId(extId), mDict(0), mAsset(asset) {
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
    Value *container = 0;

    if (mExtId) {
        if (Value *exts = FindObject(doc, "extensions")) {
            container = FindObject(*exts, mExtId);
        }
    } else {
        container = &doc;
    }

    if (container) {
        mDict = FindArray(*container, mDictId);
    }
}

template <class T>
inline void LazyDict<T>::DetachFromDocument() {
    mDict = 0;
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
        throw DeadlyImportError("GLTF: Missing section \"" + std::string(mDictId) + "\"");
    }

    if (!mDict->IsArray()) {
        throw DeadlyImportError("GLTF: Field is not an array \"" + std::string(mDictId) + "\"");
    }

    Value &obj = (*mDict)[i];

    if (!obj.IsObject()) {
        throw DeadlyImportError("GLTF: Object at index \"" + to_string(i) + "\" is not a JSON object");
    }

    if (mRecursiveReferenceCheck.find(i) != mRecursiveReferenceCheck.end()) {
        throw DeadlyImportError("GLTF: Object at index \"" + to_string(i) + "\" has recursive reference to itself");
    }
    mRecursiveReferenceCheck.insert(i);

    // Unique ptr prevents memory leak in case of Read throws an exception
    auto inst = std::unique_ptr<T>(new T());
    inst->id = std::string(mDictId) + "_" + to_string(i);
    inst->oIndex = i;
    ReadMember(obj, "name", inst->name);
    inst->Read(obj, mAsset);

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
        byteLength(0), type(Type_arraybuffer), EncodedRegion_Current(nullptr), mIsSpecial(false) {}

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
            uint8_t *data = 0;
            this->byteLength = glTFCommon::Util::DecodeBase64(dataURI.data, dataURI.dataLength, data);
            this->mData.reset(data, std::default_delete<uint8_t[]>());

            if (statedLength > 0 && this->byteLength != statedLength) {
                throw DeadlyImportError("GLTF: buffer \"" + id + "\", expected " + to_string(statedLength) +
                                        " bytes, but found " + to_string(dataURI.dataLength));
            }
        } else { // assume raw data
            if (statedLength != dataURI.dataLength) {
                throw DeadlyImportError("GLTF: buffer \"" + id + "\", expected " + to_string(statedLength) +
                                        " bytes, but found " + to_string(dataURI.dataLength));
            }

            this->mData.reset(new uint8_t[dataURI.dataLength], std::default_delete<uint8_t[]>());
            memcpy(this->mData.get(), dataURI.data, dataURI.dataLength);
        }
    } else { // Local file
        if (byteLength > 0) {
            std::string dir = !r.mCurrentAssetDir.empty() ? (r.mCurrentAssetDir) : "";

            IOStream *file = r.OpenFile(dir + uri, "rb");
            if (file) {
                bool ok = LoadFromStream(*file, byteLength);
                delete file;

                if (!ok)
                    throw DeadlyImportError("GLTF: error while reading referenced file \"" + std::string(uri) + "\"");
            } else {
                throw DeadlyImportError("GLTF: could not open referenced file \"" + std::string(uri) + "\"");
            }
        }
    }
}

inline bool Buffer::LoadFromStream(IOStream &stream, size_t length, size_t baseOffset) {
    byteLength = length ? length : stream.FileSize();

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

        ai_snprintf(val, val_size, "%llu", (long long)pOffset);
        throw DeadlyImportError(std::string("GLTF: incorrect offset value (") + val + ") for marking encoded region.");
    }

    // Check length
    if ((pOffset + pEncodedData_Length) > byteLength) {
        const uint8_t val_size = 64;

        char val[val_size];

        ai_snprintf(val, val_size, "%llu, %llu", (long long)pOffset, (long long)pEncodedData_Length);
        throw DeadlyImportError(std::string("GLTF: encoded region with offset/length (") + val + ") is out of range.");
    }

    // Add new region
    EncodedRegion_List.push_back(new SEncodedRegion(pOffset, pEncodedData_Length, pDecodedData, pDecodedData_Length, pID));
    // And set new value for "byteLength"
    byteLength += (pDecodedData_Length - pEncodedData_Length);
}

inline void Buffer::EncodedRegion_SetCurrent(const std::string &pID) {
    if ((EncodedRegion_Current != nullptr) && (EncodedRegion_Current->ID == pID)) return;

    for (SEncodedRegion *reg : EncodedRegion_List) {
        if (reg->ID == pID) {
            EncodedRegion_Current = reg;

            return;
        }
    }

    throw DeadlyImportError("GLTF: EncodedRegion with ID: \"" + pID + "\" not found.");
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
    size_t offset = this->byteLength;
    // Force alignment to 4 bits
    Grow((length + 3) & ~3);
    memcpy(mData.get() + offset, data, length);
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

    byteOffset = MemberOrDefault(obj, "byteOffset", size_t(0));
    byteLength = MemberOrDefault(obj, "byteLength", size_t(0));
    byteStride = MemberOrDefault(obj, "byteStride", 0u);
}

//
// struct Accessor
//

inline void Accessor::Read(Value &obj, Asset &r) {

    if (Value *bufferViewVal = FindUInt(obj, "bufferView")) {
        bufferView = r.bufferViews.Retrieve(bufferViewVal->GetUint());
    }

    byteOffset = MemberOrDefault(obj, "byteOffset", size_t(0));
    componentType = MemberOrDefault(obj, "componentType", ComponentType_BYTE);
    count = MemberOrDefault(obj, "count", size_t(0));

    const char *typestr;
    type = ReadMember(obj, "type", typestr) ? AttribType::FromString(typestr) : AttribType::SCALAR;
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
    if (!bufferView || !bufferView->buffer) return 0;
    uint8_t *basePtr = bufferView->buffer->GetPointer();
    if (!basePtr) return 0;

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

namespace {
inline void CopyData(size_t count,
        const uint8_t *src, size_t src_stride,
        uint8_t *dst, size_t dst_stride) {
    if (src_stride == dst_stride) {
        memcpy(dst, src, count * src_stride);
    } else {
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
}
} // namespace

template<class T>
void Accessor::ExtractData(T *&outData)
{
    uint8_t* data = GetPointer();
    if (!data) {
        throw DeadlyImportError("GLTF: data is NULL");
    }

    const size_t elemSize = GetElementSize();
    const size_t totalSize = elemSize * count;

    const size_t stride = bufferView && bufferView->byteStride ? bufferView->byteStride : elemSize;

    const size_t targetElemSize = sizeof(T);
    ai_assert(elemSize <= targetElemSize);

    ai_assert(count * stride <= bufferView->byteLength);

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

inline Accessor::Indexer::Indexer(Accessor &acc) :
        accessor(acc), data(acc.GetPointer()), elemSize(acc.GetElementSize()), stride(acc.bufferView && acc.bufferView->byteStride ? acc.bufferView->byteStride : elemSize) {
}

//! Accesses the i-th value as defined by the accessor
template <class T>
T Accessor::Indexer::GetValue(int i) {
    ai_assert(data);
    ai_assert(i * stride < accessor.bufferView->byteLength);
    T value = T();
    memcpy(&value, data + i * stride, elemSize);
    //value >>= 8 * (sizeof(T) - elemSize);
    return value;
}

inline Image::Image() :
        width(0), height(0), mDataLength(0) {
}

inline void Image::Read(Value &obj, Asset &r) {
    if (!mDataLength) {
        Value *curUri = FindString(obj, "uri");
        if (nullptr != curUri) {
            const char *uristr = curUri->GetString();

            glTFCommon::Util::DataURI dataURI;
            if (ParseDataURI(uristr, curUri->GetStringLength(), dataURI)) {
                mimeType = dataURI.mediaType;
                if (dataURI.base64) {
                    uint8_t *ptr = nullptr;
                    mDataLength = glTFCommon::Util::DecodeBase64(dataURI.data, dataURI.dataLength, ptr);
                    mData.reset(ptr);
                }
            } else {
                this->uri = uristr;
            }
        } else if (Value *bufferViewVal = FindUInt(obj, "bufferView")) {
            this->bufferView = r.bufferViews.Retrieve(bufferViewVal->GetUint());
            Ref<Buffer> buffer = this->bufferView->buffer;

            this->mDataLength = this->bufferView->byteLength;
            // maybe this memcpy could be avoided if aiTexture does not delete[] pcData at destruction.

            this->mData.reset(new uint8_t[this->mDataLength]);
            memcpy(this->mData.get(), buffer->GetPointer() + this->bufferView->byteOffset, this->mDataLength);

            if (Value *mtype = FindString(obj, "mimeType")) {
                this->mimeType = mtype->GetString();
            }
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

namespace {
inline void SetTextureProperties(Asset &r, Value *prop, TextureInfo &out) {
    if (r.extensionsUsed.KHR_texture_transform) {
        if (Value *extensions = FindObject(*prop, "extensions")) {
            out.textureTransformSupported = true;
            if (Value *pKHR_texture_transform = FindObject(*extensions, "KHR_texture_transform")) {
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
    }

    if (Value *index = FindUInt(*prop, "index")) {
        out.texture = r.textures.Retrieve(index->GetUint());
    }

    if (Value *texcoord = FindUInt(*prop, "texCoord")) {
        out.texCoord = texcoord->GetUint();
    }
}

inline void ReadTextureProperty(Asset &r, Value &vals, const char *propName, TextureInfo &out) {
    if (Value *prop = FindMember(vals, propName)) {
        SetTextureProperties(r, prop, out);
    }
}

inline void ReadTextureProperty(Asset &r, Value &vals, const char *propName, NormalTextureInfo &out) {
    if (Value *prop = FindMember(vals, propName)) {
        SetTextureProperties(r, prop, out);

        if (Value *scale = FindNumber(*prop, "scale")) {
            out.scale = static_cast<float>(scale->GetDouble());
        }
    }
}

inline void ReadTextureProperty(Asset &r, Value &vals, const char *propName, OcclusionTextureInfo &out) {
    if (Value *prop = FindMember(vals, propName)) {
        SetTextureProperties(r, prop, out);

        if (Value *strength = FindNumber(*prop, "strength")) {
            out.strength = static_cast<float>(strength->GetDouble());
        }
    }
}
} // namespace

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

        if (r.extensionsUsed.KHR_texture_transform) {
        }

        unlit = nullptr != FindObject(*extensions, "KHR_materials_unlit");
    }
}

namespace {
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
} // namespace

inline void Material::SetDefaults() {
    //pbr materials
    SetVector(pbrMetallicRoughness.baseColorFactor, defaultBaseColor);
    pbrMetallicRoughness.metallicFactor = 1.0;
    pbrMetallicRoughness.roughnessFactor = 1.0;

    SetVector(emissiveFactor, defaultEmissiveFactor);
    alphaMode = "OPAQUE";
    alphaCutoff = 0.5;
    doubleSided = false;
    unlit = false;
}

inline void PbrSpecularGlossiness::SetDefaults() {
    //pbrSpecularGlossiness properties
    SetVector(diffuseFactor, defaultDiffuseFactor);
    SetVector(specularFactor, defaultSpecularFactor);
    glossinessFactor = 1.0;
}

namespace {

template <int N>
inline int Compare(const char *attr, const char (&str)[N]) {
    return (strncmp(attr, str, N - 1) == 0) ? N - 1 : 0;
}

#ifdef _WIN32
#    pragma warning(push)
#    pragma warning(disable : 4706)
#endif // _WIN32

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
    } else if ((pos = Compare(attr, "JOINT"))) {
        v = &(p.attributes.joint);
    } else if ((pos = Compare(attr, "JOINTMATRIX"))) {
        v = &(p.attributes.jointmatrix);
    } else if ((pos = Compare(attr, "WEIGHT"))) {
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

inline void Mesh::Read(Value &pJSON_Object, Asset &pAsset_Root) {
    Value *curName = FindMember(pJSON_Object, "name");
    if (nullptr != curName) {
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

            if (Value *attrs = FindObject(primitive, "attributes")) {
                for (Value::MemberIterator it = attrs->MemberBegin(); it != attrs->MemberEnd(); ++it) {
                    if (!it->value.IsUint()) continue;
                    const char *attr = it->name.GetString();
                    // Valid attribute semantics include POSITION, NORMAL, TANGENT, TEXCOORD, COLOR, JOINT, JOINTMATRIX,
                    // and WEIGHT.Attribute semantics can be of the form[semantic]_[set_index], e.g., TEXCOORD_0, TEXCOORD_1, etc.

                    int undPos = 0;
                    Mesh::AccessorList *vec = 0;
                    if (GetAttribVector(prim, attr, vec, undPos)) {
                        size_t idx = (attr[undPos] == '_') ? atoi(attr + undPos + 1) : 0;
                        if ((*vec).size() <= idx) (*vec).resize(idx + 1);
                        (*vec)[idx] = pAsset_Root.accessors.Retrieve(it->value.GetUint());
                    }
                }
            }

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
                        Mesh::AccessorList *vec = 0;
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

            if (Value *indices = FindUInt(primitive, "indices")) {
                prim.indices = pAsset_Root.accessors.Retrieve(indices->GetUint());
            }

            if (Value *material = FindUInt(primitive, "material")) {
                prim.material = pAsset_Root.materials.Retrieve(material->GetUint());
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

    Value *extras = FindObject(pJSON_Object, "extras");
    if (nullptr != extras ) {
        if (Value* curTargetNames = FindArray(*extras, "targetNames")) {
            this->targetNames.resize(curTargetNames->Size());
            for (unsigned int i = 0; i < curTargetNames->Size(); ++i) {
                Value& targetNameValue = (*curTargetNames)[i];
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
        cameraProperties.ortographic.xmag = MemberOrDefault(obj, "xmag", 1.f);
        cameraProperties.ortographic.ymag = MemberOrDefault(obj, "ymag", 1.f);
        cameraProperties.ortographic.zfar = MemberOrDefault(obj, "zfar", 100.f);
        cameraProperties.ortographic.znear = MemberOrDefault(obj, "znear", 0.01f);
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
        outerConeAngle = MemberOrDefault(*spot, "outerConeAngle", M_PI / 4.0f);
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

    Value *curSkin = FindUInt(obj, "skin");
    if (nullptr != curSkin) {
        this->skin = r.skins.Retrieve(curSkin->GetUint());
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

        if (Value *versionString = FindString(*obj, "version")) {
            version = versionString->GetString();
        } else if (Value *versionNumber = FindNumber(*obj, "version")) {
            char buf[4];

            ai_snprintf(buf, 4, "%.1f", versionNumber->GetDouble());

            version = buf;
        }

        Value *curProfile = FindObject(*obj, "profile");
        if (nullptr != curProfile) {
            ReadMember(*curProfile, "api", this->profile.api);
            ReadMember(*curProfile, "version", this->profile.version);
        }
    }

    if (version.empty() || version[0] != '2') {
        throw DeadlyImportError("GLTF: Unsupported glTF version: " + version);
    }
}

//
// Asset methods implementation
//

inline void Asset::ReadBinaryHeader(IOStream &stream, std::vector<char> &sceneData) {
    GLB_Header header;
    if (stream.Read(&header, sizeof(header), 1) != 1) {
        throw DeadlyImportError("GLTF: Unable to read the file header");
    }

    if (strncmp((char *)header.magic, AI_GLB_MAGIC_NUMBER, sizeof(header.magic)) != 0) {
        throw DeadlyImportError("GLTF: Invalid binary glTF file");
    }

    AI_SWAP4(header.version);
    asset.version = to_string(header.version);
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

    // read the scene data

    mSceneLength = chunk.chunkLength;
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

inline void Asset::Load(const std::string &pFile, bool isBinary) {
    mCurrentAssetDir.clear();
    /*int pos = std::max(int(pFile.rfind('/')), int(pFile.rfind('\\')));
    if (pos != int(std::string::npos)) */

    mCurrentAssetDir = glTFCommon::getCurrentAssetDir(pFile);

    shared_ptr<IOStream> stream(OpenFile(pFile.c_str(), "rb", true));
    if (!stream) {
        throw DeadlyImportError("GLTF: Could not open file for reading");
    }

    // is binary? then read the header
    std::vector<char> sceneData;
    if (isBinary) {
        SetAsBinary(); // also creates the body buffer
        ReadBinaryHeader(*stream, sceneData);
    } else {
        mSceneLength = stream->FileSize();
        mBodyLength = 0;

        // read the scene data

        sceneData.resize(mSceneLength + 1);
        sceneData[mSceneLength] = '\0';

        if (stream->Read(&sceneData[0], 1, mSceneLength) != mSceneLength) {
            throw DeadlyImportError("GLTF: Could not read the file contents");
        }
    }

    // parse the JSON document

    Document doc;
    doc.ParseInsitu(&sceneData[0]);

    if (doc.HasParseError()) {
        char buffer[32];
        ai_snprintf(buffer, 32, "%d", static_cast<int>(doc.GetErrorOffset()));
        throw DeadlyImportError(std::string("GLTF: JSON parse error, offset ") + buffer + ": " + GetParseError_En(doc.GetParseError()));
    }

    if (!doc.IsObject()) {
        throw DeadlyImportError("GLTF: JSON document root must be a JSON object");
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

    // Currently Draco is not supported
    if (extensionsRequired.KHR_draco_mesh_compression) {
        throw DeadlyImportError("GLTF: Draco mesh compression not currently supported.");
    }

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

    // Force reading of skins since they're not always directly referenced
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

#undef CHECK_EXT
}

inline IOStream *Asset::OpenFile(std::string path, const char *mode, bool /*absolute*/) {
#ifdef ASSIMP_API
    return mIOSystem->Open(path, mode);
#else
    if (path.size() < 2) return 0;
    if (!absolute && path[1] != ':' && path[0] != '/') { // relative?
        path = mCurrentAssetDir + path;
    }
    FILE *f = fopen(path.c_str(), mode);
    return f ? new IOStream(f) : 0;
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

#ifdef _WIN32
#    pragma warning(pop)
#endif // _WIN32

} // namespace glTF2
