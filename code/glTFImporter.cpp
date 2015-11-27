/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2015, assimp team
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

#ifndef ASSIMP_BUILD_NO_GLTF_IMPORTER

#include "glTFImporter.h"
#include "StreamReader.h"
#include "DefaultIOSystem.h"

#include <boost/scoped_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/ai_assert.h>
#include <assimp/DefaultLogger.hpp>

#include "glTFFileData.h"
#include "glTFUtil.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

using namespace rapidjson;

using namespace Assimp;
using namespace Assimp::glTF;


using boost::shared_ptr;
using boost::scoped_ptr;

// (cannot typedef' templated classes, and "using" is c++11)
#define Ptr shared_ptr

// (used everywhere, and cannot use "auto")
typedef rapidjson::Value::MemberIterator MemIt;


//
// JSON Value reading helpers
//


#define GETF(VAL, OUT) { if ((VAL).IsNumber()) (OUT) = static_cast<float>((VAL).GetDouble()); }

template<class T>
struct ReadHelper { };

template<> struct ReadHelper<int> { static bool Read(Value& val, int& out) {
    return val.IsInt() ? val.GetInt(), true : false;
}};

template<> struct ReadHelper<unsigned int> { static bool Read(Value& val, unsigned int& out) {
    return val.IsInt() ? out = static_cast<unsigned int>(val.GetInt()), true : false;
}};

template<> struct ReadHelper<float> { static bool Read(Value& val, float& out) {
    return val.IsNumber() ? out = static_cast<float>(val.GetDouble()), true : false;
}};

template<> struct ReadHelper<const char*> { static bool Read(Value& val, const char*& out) {
    return val.IsString() ? out = val.GetString(), true : false;
}};

template<> struct ReadHelper<std::string> { static bool Read(Value& val, std::string& out) {
    return val.IsString() ? out = val.GetString(), true : false;
}};

template<> struct ReadHelper<aiColor3D> { static bool Read(Value& v, aiColor3D& out) {
    if (!v.IsArray() || v.Size() < 3) return false;
    GETF(v[0], out.r); GETF(v[1], out.g); GETF(v[2], out.b);
    return true;
}};

template<> struct ReadHelper<aiVector3D> { static bool Read(Value& v, aiVector3D& out) {
    if (!v.IsArray() || v.Size() != 3) return false;
    GETF(v[0], out.x); GETF(v[1], out.y); GETF(v[2], out.z);
    return true;
}};

template<> struct ReadHelper<aiQuaternion> { static bool Read(Value& v, aiQuaternion& out) {
    if (!v.IsArray() || v.Size() != 4) return false;
    GETF(v[0], out.x); GETF(v[1], out.y); GETF(v[2], out.z); GETF(v[3], out.w);
    return true;
}};

template<> struct ReadHelper<aiMatrix4x4> { static bool Read(Value& v, aiMatrix4x4& o) {
    if (!v.IsArray() || v.Size() != 16) return false;
    GETF(v[ 0], o.a1); GETF(v[ 1], o.b1); GETF(v[ 2], o.c1); GETF(v[ 3], o.d1);
    GETF(v[ 4], o.a2); GETF(v[ 5], o.b2); GETF(v[ 6], o.c2); GETF(v[ 7], o.d2);
    GETF(v[ 8], o.a3); GETF(v[ 9], o.b3); GETF(v[10], o.c3); GETF(v[11], o.d3);
    GETF(v[12], o.a4); GETF(v[13], o.b4); GETF(v[14], o.c4); GETF(v[15], o.d4);
    return true;
}};

template<class T>
inline static bool Read(Value& val, T& out)
{
    return ReadHelper<T>::Read(val, out);
}

template<class T>
inline static bool ReadMember(Value& obj, const char* id, T& out)
{
    MemIt it = obj.FindMember(id);
    if (it != obj.MemberEnd()) {
        return ReadHelper<T>::Read(it->value, out);
    }
    return false;
}

template<class T>
inline static T TryReadMember(Value& obj, const char* id, T defaultValue)
{
    T out;
    return ReadMember(obj, id, out) ? out : defaultValue;
}


//! References a sequence of loaded elements (e.g. meshes)
typedef std::pair<unsigned int, unsigned int> Range;



//
// glTFReader class
//

//! Manages lazy loading of the glTF top-level objects, and keeps a reference to them by ID
template< class T, class INST, T(INST::*FACTORY_FN)(const char*, Value&)>
class LazyDict
{
    typedef typename std::gltf_unordered_map<std::string, T> Map;

    Value*      mDict;     //! JSON dictionary object
    const char* mDictId;   //! ID of the dictionary object
    INST&       mInstance; //! The reader object instance
    Map         mReadObjs; //! The read objects

public:
    LazyDict(INST& instance, const char* dictId)
        : mDictId(dictId), mInstance(instance)
    {
        Document& doc = mInstance.GetDocument();

        MemIt it = doc.FindMember(dictId);
        mDict = (it != doc.MemberEnd() && it->value.IsObject()) ? &it->value : 0;
    }

