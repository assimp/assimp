/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team
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

#include "StringUtils.h"

namespace glTF {

namespace {
    
    //
    // JSON Value reading helpers
    //

    template<class T>
    struct ReadHelper { static bool Read(Value& val, T& out) {
        return val.IsInt() ? out = static_cast<T>(val.GetInt()), true : false;
    }};
    
    template<> struct ReadHelper<bool> { static bool Read(Value& val, bool& out) {
        return val.IsBool() ? out = val.GetBool(), true : false;
    }};

    template<> struct ReadHelper<float> { static bool Read(Value& val, float& out) {
        return val.IsNumber() ? out = static_cast<float>(val.GetDouble()), true : false;
    }};
    
    template<unsigned int N> struct ReadHelper<float[N]> { static bool Read(Value& val, float (&out)[N]) {
        if (!val.IsArray() || val.Size() != N) return false;
        for (unsigned int i = 0; i < N; ++i) {
            if (val[i].IsNumber())
                out[i] = static_cast<float>(val[i].GetDouble());
        }
        return true;
    }};

    template<> struct ReadHelper<const char*> { static bool Read(Value& val, const char*& out) {
        return val.IsString() ? (out = val.GetString(), true) : false;
    }};

    template<> struct ReadHelper<std::string> { static bool Read(Value& val, std::string& out) {
        return val.IsString() ? (out = std::string(val.GetString(), val.GetStringLength()), true) : false;
    }};

    template<class T> struct ReadHelper< Nullable<T> > { static bool Read(Value& val, Nullable<T>& out) {
        return out.isPresent = ReadHelper<T>::Read(val, out.value);
    }};

    template<class T>
    inline static bool ReadValue(Value& val, T& out)
    {
        return ReadHelper<T>::Read(val, out);
    }

    template<class T>
    inline static bool ReadMember(Value& obj, const char* id, T& out)
    {
        Value::MemberIterator it = obj.FindMember(id);
        if (it != obj.MemberEnd()) {
            return ReadHelper<T>::Read(it->value, out);
        }
        return false;
    }

    template<class T>
    inline static T MemberOrDefault(Value& obj, const char* id, T defaultValue)
    {
        T out;
        return ReadMember(obj, id, out) ? out : defaultValue;
    }

    inline Value* FindMember(Value& val, const char* id)
    {
        Value::MemberIterator it = val.FindMember(id);
        return (it != val.MemberEnd()) ? &it->value : 0;
    }

    inline Value* FindString(Value& val, const char* id)
    {
        Value::MemberIterator it = val.FindMember(id);
        return (it != val.MemberEnd() && it->value.IsString()) ? &it->value : 0;
    }

    inline Value* FindArray(Value& val, const char* id)
    {
        Value::MemberIterator it = val.FindMember(id);
        return (it != val.MemberEnd() && it->value.IsArray()) ? &it->value : 0;
    }