    T Get(const char* id)
    {
        if (!mDict) return T(); // section was missing

        typename Map::iterator it = mReadObjs.find(id);
        if (it != mReadObjs.end()) { // already created?
            return it->second;
        }

        // read it from the JSON object
        MemIt obj = mDict->FindMember(id);
        if (obj == mDict->MemberEnd()) {
            throw DeadlyImportError("Missing object with id \"" + std::string(id) + "\" in \"" + mDictId + "\"");
        }

        // create an instance of the given type
        T val = (mInstance.*FACTORY_FN)(id, obj->value);
        mReadObjs[id] = val;
        return val;
    }
};

struct Buffer;
struct BufferView;
struct Accessor;

struct Image;
struct Texture;

//! Handles the reading of the glTF JSON document
class glTFReader
{
    aiScene* mScene;
    Document& mDoc;
    IOSystem& mIO;

    // Vectors of imported objects, will be copied to mScene
    std::vector<aiMaterial*> mImpMaterials;
    std::vector<aiMesh*>     mImpMeshes;
    std::vector<aiTexture*>  mImpTextures;

    Extensions mExtensions;

    Ptr<Buffer> mBodyBuffer; //! Special buffer containing the body data


    Ptr<Buffer>     LoadBuffer(const char* id, Value& obj);
    Ptr<BufferView> LoadBufferView(const char* id, Value& obj);
    Ptr<Accessor>   LoadAccessor(const char* id, Value& obj);

    Ptr<Image>      LoadImage(const char* id, Value& obj);
    Ptr<Texture>    LoadTexture(const char* id, Value& obj);

    aiNode*         LoadNode(const char* id, Value& node);
    Range           LoadMesh(const char* id, Value& mesh);
    unsigned int    LoadMaterial(const char* id, Value& material);

    typedef glTFReader T; // (to shorten next declarations)

    LazyDict<Ptr<Accessor>,   T, &T::LoadAccessor>   mAccessors;
    //LazyDict<Animation*,    T, &T::LoadAnimation>  mAnimations;
    //LazyDict<Asset*,        T, &T::LoadAsset>      mAssets;
    LazyDict<Ptr<Buffer>,     T, &T::LoadBuffer>     mBuffers;
    LazyDict<Ptr<BufferView>, T, &T::LoadBufferView> mBufferViews;
    //LazyDict<Camera*,       T, &T::LoadCamera>     mCameras;
    LazyDict<Ptr<Image>,      T, &T::LoadImage>      mImages;
    LazyDict<unsigned int,    T, &T::LoadMaterial>   mMaterials;
    LazyDict<Range,           T, &T::LoadMesh>       mMeshes;
    LazyDict<aiNode*,         T, &T::LoadNode>       mNodes;
    //LazyDict<Ptr<Program>,  T, &T::LoadProgram>    mPrograms;
    //LazyDict<Ptr<Sampler>,  T, &T::LoadSampler>    mSamplers;
    //LazyDict<Ptr<Shader>,   T, &T::LoadShader>     mShaders;
    //LazyDict<Ptr<Skin>,     T, &T::LoadSkin>       mSkins;
    //LazyDict<Ptr<Technique>,T, &T::LoadTechnique>  mTechniques;
    LazyDict<Ptr<Texture>,    T, &T::LoadTexture>    mTextures;


    void LoadScene(Value& scene)
    {
        MemIt nodesm = scene.FindMember("nodes");
        if (nodesm != scene.MemberEnd() && nodesm->value.IsArray()) {
            Value& nodes = nodesm->value;

            unsigned int numRootNodes = nodes.Size();
            if (numRootNodes == 1) { // a single root node: use it
                if (nodes[0].IsString()) {
                    mScene->mRootNode = mNodes.Get(nodes[0].GetString());
                }
            }
            else if (numRootNodes > 1) { // more than one root node: create a fake root
                aiNode* root = new aiNode("ROOT");
                root->mChildren = new aiNode*[numRootNodes];
                for (unsigned int i = 0; i < numRootNodes; ++i) {
                    if (nodes[i].IsString()) {
                        aiNode* node = mNodes.Get(nodes[i].GetString());
                        if (node) {
                            node->mParent = root;
                            root->mChildren[root->mNumChildren++] = node;
                        }
                    }
                }
                mScene->mRootNode = root;
            }
        }

        //if (!mScene->mRootNode) {
            //mScene->mRootNode = new aiNode("EMPTY");
        //}
    }

    void SetMaterialColorProperty(aiMaterial* mat, Value& vals, const char* propName, aiTextureType texType,
        const char* pKey, unsigned int type, unsigned int idx);