    inline Value* FindObject(Value& val, const char* id)
    {
        Value::MemberIterator it = val.FindMember(id);
        return (it != val.MemberEnd() && it->value.IsObject()) ? &it->value : 0;
    }
}

//
// LazyDict methods
//

template<class T>
inline LazyDict<T>::LazyDict(Asset& asset, const char* dictId, const char* extId)
    : mDictId(dictId), mExtId(extId), mDict(0), mAsset(asset)
{
    asset.mDicts.push_back(this); // register to the list of dictionaries
}

template<class T>
inline LazyDict<T>::~LazyDict()
{
    for (size_t i = 0; i < mObjs.size(); ++i) {
        delete mObjs[i];
    }
}


template<class T>
inline void LazyDict<T>::AttachToDocument(Document& doc)
{
    Value* container = 0;

    if (mExtId) {
        if (Value* exts = FindObject(doc, "extensions")) {
            container = FindObject(*exts, mExtId);
        }
    }
    else {
        container = &doc;
    }

    if (container) {
        mDict = FindObject(*container, mDictId);
    }
}

template<class T>
inline void LazyDict<T>::DetachFromDocument()
{
    mDict = 0;
}

template<class T>
Ref<T> LazyDict<T>::Get(unsigned int i)
{
    return Ref<T>(mObjs, i);
}

template<class T>
Ref<T> LazyDict<T>::Get(const char* id)
{
    id = T::TranslateId(mAsset, id);

    typename Dict::iterator it = mObjsById.find(id);
    if (it != mObjsById.end()) { // already created?
        return Ref<T>(mObjs, it->second);
    }

    // read it from the JSON object
    if (!mDict) {
        throw DeadlyImportError("GLTF: Missing section \"" + std::string(mDictId) + "\"");
    }

    Value::MemberIterator obj = mDict->FindMember(id);
    if (obj == mDict->MemberEnd()) {
        throw DeadlyImportError("GLTF: Missing object with id \"" + std::string(id) + "\" in \"" + mDictId + "\"");
    }
    if (!obj->value.IsObject()) {
        throw DeadlyImportError("GLTF: Object with id \"" + std::string(id) + "\" is not a JSON object");
    }

    // create an instance of the given type
    T* inst = new T();
    inst->id = id;
    ReadMember(obj->value, "name", inst->name);
    inst->Read(obj->value, mAsset);
    return Add(inst);
}

template<class T>
Ref<T> LazyDict<T>::Add(T* obj)
{
    unsigned int idx = unsigned(mObjs.size());
    mObjs.push_back(obj);
    mObjsById[obj->id] = idx;
    mAsset.mUsedIds[obj->id] = true;
    return Ref<T>(mObjs, idx);
}

template<class T>
Ref<T> LazyDict<T>::Create(const char* id)
{
    Asset::IdMap::iterator it = mAsset.mUsedIds.find(id);
    if (it != mAsset.mUsedIds.end()) {
        throw DeadlyImportError("GLTF: two objects with the same ID exist");
    }
    T* inst = new T();
    inst->id = id;
    return Add(inst);
}


//
// glTF dictionary objects methods
//


inline Buffer::Buffer()
: byteLength(0), type(Type_arraybuffer), mIsSpecial(false)
{ }

inline const char* Buffer::TranslateId(Asset& r, const char* id)
{
    // Compatibility with old spec
    if (r.extensionsUsed.KHR_binary_glTF && strcmp(id, "KHR_binary_glTF") == 0) {
        return "binary_glTF";
    }

    return id;
}

inline void Buffer::Read(Value& obj, Asset& r)
{
    size_t statedLength = MemberOrDefault<size_t>(obj, "byteLength", 0);
    byteLength = statedLength;

    Value* it = FindString(obj, "uri");
    if (!it) {
        if (statedLength > 0) {
            throw DeadlyImportError("GLTF: buffer with non-zero length missing the \"uri\" attribute");
        }
        return;
    }

    const char* uri = it->GetString();

    Util::DataURI dataURI;
    if (ParseDataURI(uri, it->GetStringLength(), dataURI)) {
        if (dataURI.base64) {
            uint8_t* data = 0;
            this->byteLength = Util::DecodeBase64(dataURI.data, dataURI.dataLength, data);
            this->mData.reset(data);

            if (statedLength > 0 && this->byteLength != statedLength) {
                throw DeadlyImportError("GLTF: buffer \"" + id + "\", expected " + std::to_string(statedLength) +
                    " bytes, but found " + std::to_string(dataURI.dataLength));
            }
        }
        else { // assume raw data
            if (statedLength != dataURI.dataLength) {
                throw DeadlyImportError("GLTF: buffer \"" + id + "\", expected " + std::to_string(statedLength) +
                                        " bytes, but found " + std::to_string(dataURI.dataLength));
            }

            this->mData.reset(new uint8_t[dataURI.dataLength]);
            memcpy( this->mData.get(), dataURI.data, dataURI.dataLength );
        }
    }
    else { // Local file
        if (byteLength > 0) {
            IOStream* file = r.OpenFile(uri, "rb");
            if (file) {
                bool ok = LoadFromStream(*file, byteLength);
                delete file;
                
                if (!ok)
                    throw DeadlyImportError("GLTF: error while reading referenced file \"" + std::string(uri) + "\"" );
            }
            else {
                throw DeadlyImportError("GLTF: could not open referenced file \"" + std::string(uri) + "\"");
            }
        }
    }
}

inline bool Buffer::LoadFromStream(IOStream& stream, size_t length, size_t baseOffset)
{
    byteLength = length ? length : stream.FileSize();

    if (baseOffset) {
        stream.Seek(baseOffset, aiOrigin_SET);
    }

    mData.reset(new uint8_t[byteLength]);

    if (stream.Read(mData.get(), byteLength, 1) != 1) {
        return false;
    }
    return true;
}

inline size_t Buffer::AppendData(uint8_t* data, size_t length)
{
    size_t offset = this->byteLength;
    Grow(length);
    memcpy(mData.get() + offset, data, length);
    return offset;
}

inline void Buffer::Grow(size_t amount)
{
    if (amount <= 0) return;
    uint8_t* b = new uint8_t[byteLength + amount];
    if (mData) memcpy(b, mData.get(), byteLength);
    mData.reset(b);
    byteLength += amount;
}


inline void BufferView::Read(Value& obj, Asset& r)
{
    const char* bufferId = MemberOrDefault<const char*>(obj, "buffer", 0);
    if (bufferId) {
        buffer = r.buffers.Get(bufferId);
    }
        
    byteOffset = MemberOrDefault(obj, "byteOffset", 0u);
    byteLength = MemberOrDefault(obj, "byteLength", 0u);
}



inline void Accessor::Read(Value& obj, Asset& r)
{
    const char* bufferViewId = MemberOrDefault<const char*>(obj, "bufferView", 0);
    if (bufferViewId) {
        bufferView = r.bufferViews.Get(bufferViewId);
    }

    byteOffset = MemberOrDefault(obj, "byteOffset", 0u);
    byteStride = MemberOrDefault(obj, "byteStride", 0u);
    componentType = MemberOrDefault(obj, "componentType", ComponentType_BYTE);
    count = MemberOrDefault(obj, "count", 0u);

    const char* typestr;
    type = ReadMember(obj, "type", typestr) ? AttribType::FromString(typestr) : AttribType::SCALAR;
}

inline unsigned int Accessor::GetNumComponents()
{
    return AttribType::GetNumComponents(type);
}

inline unsigned int Accessor::GetBytesPerComponent()
{
    return int(ComponentTypeSize(componentType));
}

inline unsigned int Accessor::GetElementSize()
{
    return GetNumComponents() * GetBytesPerComponent();
}

inline uint8_t* Accessor::GetPointer()
{
    if (!bufferView || !bufferView->buffer) return 0;
    uint8_t* basePtr = bufferView->buffer->GetPointer();
    if (!basePtr) return 0;

    size_t offset = byteOffset + bufferView->byteOffset;
    return basePtr + offset;
}

namespace {
    inline void CopyData(size_t count,
            const uint8_t* src, size_t src_stride,
                  uint8_t* dst, size_t dst_stride)
    {
        if (src_stride == dst_stride) {
            memcpy(dst, src, count * src_stride);
        }
        else {
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
}

template<class T>
bool Accessor::ExtractData(T*& outData)
{
    uint8_t* data = GetPointer();
    if (!data) return false;

    const size_t elemSize = GetElementSize();
    const size_t totalSize = elemSize * count;

    const size_t stride = byteStride ? byteStride : elemSize;

    const size_t targetElemSize = sizeof(T);
    ai_assert(elemSize <= targetElemSize);

    ai_assert(count*stride <= bufferView->byteLength);

    outData = new T[count];
    if (stride == elemSize && targetElemSize == elemSize) {
        memcpy(outData, data, totalSize);
    }
    else {
        for (size_t i = 0; i < count; ++i) {
            memcpy(outData + i, data + i*stride, elemSize);
        }
    }

    return true;
}

inline void Accessor::WriteData(size_t count, const void* src_buffer, size_t src_stride)
{
    uint8_t* buffer_ptr = bufferView->buffer->GetPointer();
    size_t offset = byteOffset + bufferView->byteOffset;

    size_t dst_stride = GetNumComponents() * GetBytesPerComponent();

    const uint8_t* src = reinterpret_cast<const uint8_t*>(src_buffer);
    uint8_t*       dst = reinterpret_cast<      uint8_t*>(buffer_ptr + offset);

    ai_assert(dst + count*dst_stride <= buffer_ptr + bufferView->buffer->byteLength);
    CopyData(count, src, src_stride, dst, dst_stride);
}



inline Accessor::Indexer::Indexer(Accessor& acc)
    : accessor(acc)
    , data(acc.GetPointer())
    , elemSize(acc.GetElementSize())
    , stride(acc.byteStride ? acc.byteStride : elemSize)
{

}

//! Accesses the i-th value as defined by the accessor
template<class T>
T Accessor::Indexer::GetValue(int i)
{
    ai_assert(data);
    ai_assert(i*stride < accessor.bufferView->byteLength);
    T value = T();
    memcpy(&value, data + i*stride, elemSize);
    //value >>= 8 * (sizeof(T) - elemSize);
    return value;
}

inline Image::Image()
    : width(0)
    , height(0)
    , mData(0)
    , mDataLength(0)
{

}

inline void Image::Read(Value& obj, Asset& r)
{
    // Check for extensions first (to detect binary embedded data) 
    if (Value* extensions = FindObject(obj, "extensions")) {
        if (r.extensionsUsed.KHR_binary_glTF) {
            if (Value* ext = FindObject(*extensions, "KHR_binary_glTF")) {

                width  = MemberOrDefault(*ext, "width", 0);
                height = MemberOrDefault(*ext, "height", 0);

                ReadMember(*ext, "mimeType", mimeType);

                const char* bufferViewId;
                if (ReadMember(*ext, "bufferView", bufferViewId)) {
                    Ref<BufferView> bv = r.bufferViews.Get(bufferViewId);
                    if (bv) {
                        mDataLength = bv->byteLength;
                        mData = new uint8_t[mDataLength];
                        memcpy(mData, bv->buffer->GetPointer() + bv->byteOffset, mDataLength);
                    }
                }
            }
        }
    }

    if (!mDataLength) {
        if (Value* uri = FindString(obj, "uri")) {
            const char* uristr = uri->GetString();

            Util::DataURI dataURI;
            if (ParseDataURI(uristr, uri->GetStringLength(), dataURI)) {
                mimeType = dataURI.mediaType;
                if (dataURI.base64) {
                    mDataLength = Util::DecodeBase64(dataURI.data, dataURI.dataLength, mData);
                }
            }
            else {
                this->uri = uristr;
            }
        }
    }
}

inline uint8_t* Image::StealData()
{
    uint8_t* data = mData;
    mDataLength = 0;
    mData = 0;
    return data;
}

inline void Image::SetData(uint8_t* data, size_t length, Asset& r)
{
    Ref<Buffer> b = r.GetBodyBuffer();
    if (b) { // binary file: append to body
        std::string bvId = r.FindUniqueID(this->id, "imgdata");
        bufferView = r.bufferViews.Create(bvId);

        bufferView->buffer = b;
        bufferView->byteLength = length;
        bufferView->byteOffset = b->AppendData(data, length);
    }
    else { // text file: will be stored as a data uri
        this->mData = data;
        this->mDataLength = length;
    }
}

inline void Texture::Read(Value& obj, Asset& r)
{
    const char* sourcestr;
    if (ReadMember(obj, "source", sourcestr)) {
        source = r.images.Get(sourcestr);
    }
}

namespace {
    inline void ReadMaterialProperty(Asset& r, Value& vals, const char* propName, TexProperty& out)
    {
        if (Value* prop = FindMember(vals, propName)) {
            if (prop->IsString()) {
                out.texture = r.textures.Get(prop->GetString());
            }
            else {
                ReadValue(*prop, out.color);
            }
        }
    }
}

inline void Material::Read(Value& material, Asset& r)
{
    SetDefaults();

    if (Value* values = FindObject(material, "values")) {
        ReadMaterialProperty(r, *values, "ambient", this->ambient);
        ReadMaterialProperty(r, *values, "diffuse", this->diffuse);
        ReadMaterialProperty(r, *values, "specular", this->specular);

        ReadMember(*values, "shininess", shininess);
    }

    if (Value* extensions = FindObject(material, "extensions")) {
        if (r.extensionsUsed.KHR_materials_common) {
            if (Value* ext = FindObject(*extensions, "KHR_materials_common")) {
                if (Value* tnq = FindString(*ext, "technique")) {
                    const char* t = tnq->GetString();
                    if      (strcmp(t, "BLINN") == 0)    technique = Technique_BLINN;
                    else if (strcmp(t, "PHONG") == 0)    technique = Technique_PHONG;
                    else if (strcmp(t, "LAMBERT") == 0)  technique = Technique_LAMBERT;
                    else if (strcmp(t, "CONSTANT") == 0) technique = Technique_CONSTANT;
                }

                if (Value* values = FindObject(*ext, "values")) {
                    ReadMaterialProperty(r, *values, "ambient", this->ambient);
                    ReadMaterialProperty(r, *values, "diffuse", this->diffuse);
                    ReadMaterialProperty(r, *values, "specular", this->specular);

                    ReadMember(*values, "doubleSided", doubleSided);
                    ReadMember(*values, "transparent", transparent);
                    ReadMember(*values, "transparency", transparency);
                    ReadMember(*values, "shininess", shininess);
                }
            }
        }
    }
}

namespace {
    void SetVector(vec4& v, float x, float y, float z, float w)
        { v[0] = x; v[1] = y; v[2] = z; v[3] = w; }
}

inline void Material::SetDefaults()
{
    SetVector(ambient.color, 0, 0, 0, 1);
    SetVector(diffuse.color, 0, 0, 0, 1);
    SetVector(specular.color, 0, 0, 0, 1);
    SetVector(emission.color, 0, 0, 0, 1);

    doubleSided = false;
    transparent = false;
    transparency = 1.0;
    shininess = 0.0;

    technique = Technique_undefined;
}

namespace {

    template<int N>
    inline int Compare(const char* attr, const char (&str)[N]) {
        return (strncmp(attr, str, N - 1) == 0) ? N - 1 : 0;
    }

    inline bool GetAttribVector(Mesh::Primitive& p, const char* attr, Mesh::AccessorList*& v, int& pos)
    {
        if ((pos = Compare(attr, "POSITION"))) {
            v = &(p.attributes.position);
        }
        else if ((pos = Compare(attr, "NORMAL"))) {
            v = &(p.attributes.normal);
        }
        else if ((pos = Compare(attr, "TEXCOORD"))) {
            v = &(p.attributes.texcoord);
        }
        else if ((pos = Compare(attr, "COLOR"))) {
            v = &(p.attributes.color);
        }
        else if ((pos = Compare(attr, "JOINT"))) {
            v = &(p.attributes.joint);
        }
        else if ((pos = Compare(attr, "JOINTMATRIX"))) {
            v = &(p.attributes.jointmatrix);
        }
        else if ((pos = Compare(attr, "WEIGHT"))) {
            v = &(p.attributes.weight);
        }
        else return false;
        return true;
    }
}

inline void Mesh::Read(Value& obj, Asset& r)
{  
    if (Value* primitives = FindArray(obj, "primitives")) {
        this->primitives.resize(primitives->Size());
        for (unsigned int i = 0; i < primitives->Size(); ++i) {
            Value& primitive = (*primitives)[i];

            Primitive& prim = this->primitives[i];
            prim.mode = MemberOrDefault(primitive, "mode", PrimitiveMode_TRIANGLES);

            if (Value* attrs = FindObject(primitive, "attributes")) {
                for (Value::MemberIterator it = attrs->MemberBegin(); it != attrs->MemberEnd(); ++it) {
                    if (!it->value.IsString()) continue;
                    const char* attr = it->name.GetString();
                    // Valid attribute semantics include POSITION, NORMAL, TEXCOORD, COLOR, JOINT, JOINTMATRIX,
                    // and WEIGHT.Attribute semantics can be of the form[semantic]_[set_index], e.g., TEXCOORD_0, TEXCOORD_1, etc.

                    int undPos = 0;
                    Mesh::AccessorList* vec = 0;
                    if (GetAttribVector(prim, attr, vec, undPos)) {
                        size_t idx = (attr[undPos] == '_') ? atoi(attr + undPos + 1) : 0;
                        if ((*vec).size() <= idx) (*vec).resize(idx + 1);
                        (*vec)[idx] = r.accessors.Get(it->value.GetString());
                    }
                }
            }

            if (Value* indices = FindString(primitive, "indices")) {
                prim.indices = r.accessors.Get(indices->GetString());
            }

            if (Value* material = FindString(primitive, "material")) {
                prim.material = r.materials.Get(material->GetString());
            }
        }
    }
}


inline void Camera::Read(Value& obj, Asset& r)
{
    type = MemberOrDefault(obj, "type", Camera::Perspective);

    const char* subobjId = (type == Camera::Orthographic) ? "ortographic" : "perspective";

    Value* it = FindObject(obj, subobjId);
    if (!it) throw DeadlyImportError("GLTF: Camera missing its parameters");

    if (type == Camera::Perspective) {
        perspective.aspectRatio = MemberOrDefault(*it, "aspectRatio", 0.f);
        perspective.yfov        = MemberOrDefault(*it, "yfov", 3.1415f/2.f);
        perspective.zfar        = MemberOrDefault(*it, "zfar", 100.f);
        perspective.znear       = MemberOrDefault(*it, "znear", 0.01f);
    }
    else {
        ortographic.xmag  = MemberOrDefault(obj, "xmag", 1.f);
        ortographic.ymag  = MemberOrDefault(obj, "ymag", 1.f);
        ortographic.zfar  = MemberOrDefault(obj, "zfar", 100.f);
        ortographic.znear = MemberOrDefault(obj, "znear", 0.01f);
    }
}

inline void Light::Read(Value& obj, Asset& r)
{
    SetDefaults();

    if (Value* type = FindString(obj, "type")) {
        const char* t = type->GetString();
        if      (strcmp(t, "ambient") == 0)     this->type = Type_ambient;
        else if (strcmp(t, "directional") == 0) this->type = Type_directional;
        else if (strcmp(t, "point") == 0)       this->type = Type_point;
        else if (strcmp(t, "spot") == 0)        this->type = Type_spot;

        if (this->type != Type_undefined) {
            if (Value* vals = FindString(obj, t)) {
                ReadMember(*vals, "color", color);

                ReadMember(*vals, "constantAttenuation", constantAttenuation);
                ReadMember(*vals, "linearAttenuation", linearAttenuation);
                ReadMember(*vals, "quadraticAttenuation", quadraticAttenuation);
                ReadMember(*vals, "distance", distance);

                ReadMember(*vals, "falloffAngle", falloffAngle);
                ReadMember(*vals, "falloffExponent", falloffExponent);
            }
        }
    }
}

inline void Light::SetDefaults()
{
    #ifndef M_PI
        const float M_PI = 3.14159265358979323846f;
    #endif

    type = Type_undefined;

    SetVector(color, 0.f, 0.f, 0.f, 1.f);

    constantAttenuation = 0.f;
    linearAttenuation = 1.f;
    quadraticAttenuation = 1.f;
    distance = 0.f;

    falloffAngle = static_cast<float>(M_PI / 2.f);
    falloffExponent = 0.f;
}

inline void Node::Read(Value& obj, Asset& r)
{
    if (Value* children = FindArray(obj, "children")) {
        this->children.reserve(children->Size());
        for (unsigned int i = 0; i < children->Size(); ++i) {
            Value& child = (*children)[i];
            if (child.IsString()) {
                // get/create the child node
                Ref<Node> chn = r.nodes.Get(child.GetString());
                if (chn) this->children.push_back(chn);
            }
        }
    }

    
    if (Value* matrix = FindArray(obj, "matrix")) {
        ReadValue(*matrix, this->matrix);
    }
    else {
        ReadMember(obj, "translation", translation);
        ReadMember(obj, "scale", scale);
        ReadMember(obj, "rotation", rotation);
    }

    if (Value* meshes = FindArray(obj, "meshes")) {
        unsigned numMeshes = (unsigned)meshes->Size();

        std::vector<unsigned int> meshList;

        this->meshes.reserve(numMeshes);
        for (unsigned i = 0; i < numMeshes; ++i) {
            if ((*meshes)[i].IsString()) {
                Ref<Mesh> mesh = r.meshes.Get((*meshes)[i].GetString());
                if (mesh) this->meshes.push_back(mesh);
            }
        }
    }

    if (Value* camera = FindString(obj, "camera")) {
        this->camera = r.cameras.Get(camera->GetString());
        if (this->camera)
            this->camera->id = this->id;
    }

    // TODO load "skeletons", "skin", "jointName"

    if (Value* extensions = FindObject(obj, "extensions")) {
        if (r.extensionsUsed.KHR_materials_common) {

            if (Value* ext = FindObject(*extensions, "KHR_materials_common")) {
                if (Value* light = FindString(*ext, "light")) {
                    this->light = r.lights.Get(light->GetString());
                }
            }

        }
    }
}

inline void Scene::Read(Value& obj, Asset& r)
{
    if (Value* array = FindArray(obj, "nodes")) {
        for (unsigned int i = 0; i < array->Size(); ++i) {
            if (!(*array)[i].IsString()) continue;
            Ref<Node> node = r.nodes.Get((*array)[i].GetString());
            if (node)
                this->nodes.push_back(node);
        }
    }
}


inline void AssetMetadata::Read(Document& doc)
{
    // read the version, etc.
    int statedVersion = 0;
    if (Value* obj = FindObject(doc, "asset")) {
        ReadMember(*obj, "copyright", copyright);
        ReadMember(*obj, "generator", generator);

        premultipliedAlpha = MemberOrDefault(*obj, "premultipliedAlpha", false);
        statedVersion = MemberOrDefault(*obj, "version", 0);

        if (Value* profile = FindObject(*obj, "profile")) {
            ReadMember(*profile, "api",     this->profile.api);
            ReadMember(*profile, "version", this->profile.version);
        }
    }

    version = std::max(statedVersion, version);
    if (version == 0) {
        // if missing version, we'll assume version 1...
        version = 1;
    }

    if (version != 1) {
        char msg[128];
        ai_snprintf(msg, 128, "GLTF: Unsupported glTF version: %d", version);
        throw DeadlyImportError(msg);
    }
}



//
// Asset methods implementation
//

inline void Asset::ReadBinaryHeader(IOStream& stream)
{
    GLB_Header header;
    if (stream.Read(&header, sizeof(header), 1) != 1) {
        throw DeadlyImportError("GLTF: Unable to read the file header");
    }

    if (strncmp((char*)header.magic, AI_GLB_MAGIC_NUMBER, sizeof(header.magic)) != 0) {
        throw DeadlyImportError("GLTF: Invalid binary glTF file");
    }

    AI_SWAP4(header.version);
    asset.version = header.version;
    if (header.version != 1) {
        throw DeadlyImportError("GLTF: Unsupported binary glTF version");
    }

    AI_SWAP4(header.sceneFormat);
    if (header.sceneFormat != SceneFormat_JSON) {
        throw DeadlyImportError("GLTF: Unsupported binary glTF scene format");
    }

    AI_SWAP4(header.length);
    AI_SWAP4(header.sceneLength);

    mSceneLength = static_cast<size_t>(header.sceneLength);

    mBodyOffset = sizeof(header)+mSceneLength;
    mBodyOffset = (mBodyOffset + 3) & ~3; // Round up to next multiple of 4

    mBodyLength = header.length - mBodyOffset;
}

inline void Asset::Load(const std::string& pFile, bool isBinary)
{
    mCurrentAssetDir.clear();
    int pos = std::max(int(pFile.rfind('/')), int(pFile.rfind('\\')));
    if (pos != int(std::string::npos)) mCurrentAssetDir = pFile.substr(0, pos + 1);

    shared_ptr<IOStream> stream(OpenFile(pFile.c_str(), "rb", true));
    if (!stream) {
        throw DeadlyImportError("GLTF: Could not open file for reading");
    }

    // is binary? then read the header
    if (isBinary) {
        SetAsBinary(); // also creates the body buffer
        ReadBinaryHeader(*stream);
    }
    else {
        mSceneLength = stream->FileSize();
        mBodyLength = 0;
    }


    // read the scene data

    std::vector<char> sceneData(mSceneLength + 1);
    sceneData[mSceneLength] = '\0';

    if (stream->Read(&sceneData[0], 1, mSceneLength) != mSceneLength) {
        throw DeadlyImportError("GLTF: Could not read the file contents");
    }


    // parse the JSON document

    Document doc;
    doc.ParseInsitu(&sceneData[0]);

    if (doc.HasParseError()) {
        char buffer[32];
        ai_snprintf(buffer, 32, "%d", static_cast<int>(doc.GetErrorOffset()));
        throw DeadlyImportError(std::string("GLTF: JSON parse error, offset ") + buffer + ": "
            + GetParseError_En(doc.GetParseError()));
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

    // Prepare the dictionaries
    for (size_t i = 0; i < mDicts.size(); ++i) {
        mDicts[i]->AttachToDocument(doc);
    }



    // Read the "scene" property, which specifies which scene to load
    // and recursively load everything referenced by it
    if (Value* scene = FindString(doc, "scene")) {
        this->scene = scenes.Get(scene->GetString());
    }

    // Clean up
    for (size_t i = 0; i < mDicts.size(); ++i) {
        mDicts[i]->DetachFromDocument();
    }
}

inline void Asset::SetAsBinary()
{
    if (!extensionsUsed.KHR_binary_glTF) {
        extensionsUsed.KHR_binary_glTF = true;
        mBodyBuffer = buffers.Create("binary_glTF");
        mBodyBuffer->MarkAsSpecial();
    }
}


inline void Asset::ReadExtensionsUsed(Document& doc)
{
    Value* extsUsed = FindArray(doc, "extensionsUsed");
    if (!extsUsed) return;

    std::gltf_unordered_map<std::string, bool> exts;

    for (unsigned int i = 0; i < extsUsed->Size(); ++i) {
        if ((*extsUsed)[i].IsString()) {
            exts[(*extsUsed)[i].GetString()] = true;
        }
    }

    #define CHECK_EXT(EXT) \
        if (exts.find(#EXT) != exts.end()) extensionsUsed.EXT = true;

    CHECK_EXT(KHR_binary_glTF);
    CHECK_EXT(KHR_materials_common);

    #undef CHECK_EXT
}

inline IOStream* Asset::OpenFile(std::string path, const char* mode, bool absolute)
{
    #ifdef ASSIMP_API
        return mIOSystem->Open(path, mode);
    #else
        if (path.size() < 2) return 0;
        if (!absolute && path[1] != ':' && path[0] != '/') { // relative?
            path = mCurrentAssetDir + path;
        }
        FILE* f = fopen(path.c_str(), mode);
        return f ? new IOStream(f) : 0;
    #endif
}

inline std::string Asset::FindUniqueID(const std::string& str, const char* suffix)
{
    std::string id = str;

    if (!id.empty()) {
        if (mUsedIds.find(id) == mUsedIds.end())
            return id;

        id += "_";
    }

    id += suffix;

    Asset::IdMap::iterator it = mUsedIds.find(id);
    if (it == mUsedIds.end())
        return id;

    char buffer[256];
    int offset = ai_snprintf(buffer, sizeof(buffer), "%s_", id.c_str());
    for (int i = 0; it != mUsedIds.end(); ++i) {
        ai_snprintf(buffer + offset, sizeof(buffer) - offset, "%d", i);
        id = buffer;
        it = mUsedIds.find(id);
    }
    
    return id;
}

namespace Util {

    inline 
    bool ParseDataURI(const char* const_uri, size_t uriLen, DataURI& out) {
        if ( NULL == const_uri ) {
            return false;
        }

        if (const_uri[0] != 0x10) { // we already parsed this uri?
            if (strncmp(const_uri, "data:", 5) != 0) // not a data uri?
                return false;
        }

        // set defaults
        out.mediaType = "text/plain";
        out.charset = "US-ASCII";
        out.base64 = false;

        char* uri = const_cast<char*>(const_uri);
        if (uri[0] != 0x10) {
            uri[0] = 0x10;
            uri[1] = uri[2] = uri[3] = uri[4] = 0;

            size_t i = 5, j;
            if (uri[i] != ';' && uri[i] != ',') { // has media type?
                uri[1] = char(i);
                for (; uri[i] != ';' && uri[i] != ',' && i < uriLen; ++i) {
                    // nothing to do!
                }
            }
            while (uri[i] == ';' && i < uriLen) {
                uri[i++] = '\0';
                for (j = i; uri[i] != ';' && uri[i] != ',' && i < uriLen; ++i) {
                    // nothing to do!
                }

                if ( strncmp( uri + j, "charset=", 8 ) == 0 ) {
                    uri[2] = char(j + 8);
                } else if ( strncmp( uri + j, "base64", 6 ) == 0 ) {
                    uri[3] = char(j);
                }
            }
            if (i < uriLen) {
                uri[i++] = '\0';
                uri[4] = char(i);
            } else {
                uri[1] = uri[2] = uri[3] = 0;
                uri[4] = 5;
            }
        }

        if ( uri[ 1 ] != 0 ) {
            out.mediaType = uri + uri[ 1 ];
        }
        if ( uri[ 2 ] != 0 ) {
            out.charset = uri + uri[ 2 ];
        }
        if ( uri[ 3 ] != 0 ) {
            out.base64 = true;
        }
        out.data = uri + uri[4];
        out.dataLength = (uri + uriLen) - out.data;

        return true;
    }

    template<bool B>
    struct DATA
    {
        static const uint8_t tableDecodeBase64[128];
    };

    template<bool B>
    const uint8_t DATA<B>::tableDecodeBase64[128] = {
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0, 64,  0,  0,
         0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
         0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0
    };

    inline char EncodeCharBase64(uint8_t b)
    {
        return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="[size_t(b)];
    }

    inline uint8_t DecodeCharBase64(char c)
    {
        return DATA<true>::tableDecodeBase64[size_t(c)]; // TODO faster with lookup table or ifs?
        /*if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return 64; // '-' */
    }

    inline size_t DecodeBase64(const char* in, size_t inLength, uint8_t*& out)
    {
        ai_assert(inLength % 4 == 0);

        if (inLength < 4) {
            out = 0;
            return 0;
        }

        int nEquals = int(in[inLength - 1] == '=') +
                      int(in[inLength - 2] == '=');

        size_t outLength = (inLength * 3) / 4 - nEquals;
        out = new uint8_t[outLength];
        memset(out, 0, outLength);

        size_t i, j = 0;

        for (i = 0; i + 4 < inLength; i += 4) {
            uint8_t b0 = DecodeCharBase64(in[i]);
            uint8_t b1 = DecodeCharBase64(in[i + 1]);
            uint8_t b2 = DecodeCharBase64(in[i + 2]);
            uint8_t b3 = DecodeCharBase64(in[i + 3]);

            out[j++] = (uint8_t)((b0 << 2) | (b1 >> 4));
            out[j++] = (uint8_t)((b1 << 4) | (b2 >> 2));
            out[j++] = (uint8_t)((b2 << 6) | b3);
        }

        {
            uint8_t b0 = DecodeCharBase64(in[i]);
            uint8_t b1 = DecodeCharBase64(in[i + 1]);
            uint8_t b2 = DecodeCharBase64(in[i + 2]);
            uint8_t b3 = DecodeCharBase64(in[i + 3]);

            out[j++] = (uint8_t)((b0 << 2) | (b1 >> 4));
            if (b2 < 64) out[j++] = (uint8_t)((b1 << 4) | (b2 >> 2));
            if (b3 < 64) out[j++] = (uint8_t)((b2 << 6) | b3);
        }

        return outLength;
    }



    inline void EncodeBase64(
        const uint8_t* in, size_t inLength,
        std::string& out)
    {
        size_t outLength = ((inLength + 2) / 3) * 4;

        size_t j = out.size();
        out.resize(j + outLength);

        for (size_t i = 0; i <  inLength; i += 3) {
            uint8_t b = (in[i] & 0xFC) >> 2;
            out[j++] = EncodeCharBase64(b);

            b = (in[i] & 0x03) << 4;
            if (i + 1 < inLength) {
                b |= (in[i + 1] & 0xF0) >> 4;
                out[j++] = EncodeCharBase64(b);

                b = (in[i + 1] & 0x0F) << 2;
                if (i + 2 < inLength) {
                    b |= (in[i + 2] & 0xC0) >> 6;
                    out[j++] = EncodeCharBase64(b);

                    b = in[i + 2] & 0x3F;
                    out[j++] = EncodeCharBase64(b);
                }
                else {
                    out[j++] = EncodeCharBase64(b);
                    out[j++] = '=';
                }
            }
            else {
                out[j++] = EncodeCharBase64(b);
                out[j++] = '=';
                out[j++] = '=';
            }
        }
    }

}

}