    void CopyData()
    {
        // TODO: it does not split the loaded vertices, should it?
        mScene->mFlags |= AI_SCENE_FLAGS_NON_VERBOSE_FORMAT;

        if (mImpMaterials.empty()) {
            mImpMaterials.push_back(new aiMaterial());
        }

        if (mImpMaterials.size()) {
            mScene->mNumMaterials = mImpMaterials.size();
            mScene->mMaterials = new aiMaterial*[mImpMaterials.size()];
            std::swap_ranges(mImpMaterials.begin(), mImpMaterials.end(), mScene->mMaterials);
        }

        if (mImpMeshes.size()) {
            mScene->mNumMeshes = mImpMeshes.size();
            mScene->mMeshes = new aiMesh*[mImpMeshes.size()];
            std::swap_ranges(mImpMeshes.begin(), mImpMeshes.end(), mScene->mMeshes);
        }

        if (mImpTextures.size()) {
            mScene->mNumTextures = mImpTextures.size();
            mScene->mTextures = new aiTexture*[mImpTextures.size()];
            std::swap_ranges(mImpTextures.begin(), mImpTextures.end(), mScene->mTextures);
        }
    }

public:
    glTFReader(aiScene* scene, Document& document, IOSystem& iohandler, shared_ptr<Buffer>& bodyBuff) :
        mScene(scene),
        mDoc(document),
        mIO(iohandler),
        mBodyBuffer(bodyBuff),
        mAccessors(*this, "accessors"),
        //mAnimations(*this, "animations"),
        //mAssets(*this, "assets"),
        mBuffers(*this, "buffers"),
        mBufferViews(*this, "bufferViews"),
        //mCameras(*this, "cameras"),
        mImages(*this, "images"),
        mMaterials(*this, "materials"),
        mMeshes(*this, "meshes"),
        mNodes(*this, "nodes"),
        //mPrograms(*this, "programs"),
        //mSamplers(*this, "samplers"),
        //mShaders(*this, "shaders"),
        //mSkins(*this, "skins"),
        //mTechniques(*this, "techniques"),
        mTextures(*this, "textures")
    {
        memset(&mExtensions, 0, sizeof(mExtensions));
    }

    Document& GetDocument()
    {
        return mDoc;
    }

    //! Main function
    void Load()
    {
        // read the used extensions
        MemIt extensionsUsed = mDoc.FindMember("extensionsUsed");
        if (extensionsUsed != mDoc.MemberEnd() && extensionsUsed->value.IsArray()) {
            std::gltf_unordered_map<std::string, bool> exts;

            for (unsigned int i = 0; i < extensionsUsed->value.Size(); ++i) {
                if (extensionsUsed->value[i].IsString()) {
                    exts[extensionsUsed->value[i].GetString()] = true;
                }
            }

            if (exts.find("KHR_binary_glTF") != exts.end()) {
                mExtensions.KHR_binary_glTF = true;
            }
        }


        const char* sceneId = 0;

        // the "scene" property specifies which scene to load
        {
            MemIt scene = mDoc.FindMember("scene");
            if (scene != mDoc.MemberEnd() && scene->value.IsString()) {
                sceneId = scene->value.GetString();
            }
        }

        MemIt scene;

        MemIt scenes = mDoc.FindMember("scenes");
        if (scenes != mDoc.MemberEnd() && scenes->value.IsObject()) {
            if (sceneId) {
                scene = scenes->value.FindMember(sceneId);
                if (scene == scenes->value.MemberEnd()) {
                    //ThrowException("Missing scene!");
                }
            }
            else { // if not specified, use the first one
                scene = scenes->value.MemberBegin();
            }
        }

        if (scene != scenes->value.MemberEnd()) {
            LoadScene(scene->value);
        }

        CopyData();
    }
};

struct Buffer
{
private:
    std::size_t byteLength;
    shared_ptr<uint8_t> data;

public:
    Buffer(shared_ptr<uint8_t>& d, std::size_t length)
        : data(d), byteLength(length)
    { }

    std::size_t GetLength() const
    {
        return byteLength;
    }

    uint8_t* GetPointer()
    {
        return data.get();
    }

    static Buffer* FromStream(IOStream& stream, std::size_t length = 0, std::size_t baseOffset = 0)
    {
        if (!length) {
            length = stream.FileSize();
        }

        if (baseOffset) {
            stream.Seek(baseOffset, aiOrigin_SET);
        }

        shared_ptr<uint8_t> data(new uint8_t[length]);


        if (stream.Read(data.get(), length, 1) != 1) {
            throw DeadlyImportError("Unable to load buffer from file!");
        }

        return new Buffer(data, length);
    }
};


Ptr<Buffer> glTFReader::LoadBuffer(const char* id, Value& obj)
{
    if (!obj.IsObject()) return Ptr<Buffer>();

    if (mExtensions.KHR_binary_glTF && strcmp(id, "KHR_binary_glTF") == 0) {
        return mBodyBuffer;
    }
    else {
        const char* uri = TryReadMember<const char*>(obj, "uri", 0);

        Buffer* b = 0;
        if (IsDataURI(uri)) {
            const char* comma = strchr(uri, ',');
            *const_cast<char*>(comma) = '\0';
            
            bool isBase64 = (strstr(uri, "base64") != 0);
            if (isBase64) {
                uint8_t* data;
                std::size_t dataLen = DecodeBase64(comma + 1, data);
                shared_ptr<uint8_t> dataptr(data);
                b = new Buffer(dataptr, dataLen);
            }
        }
        else if (uri) { // Local file
            unsigned int byteLength = TryReadMember(obj, "byteLength", 0u);

            scoped_ptr<IOStream> file(mIO.Open(uri));
            b = Buffer::FromStream(*file.get(), byteLength);
        }
        return Ptr<Buffer>(b);
    }
}


struct BufferView
{
    Ptr<Buffer> buffer;
    unsigned int byteOffset;
    unsigned int byteLength;

    BufferView() {}

    BufferView(Value& obj)
    {
        Read(obj);
    }

    void Read(Value& obj)
    {
        if (!obj.IsObject()) return;

    }

};

Ptr<BufferView> glTFReader::LoadBufferView(const char* id, Value& obj)
{
    if (!obj.IsObject()) return Ptr<BufferView>();


    const char* bufferId = TryReadMember<const char*>(obj, "buffer", 0);
    if (!bufferId) return Ptr<BufferView>();

    BufferView* bv = new BufferView();

    bv->buffer = mBuffers.Get(bufferId);
    bv->byteOffset = TryReadMember(obj, "byteOffset", 0u);
    bv->byteLength = TryReadMember(obj, "byteLength", 0u);

    return Ptr<BufferView>(bv);
}


struct Accessor
{
    Ptr<BufferView> bufferView;
    unsigned int byteOffset;
    unsigned int byteStride;
    ComponentType componentType;
    unsigned int count;
    std::string type; // "SCALAR", "VEC2", "VEC3", "VEC4", "MAT2", "MAT3", "MAT4"
    //unsigned int max;
    ///unsigned int min;

    unsigned int numComponents;
    unsigned int bytesPerComponent;
    unsigned int elemSize;

    uint8_t* data;

    inline uint8_t* GetPointer()
    {
        if (!bufferView || !bufferView->buffer) return 0;

        std::size_t offset = byteOffset + bufferView->byteOffset;
        return bufferView->buffer->GetPointer() + offset;
    }

    template<class T>
    void ExtractData(T*& outData, unsigned int* outCount = 0, unsigned int* outComponents = 0)
    {
        ai_assert(data);

        const std::size_t totalSize = elemSize * count;

        const std::size_t targetElemSize = sizeof(T);
        ai_assert(elemSize <= targetElemSize);

        ai_assert(count*byteStride <= bufferView->byteLength);

        outData = new T[count];
        if (byteStride == elemSize && targetElemSize == elemSize) {
            memcpy(outData, data, totalSize);
        }
        else {
            for (std::size_t i = 0; i < count; ++i) {
                memcpy(outData + i, data + i*byteStride, elemSize);
            }
        }

        if (outCount) *outCount = count;
        if (outComponents) *outComponents = numComponents;
    }

    //! Gets the i-th value as defined by the accessor
    template<class T>
    T GetValue(int i)
    {
        ai_assert(data);
        ai_assert(i*byteStride < bufferView->byteLength);
        T value = T();
        memcpy(&value, data + i*byteStride, elemSize);
        //value >>= 8 * (sizeof(T) - elemSize);
        return value;
    }

    //! Gets the i-th value as defined by the accessor
    unsigned int GetUInt(int i)
    {
        return GetValue<unsigned int>(i);
    }
};

Ptr<Accessor> glTFReader::LoadAccessor(const char* id, Value& obj)
{
    if (!obj.IsObject()) return Ptr<Accessor>();

    Accessor* a = new Accessor();

    const char* bufferViewId = TryReadMember<const char*>(obj, "bufferView", 0);
    if (bufferViewId) {
        a->bufferView = mBufferViews.Get(bufferViewId);
    }

    int compType = TryReadMember(obj, "componentType", unsigned(ComponentType_BYTE));

    a->byteOffset = TryReadMember(obj, "byteOffset", 0u);
    a->byteStride = TryReadMember(obj, "byteStride", 0u);
    a->componentType = static_cast<ComponentType>(compType);
    a->count = TryReadMember(obj, "count", 0u);
    a->type = TryReadMember(obj, "type", "");

    a->numComponents = 1; // "SCALAR"
    if (a->type == "VEC2") a->numComponents = 2;
    else if (a->type == "VEC3") a->numComponents = 3;
    else if (a->type == "VEC4") a->numComponents = 4;
    else if (a->type == "MAT2") a->numComponents = 4;
    else if (a->type == "MAT3") a->numComponents = 9;
    else if (a->type == "MAT4") a->numComponents = 16;

    switch (a->componentType) {
        case ComponentType_SHORT:
        case ComponentType_UNSIGNED_SHORT:
            a->bytesPerComponent = 2;
            break;

        case ComponentType_FLOAT:
            a->bytesPerComponent = 4;
            break;

        //case Accessor::ComponentType_BYTE:
        //case Accessor::ComponentType_UNSIGNED_BYTE:
        default:
            a->bytesPerComponent = 1;
    }

    a->elemSize = a->numComponents * a->bytesPerComponent;
    if (!a->byteStride) a->byteStride = a->elemSize;

    a->data = a->GetPointer();

    return Ptr<Accessor>(a);
}

static inline void setFace(aiFace& face, int a)
{
    face.mNumIndices = 1;
    face.mIndices = new unsigned int[1];
    face.mIndices[0] = a;
}

static inline void setFace(aiFace& face, int a, int b)
{
    face.mNumIndices = 2;
    face.mIndices = new unsigned int[2];
    face.mIndices[0] = a;
    face.mIndices[1] = b;
}

static inline void setFace(aiFace& face, int a, int b, int c)
{
    face.mNumIndices = 3;
    face.mIndices = new unsigned int[3];
    face.mIndices[0] = a;
    face.mIndices[1] = b;
    face.mIndices[2] = c;
}

Range glTFReader::LoadMesh(const char* id, Value& mesh)
{  
    Range range;
    range.first = mImpMeshes.size();
    range.second = mImpMeshes.size();

    MemIt primitives = mesh.FindMember("primitives");
    if (primitives != mesh.MemberEnd() && primitives->value.IsArray()) {
        for (unsigned int i = 0; i < primitives->value.Size(); ++i) {
            Value& primitive = primitives->value[i];

            aiMesh* aimesh = new aiMesh();
            mImpMeshes.push_back(aimesh);
            ++range.second;

            MemIt mode = primitive.FindMember("mode");
            if (mode != primitive.MemberEnd() && mode->value.IsInt()) {
                switch (mode->value.GetInt()) {
                    case PrimitiveMode_POINTS:
                        aimesh->mPrimitiveTypes |= aiPrimitiveType_POINT;
                        break;
                            
                    case PrimitiveMode_LINES:
                    case PrimitiveMode_LINE_LOOP:
                    case PrimitiveMode_LINE_STRIP:
                        aimesh->mPrimitiveTypes |= aiPrimitiveType_LINE;
                        break;

                    case PrimitiveMode_TRIANGLES:
                    case PrimitiveMode_TRIANGLE_STRIP:
                    case PrimitiveMode_TRIANGLE_FAN:
                        aimesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
                        break;
                }
            }

            MemIt attrs = primitive.FindMember("attributes");
            if (attrs != primitive.MemberEnd() && attrs->value.IsObject()) {
                for (MemIt it = attrs->value.MemberBegin(); it != attrs->value.MemberEnd(); ++it) {
                    if (!it->value.IsString()) continue;
                    const char* attr = it->name.GetString();
                    const char* accessorId = it->value.GetString();

                    Ptr<Accessor> accessor = mAccessors.Get(accessorId);
                    if (!accessor) continue;

                    if (strcmp(attr, "POSITION") == 0) {
                        accessor->ExtractData(aimesh->mVertices, &aimesh->mNumVertices);
                    }
                    else if (strcmp(attr, "NORMAL") == 0) {
                        accessor->ExtractData(aimesh->mNormals);
                    }
                   else if (strncmp(attr, "TEXCOORD_", 9) == 0) {
                        int idx = attr[9] - '0';
                        if (idx >= 0 && idx <= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
                            accessor->ExtractData(aimesh->mTextureCoords[idx], 0, &aimesh->mNumUVComponents[idx]);
                        }
                    }
                }
            }

            MemIt indices = primitive.FindMember("indices");
            if (indices != primitive.MemberEnd() && indices->value.IsString()) {
                Ptr<Accessor> acc = mAccessors.Get(indices->value.GetString());
                if (acc) {
                    aiFace* faces = 0;
                    std::size_t nFaces = 0;

                    int primitiveMode = mode->value.GetInt();
                    switch (primitiveMode) {
                        case PrimitiveMode_POINTS: {
                            nFaces = acc->count;
                            faces = new aiFace[nFaces];
                            for (unsigned int i = 0; i < acc->count; ++i) {
                                setFace(faces[i], acc->GetUInt(i));
                            }
                            break;
                        }

                        case PrimitiveMode_LINES: {
                            nFaces = acc->count / 2;
                            faces = new aiFace[nFaces];
                            for (unsigned int i = 0; i < acc->count; i += 2) {
                                setFace(faces[i / 2], acc->GetUInt(i), acc->GetUInt(i + 1));
                            }
                            break;
                        }

                        case PrimitiveMode_LINE_LOOP:
                        case PrimitiveMode_LINE_STRIP: {
                            nFaces = acc->count - ((primitiveMode == PrimitiveMode_LINE_STRIP) ? 1 : 0);
                            faces = new aiFace[nFaces];
                            setFace(faces[0], acc->GetUInt(0), acc->GetUInt(1));
                            for (unsigned int i = 2; i < acc->count; ++i) {
                                setFace(faces[i - 1], faces[i - 2].mIndices[1], acc->GetUInt(i));
                            }
                            if (primitiveMode == PrimitiveMode_LINE_LOOP) { // close the loop
                                setFace(faces[acc->count - 1], faces[acc->count - 2].mIndices[1], faces[0].mIndices[0]);
                            }
                            break;
                        }

                        case PrimitiveMode_TRIANGLES: {
                            nFaces = acc->count / 3;
                            faces = new aiFace[nFaces];
                            for (unsigned int i = 0; i < acc->count; i += 3) {
                                setFace(faces[i / 3], acc->GetUInt(i), acc->GetUInt(i + 1), acc->GetUInt(i + 2));
                            }
                            break;
                        }
                        case PrimitiveMode_TRIANGLE_STRIP: {
                            nFaces = acc->count - 2;
                            faces = new aiFace[nFaces];
                            setFace(faces[0], acc->GetUInt(0), acc->GetUInt(1), acc->GetUInt(2));
                            for (unsigned int i = 3; i < acc->count; ++i) {
                                setFace(faces[i - 2], faces[i - 1].mIndices[1], faces[i - 1].mIndices[2], acc->GetUInt(i));
                            }
                            break;
                        }
                        case PrimitiveMode_TRIANGLE_FAN:
                            nFaces = acc->count - 2;
                            faces = new aiFace[nFaces];
                            setFace(faces[0], acc->GetUInt(0), acc->GetUInt(1), acc->GetUInt(2));
                            for (unsigned int i = 3; i < acc->count; ++i) {
                                setFace(faces[i - 2], faces[0].mIndices[0], faces[i - 1].mIndices[2], acc->GetUInt(i));
                            }
                            break;
                    }

                    if (faces) {
                        aimesh->mFaces = faces;
                        aimesh->mNumFaces = nFaces;
                    }
                }
            }


            MemIt material = primitive.FindMember("material");
            if (material != primitive.MemberEnd() && material->value.IsString()) {
                aimesh->mMaterialIndex = mMaterials.Get(material->value.GetString());
            }
        }
    }

    return range;
}


struct Image
{
    aiString uri;
};

struct Texture
{
    Ptr<Image> source;
};

Ptr<Image> glTFReader::LoadImage(const char* id, Value& obj)
{

    Image* img = new Image();

    std::size_t embeddedDataLen = 0;
    uint8_t* embeddedData = 0;
    const char* mimeType = 0;

    // Check for extensions first (to detect binary embedded data) 
    MemIt extensions = obj.FindMember("extensions");
    if (extensions != obj.MemberEnd()) {
        Value& exts = extensions->value;

        MemIt KHR_binary_glTF = exts.FindMember("KHR_binary_glTF");
        if (KHR_binary_glTF != exts.MemberEnd() && KHR_binary_glTF->value.IsObject()) {

            int width  = TryReadMember(KHR_binary_glTF->value, "width", 0);
            int height = TryReadMember(KHR_binary_glTF->value, "height", 0);
            
            ReadMember(KHR_binary_glTF->value, "mimeType", mimeType);

            const char* bufferViewId;
            if (ReadMember(KHR_binary_glTF->value, "bufferView", bufferViewId)) {
                Ptr<BufferView> bv = mBufferViews.Get(bufferViewId);
                if (bv) {
                    embeddedDataLen = bv->byteLength;
                    embeddedData = new uint8_t[embeddedDataLen];
                    memcpy(embeddedData, bv->buffer->GetPointer() + bv->byteOffset, embeddedDataLen);
                }
            }
        }
    }
    
    if (!embeddedDataLen) {
        const char* uri;
        if (ReadMember(obj, "uri", uri)) {
            if (IsDataURI(uri)) {
                const char* comma = strchr(uri, ',');
                *const_cast<char*>(comma) = '\0';

                bool isBase64 = (strstr(uri, "base64") != 0);
                if (isBase64) {
                    embeddedDataLen = DecodeBase64(comma + 1, embeddedData);
                }

                const char* sc = strchr(uri, ';');
                if (sc != 0) {
                    *const_cast<char*>(sc) = '\0';
                    mimeType = uri;
                }
            }
            else {
                img->uri = uri;
            }
        }
    }

    // Add the embedded texture
    if (embeddedDataLen > 0) {
        aiTexture* tex = new aiTexture();
        mImpTextures.push_back(tex);

        tex->mWidth = static_cast<unsigned int>(embeddedDataLen);
        tex->mHeight = 0;
        tex->pcData = reinterpret_cast<aiTexel*>(embeddedData);

        if (mimeType) {
            const char* ext = strchr(mimeType, '/') + 1;
            if (ext) {
                if (strcmp(ext, "jpeg") == 0) ext = "jpg";
            
                std::size_t len = strlen(ext);
                if (len <= 3) {
                    strcpy(tex->achFormatHint, ext);
                }
            }
        }

        // setup texture reference string (copied from ColladaLoader::FindFilenameForEffectTexture)
        img->uri.data[0] = '*';
        img->uri.length = 1 + ASSIMP_itoa10(img->uri.data + 1, MAXLEN - 1, mImpTextures.size() - 1);
    }

    return Ptr<Image>(img);
}

Ptr<Texture> glTFReader::LoadTexture(const char* id, Value& obj)
{
    Texture* tex = new Texture();

    const char* source;
    if (ReadMember(obj, "source", source)) {
        tex->source = mImages.Get(source);
    }

    return Ptr<Texture>(tex);
}

void glTFReader::SetMaterialColorProperty(aiMaterial* mat, Value& vals, const char* propName, aiTextureType texType, const char* pKey, unsigned int type, unsigned int idx)
{
    MemIt prop = vals.FindMember(propName);
    if (prop != vals.MemberEnd()) {
        aiColor3D col;
        if (Read(prop->value, col)) {
            mat->AddProperty(&col, 1, pKey, type, idx);
        }
        else if (prop->value.IsString()) {
            Ptr<Texture> tex = mTextures.Get(prop->value.GetString());
            if (tex && tex->source) {
                mat->AddProperty(&tex->source->uri, _AI_MATKEY_TEXTURE_BASE, texType, 0);
            }
        }
    }
}

unsigned int glTFReader::LoadMaterial(const char* id, Value& material)
{
    aiMaterial* mat = new aiMaterial();
    mImpMaterials.push_back(mat);

    const char* name;
    if (ReadMember(material, "name", name)) {
        aiString str(name);
        mat->AddProperty(&str, AI_MATKEY_NAME);
    }

    MemIt values = material.FindMember("values");
    if (values != material.MemberEnd() && values->value.IsObject()) {
        Value& vals = values->value;

        SetMaterialColorProperty(mat, vals, "diffuse",  aiTextureType_DIFFUSE,  AI_MATKEY_COLOR_DIFFUSE);
        SetMaterialColorProperty(mat, vals, "specular", aiTextureType_SPECULAR, AI_MATKEY_COLOR_SPECULAR);
        SetMaterialColorProperty(mat, vals, "ambient",  aiTextureType_AMBIENT,  AI_MATKEY_COLOR_AMBIENT);

        float shininess;
        if (ReadMember(vals, "shininess", shininess)) {
            mat->AddProperty(&shininess, 1, AI_MATKEY_SHININESS);
        }
    }

    MemIt extensions = material.FindMember("values");
    if (extensions != material.MemberEnd() && extensions->value.IsObject()) {
        Value& exts = extensions->value;

        MemIt KHR_materials_common = exts.FindMember("KHR_materials_common");
        if (KHR_materials_common != exts.MemberEnd() && KHR_materials_common->value.IsObject()) {
            // TODO: support KHR_materials_common (https://github.com/KhronosGroup/glTF/tree/master/extensions/Khronos/KHR_materials_common)
        }
    }

    return static_cast<unsigned int>(mImpMaterials.size() - 1);
}

aiNode* glTFReader::LoadNode(const char* id, Value& node)
{
    aiNode* ainode = new aiNode(id);

    //MemIt name = node.FindMember("name");
    //if (name != node.MemberEnd() && name->value.IsString()) {
    //    strcpy(ainode->mName.data, name->value.GetString());
    //}

    MemIt children = node.FindMember("children");
    if (children != node.MemberEnd() && children->value.IsArray()) {

        ainode->mChildren = new aiNode*[children->value.Size()];
        //ainode->mNumChildren = 0;

        for (unsigned int i = 0; i < children->value.Size(); ++i) {
            Value& child = children->value[i];
            if (child.IsString()) {
                // get/create the child node
                aiNode* aichild = mNodes.Get(child.GetString());
                if (aichild) {
                    aichild->mParent = ainode;
                    ainode->mChildren[ainode->mNumChildren++] = aichild;
                }
            }
        }
    }

    aiMatrix4x4& transf = ainode->mTransformation;

    MemIt matrix = node.FindMember("matrix");
    if (matrix != node.MemberEnd()) {
        Read(matrix->value, transf);
    }
    else {
        MemIt translation = node.FindMember("translation");
        if (translation != node.MemberEnd()) {
            aiVector3D trans;
            if (Read(matrix->value, trans)) {
                aiMatrix4x4 m;
                aiMatrix4x4::Translation(trans, m);
                transf = m * transf;
            }
        }

        MemIt scale = node.FindMember("scale");
        if (scale != node.MemberEnd()) {
            aiVector3D scal(1.f);
            if (Read(matrix->value, scal)) {
                aiMatrix4x4 m;
                aiMatrix4x4::Scaling(scal, m);
                transf = m * transf;
            }
        }

        MemIt rotation = node.FindMember("rotation");
        if (rotation != node.MemberEnd()) {
            aiQuaternion rot;
            if (Read(matrix->value, rot)) {
                transf = aiMatrix4x4(rot.GetMatrix()) * transf;
            }
        }
    }

    MemIt meshes = node.FindMember("meshes");
    if (meshes != node.MemberEnd() && meshes->value.IsArray()) {
        std::size_t numMeshes = (std::size_t)meshes->value.Size();

        std::vector<unsigned int> meshList;

        for (std::size_t i = 0; i < numMeshes; ++i) {
            if (meshes->value[i].IsString()) {
                Range range = mMeshes.Get(meshes->value[i].GetString());
                for (unsigned int m = range.first; m < range.second; ++m) {
                    meshList.push_back(m);
                }                
            }
        }

        if (meshList.size()) {
            ainode->mNumMeshes = meshList.size();
            ainode->mMeshes = new unsigned int[meshList.size()];
            std::swap_ranges(meshList.begin(), meshList.end(), ainode->mMeshes);
        }
    }

    // TODO load "skeletons", "skin", "jointName", "camera"

    return ainode;
}





//
// glTFImporter methods
//

template<> const std::string LogFunctions<glTFImporter>::log_prefix = "glTF: ";



static const aiImporterDesc desc = {
    "glTF Importer",
    "",
    "",
    "",
    aiImporterFlags_SupportTextFlavour | aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_SupportCompressedFlavour
    | aiImporterFlags_LimitedSupport | aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    "gltf glb"
};

glTFImporter::glTFImporter() 
: BaseImporter()
{

}

glTFImporter::~glTFImporter() {

}

bool glTFImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig ) const {
    const std::string& extension = GetExtension(pFile);
    if (extension == "gltf" || extension == "glb") {
        return true;
    }
    return false;
}

const aiImporterDesc* glTFImporter::GetInfo() const {
    return &desc;
}

void glTFImporter::ReadBinaryHeader(IOStream& stream)
{
    GLB_Header header;
    if (stream.Read(&header, sizeof(header), 1) != 1) {
        ThrowException("Unable to read the file header");
    }

    if (strncmp((char*)header.magic, AI_GLB_MAGIC_NUMBER, sizeof(header.magic)) != 0) {
        ThrowException("Invalid binary glTF file");
    }

    AI_SWAP4(header.version);
    if (header.version != 1) {
        ThrowException("Unsupported binary glTF version");
    }

    AI_SWAP4(header.sceneFormat);
    if (header.sceneFormat != SceneFormat_JSON) {
        ThrowException("Unsupported binary glTF scene format");
    }

    AI_SWAP4(header.length);
    AI_SWAP4(header.sceneLength);

    mSceneLength = static_cast<std::size_t>(header.sceneLength);
        
    mBodyOffset = sizeof(header) + mSceneLength;
    mBodyOffset = (mBodyOffset + 3) & ~3; // Round up to next multiple of 4

    mBodyLength = header.length - mBodyOffset;
}

void glTFImporter::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler ) {
    scoped_ptr<IOStream> stream(pIOHandler->Open(pFile, "rb"));
    if (!stream) {
        ThrowException("Could not open file for reading");
    }

    // is binary? then read the header
    if (GetExtension(pFile) == "glb") {
        ReadBinaryHeader(*stream);
    }
    else {
        mSceneLength = stream->FileSize();
        mBodyLength = 0;
    }


    // read the scene data

    scoped_ptr<char> sceneData = new char[mSceneLength + 1];
    sceneData[mSceneLength] = '\0';

    if (stream->Read(sceneData, 1, mSceneLength) != mSceneLength) {
        ThrowException("Could not read the file contents");
    }


    // parse the JSON document

    Document doc;
    doc.ParseInsitu(sceneData);

    if (doc.HasParseError()) {
        char buffer[32];
        ASSIMP_itoa10(buffer, doc.GetErrorOffset());
        ThrowException(std::string("JSON parse error, offset ") + buffer + ": "
            + GetParseError_En(doc.GetParseError()));
    }

    if (!doc.IsObject()) {
        ThrowException("gltf file must be a JSON object!");
    }


    // Buffer instance for the current file embedded contents
    shared_ptr<Buffer> bodyBuffer;
    if (mBodyLength > 0) {
        bodyBuffer.reset(Buffer::FromStream(*stream, mBodyLength, mBodyOffset));
    }

    // import the data
    glTFReader reader(pScene, doc, *pIOHandler, bodyBuffer);
    reader.Load();

    if (pScene->mNumMeshes == 0) {
        pScene->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;
    }
}

#endif // ASSIMP_BUILD_NO_GLTF_IMPORTER

