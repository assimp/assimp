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

/** @file glTFAsset.h
 * Declares a glTF class to handle gltf/glb files
 *
 * glTF Extensions Support:
 *   KHR_binary_glTF: full
 *   KHR_materials_common: full
 */
#ifndef glTFAsset_H_INC
#define glTFAsset_H_INC

#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#ifdef ASSIMP_API
#   include <memory>
#   include "DefaultIOSystem.h"
#   include "ByteSwapper.h"
#else
#   include <memory>
#   define AI_SWAP4(p)
#   define ai_assert
#endif


#if _MSC_VER > 1500 || (defined __GNUC___)
#       define ASSIMP_GLTF_USE_UNORDERED_MULTIMAP
#   else
#       define gltf_unordered_map map
#endif

#ifdef ASSIMP_GLTF_USE_UNORDERED_MULTIMAP
#   include <unordered_map>
#   if _MSC_VER > 1600
#       define gltf_unordered_map unordered_map
#   else
#       define gltf_unordered_map tr1::unordered_map
#   endif
#endif

namespace glTF
{
#ifdef ASSIMP_API
    using Assimp::IOStream;
    using Assimp::IOSystem;
    using std::shared_ptr;
#else
    using std::shared_ptr;

    typedef std::runtime_error DeadlyImportError;
    typedef std::runtime_error DeadlyExportError;

    enum aiOrigin { aiOrigin_SET = 0, aiOrigin_CUR = 1, aiOrigin_END = 2 };
    class IOSystem;
    class IOStream
    {
        FILE* f;
    public:
        IOStream(FILE* file) : f(file) {}
        ~IOStream() { fclose(f); f = 0; }

        size_t Read(void* b, size_t sz, size_t n) { return fread(b, sz, n, f); }
        size_t Write(const void* b, size_t sz, size_t n) { return fwrite(b, sz, n, f); }
        int    Seek(size_t off, aiOrigin orig) { return fseek(f, off, int(orig)); }
        size_t Tell() const { return ftell(f); }

        size_t FileSize() {
            long p = Tell(), len = (Seek(0, aiOrigin_END), Tell());
            return size_t((Seek(p, aiOrigin_SET), len));
        }
    };
#endif

    using rapidjson::Value;
    using rapidjson::Document;

    class Asset;
    class AssetWriter;

    struct BufferView; // here due to cross-reference
    struct Texture;
    struct Light;


    // Vec/matrix types, as raw float arrays
    typedef float (vec3)[3];
    typedef float (vec4)[4];
    typedef float (mat4)[16];


    namespace Util
    {
        void EncodeBase64(const uint8_t* in, size_t inLength, std::string& out);

        size_t DecodeBase64(const char* in, size_t inLength, uint8_t*& out);

        inline size_t DecodeBase64(const char* in, uint8_t*& out)
        {
            return DecodeBase64(in, strlen(in), out);
        }

        struct DataURI
        {
            const char* mediaType;
            const char* charset;
            bool base64;
            const char* data;
            size_t dataLength;
        };

        //! Check if a uri is a data URI
        inline bool ParseDataURI(const char* uri, size_t uriLen, DataURI& out);
    }


    //! Magic number for GLB files
    #define AI_GLB_MAGIC_NUMBER "glTF"

    #ifdef ASSIMP_API
        #include "./../include/assimp/Compiler/pushpack1.h"
    #endif

    //! For the KHR_binary_glTF extension (binary .glb file)
    //! 20-byte header (+ the JSON + a "body" data section)
    struct GLB_Header
    {
        uint8_t magic[4];     //!< Magic number: "glTF"
        uint32_t version;     //!< Version number (always 1 as of the last update)
        uint32_t length;      //!< Total length of the Binary glTF, including header, scene, and body, in bytes
        uint32_t sceneLength; //!< Length, in bytes, of the glTF scene
        uint32_t sceneFormat; //!< Specifies the format of the glTF scene (see the SceneFormat enum)
    } PACK_STRUCT;

    #ifdef ASSIMP_API
        #include "./../include/assimp/Compiler/poppack1.h"
    #endif


    //! Values for the GLB_Header::sceneFormat field
    enum SceneFormat
    {
        SceneFormat_JSON = 0
    };

    //! Values for the mesh primitive modes
    enum PrimitiveMode
    {
        PrimitiveMode_POINTS = 0,
        PrimitiveMode_LINES = 1,
        PrimitiveMode_LINE_LOOP = 2,
        PrimitiveMode_LINE_STRIP = 3,
        PrimitiveMode_TRIANGLES = 4,
        PrimitiveMode_TRIANGLE_STRIP = 5,
        PrimitiveMode_TRIANGLE_FAN = 6
    };

    //! Values for the Accessor::componentType field
    enum ComponentType
    {
        ComponentType_BYTE = 5120,
        ComponentType_UNSIGNED_BYTE = 5121,
        ComponentType_SHORT = 5122,
        ComponentType_UNSIGNED_SHORT = 5123,
        ComponentType_FLOAT = 5126
    };

    inline size_t ComponentTypeSize(ComponentType t)
    {
        switch (t) {
            case ComponentType_SHORT:
            case ComponentType_UNSIGNED_SHORT:
                return 2;

            case ComponentType_FLOAT:
                return 4;

            //case Accessor::ComponentType_BYTE:
            //case Accessor::ComponentType_UNSIGNED_BYTE:
            default:
                return 1;
        }
    }

    //! Values for the BufferView::target field
    enum BufferViewTarget
    {
        BufferViewTarget_ARRAY_BUFFER = 34962,
        BufferViewTarget_ELEMENT_ARRAY_BUFFER = 34963
    };

    //! Values for the Texture::format and Texture::internalFormat fields
    enum TextureFormat
    {
        TextureFormat_ALPHA = 6406,
        TextureFormat_RGB = 6407,
        TextureFormat_RGBA = 6408,
        TextureFormat_LUMINANCE = 6409,
        TextureFormat_LUMINANCE_ALPHA = 6410
    };

    //! Values for the Texture::target field
    enum TextureTarget
    {
        TextureTarget_TEXTURE_2D = 3553
    };

    //! Values for the Texture::type field
    enum TextureType
    {
        TextureType_UNSIGNED_BYTE = 5121,
        TextureType_UNSIGNED_SHORT_5_6_5 = 33635,
        TextureType_UNSIGNED_SHORT_4_4_4_4 = 32819,
        TextureType_UNSIGNED_SHORT_5_5_5_1 = 32820
    };


    //! Values for the Accessor::type field (helper class)
    class AttribType
    {
    public:
        enum Value
            { SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4 };

    private:
        static const size_t NUM_VALUES = static_cast<size_t>(MAT4)+1;

        struct Info
            { const char* name; unsigned int numComponents; };

        template<int N> struct data
            { static const Info infos[NUM_VALUES]; };

    public:
        inline static Value FromString(const char* str)
        {
            for (size_t i = 0; i < NUM_VALUES; ++i) {
                if (strcmp(data<0>::infos[i].name, str) == 0) {
                    return static_cast<Value>(i);
                }
            }
            return SCALAR;
        }

        inline static const char* ToString(Value type)
        {
            return data<0>::infos[static_cast<size_t>(type)].name;
        }

        inline static unsigned int GetNumComponents(Value type)
        {
            return data<0>::infos[static_cast<size_t>(type)].numComponents;
        }
    };

    // must match the order of the AttribTypeTraits::Value enum!
    template<int N> const AttribType::Info
    AttribType::data<N>::infos[AttribType::NUM_VALUES] = {
        { "SCALAR", 1 }, { "VEC2", 2 }, { "VEC3", 3 }, { "VEC4", 4 }, { "MAT2", 4 }, { "MAT3", 9 }, { "MAT4", 16 }
    };



    //! A reference to one top-level object, which is valid
    //! until the Asset instance is destroyed
    template<class T>
    class Ref
    {
        std::vector<T*>* vector;
        int index;

    public:
        Ref() : vector(0), index(0) {}
        Ref(std::vector<T*>& vec, int idx) : vector(&vec), index(idx) {}

        inline size_t GetIndex() const
            { return index; }

        operator bool() const
            { return vector != 0; }

        T* operator->()
            { return (*vector)[index]; }

        T& operator*()
            { return *((*vector)[index]); }
    };

    //! Helper struct to represent values that might not be present
    template<class T>
    struct Nullable
    {
        T value;
        bool isPresent;

        Nullable() : isPresent(false) {}
        Nullable(T& val) : value(val), isPresent(true) {}
    };


    //! Base classe for all glTF top-level objects
    struct Object
    {
        std::string id;   //!< The globally unique ID used to reference this object
        std::string name; //!< The user-defined name of this object

        //! Objects marked as special are not exported (used to emulate the binary body buffer)
        virtual bool IsSpecial() const
            { return false; }

        virtual ~Object() {}
    };



    //
    // Classes for each glTF top-level object type
    //

    //! A typed view into a BufferView. A BufferView contains raw binary data.
    //! An accessor provides a typed view into a BufferView or a subset of a BufferView
    // !similar to how WebGL's vertexAttribPointer() defines an attribute in a buffer.
    struct Accessor : public Object
    {
        Ref<BufferView> bufferView;  //!< The ID of the bufferView. (required)
        unsigned int byteOffset;     //!< The offset relative to the start of the bufferView in bytes. (required)
        unsigned int byteStride;     //!< The stride, in bytes, between attributes referenced by this accessor. (default: 0)
        ComponentType componentType; //!< The datatype of components in the attribute. (required)
        unsigned int count;          //!< The number of attributes referenced by this accessor. (required)
        AttribType::Value type;      //!< Specifies if the attribute is a scalar, vector, or matrix. (required)
        //std::vector<float> max;    //!< Maximum value of each component in this attribute.
        //std::vector<float> min;    //!< Minimum value of each component in this attribute.

        unsigned int GetNumComponents();
        unsigned int GetBytesPerComponent();
        unsigned int GetElementSize();

        inline uint8_t* GetPointer();

        template<class T>
        void ExtractData(T*& outData);

        void WriteData(size_t count, const void* src_buffer, size_t src_stride);

        //! Helper class to iterate the data
        class Indexer
        {
            friend struct Accessor;

            Accessor& accessor;
            uint8_t* data;
            size_t elemSize, stride;

            Indexer(Accessor& acc);

        public:

            //! Accesses the i-th value as defined by the accessor
            template<class T>
            T GetValue(int i);

            //! Accesses the i-th value as defined by the accessor
            inline unsigned int GetUInt(int i)
            {
                return GetValue<unsigned int>(i);
            }
        };

        inline Indexer GetIndexer()
        {
            return Indexer(*this);
        }

        Accessor() {}
        void Read(Value& obj, Asset& r);
    };


    struct Animation : public Object
    {
        struct Channel
        {

        };

        struct Target
        {

        };

        struct Sampler
        {

        };
    };

    //! A buffer points to binary geometry, animation, or skins.
    struct Buffer : public Object
    {
    public:

        enum Type
        {
            Type_arraybuffer,
            Type_text
        };

        //std::string uri; //!< The uri of the buffer. Can be a filepath, a data uri, etc. (required)
        size_t byteLength; //!< The length of the buffer in bytes. (default: 0)
        //std::string type; //!< XMLHttpRequest responseType (default: "arraybuffer")

        Type type;

    private:
        shared_ptr<uint8_t> mData; //!< Pointer to the data
        bool mIsSpecial; //!< Set to true for special cases (e.g. the body buffer)

    public:
        Buffer();

        void Read(Value& obj, Asset& r);

        void LoadFromStream(IOStream& stream, size_t length = 0, size_t baseOffset = 0);

        size_t AppendData(uint8_t* data, size_t length);
        void Grow(size_t amount);

        uint8_t* GetPointer()
            { return mData.get(); }

        void MarkAsSpecial()
            { mIsSpecial = true; }

        bool IsSpecial() const
            { return mIsSpecial; }
    };


    //! A view into a buffer generally representing a subset of the buffer.
    struct BufferView : public Object
    {
        Ref<Buffer> buffer; //! The ID of the buffer. (required)
        size_t byteOffset; //! The offset into the buffer in bytes. (required)
        size_t byteLength; //! The length of the bufferView in bytes. (default: 0)

        BufferViewTarget target; //! The target that the WebGL buffer should be bound to.

        BufferView() {}
        void Read(Value& obj, Asset& r);
    };


    struct Camera : public Object
    {
        enum Type
        {
            Perspective,
            Orthographic
        };

        Type type;

        union
        {
            struct {
                float aspectRatio; //!<The floating - point aspect ratio of the field of view. (0 = undefined = use the canvas one)
                float yfov;  //!<The floating - point vertical field of view in radians. (required)
                float zfar;  //!<The floating - point distance to the far clipping plane. (required)
                float znear; //!< The floating - point distance to the near clipping plane. (required)
            } perspective;

            struct {
                float xmag;  //! The floating-point horizontal magnification of the view. (required)
                float ymag;  //! The floating-point vertical magnification of the view. (required)
                float zfar;  //! The floating-point distance to the far clipping plane. (required)
                float znear; //! The floating-point distance to the near clipping plane. (required)
            } ortographic;
        };

        Camera() {}
        void Read(Value& obj, Asset& r);
    };


    //! Image data used to create a texture.
    struct Image : public Object
    {
        std::string uri; //! The uri of the image, that can be a file path, a data URI, etc.. (required)

        Ref<BufferView> bufferView;

        std::string mimeType;

        int width, height;

    private:
        uint8_t* mData;
        size_t mDataLength;

    public:

        Image();
        void Read(Value& obj, Asset& r);

        inline bool HasData() const
            { return mDataLength > 0; }

        inline size_t GetDataLength() const
            { return mDataLength; }

        inline const uint8_t* GetData() const
            { return mData; }

        inline uint8_t* StealData();

        inline void SetData(uint8_t* data, size_t length, Asset& r);
    };

    //! Holds a material property that can be a texture or a color
    struct TexProperty
    {
        Ref<Texture> texture;
        vec4 color;
    };

    //! The material appearance of a primitive.
    struct Material : public Object
    {
        //Ref<Sampler> source; //!< The ID of the technique.
        //std::gltf_unordered_map<std::string, std::string> values; //!< A dictionary object of parameter values.

        //! Techniques defined by KHR_materials_common
        enum Technique
        {
            Technique_undefined = 0,
            Technique_BLINN,
            Technique_PHONG,
            Technique_LAMBERT,
            Technique_CONSTANT
        };

        TexProperty ambient;
        TexProperty diffuse;
        TexProperty specular;
        TexProperty emission;

        bool doubleSided;
        bool transparent;
        float transparency;
        float shininess;

        Technique technique;

        Material() { SetDefaults(); }
        void Read(Value& obj, Asset& r);
        void SetDefaults();
    };

    //! A set of primitives to be rendered. A node can contain one or more meshes. A node's transform places the mesh in the scene.
    struct Mesh : public Object
    {
        typedef std::vector< Ref<Accessor> > AccessorList;

        struct Primitive
        {
            PrimitiveMode mode;

            struct Attributes {
                AccessorList position, normal, texcoord, color, joint, jointmatrix, weight;
            } attributes;

            Ref<Accessor> indices;

            Ref<Material> material;
        };

        std::vector<Primitive> primitives;

        Mesh() {}
        void Read(Value& obj, Asset& r);
    };

    struct Node : public Object
    {
        std::vector< Ref<Node> > children;
        std::vector< Ref<Mesh> > meshes;

        Nullable<mat4> matrix;
        Nullable<vec3> translation;
        Nullable<vec4> rotation;
        Nullable<vec3> scale;

        Ref<Camera> camera;
        Ref<Light>  light;

        Node() {}
        void Read(Value& obj, Asset& r);
    };

    struct Program : public Object
    {
        Program() {}
        void Read(Value& obj, Asset& r);
    };


    struct Sampler : public Object
    {
        Sampler() {}
        void Read(Value& obj, Asset& r);
    };

    struct Scene : public Object
    {
        std::vector< Ref<Node> > nodes;

        Scene() {}
        void Read(Value& obj, Asset& r);
    };

    struct Shader : public Object
    {
        Shader() {}
        void Read(Value& obj, Asset& r);
    };

    struct Skin : public Object
    {
        Skin() {}
        void Read(Value& obj, Asset& r);
    };

    struct Technique : public Object
    {
        struct Parameters
        {

        };

        struct States
        {

        };

        struct Functions
        {

        };

        Technique() {}
        void Read(Value& obj, Asset& r);
    };

    //! A texture and its sampler.
    struct Texture : public Object
    {
        //Ref<Sampler> source; //!< The ID of the sampler used by this texture. (required)
        Ref<Image> source; //!< The ID of the image used by this texture. (required)

        //TextureFormat format; //!< The texture's format. (default: TextureFormat_RGBA)
        //TextureFormat internalFormat; //!< The texture's internal format. (default: TextureFormat_RGBA)

        //TextureTarget target; //!< The target that the WebGL texture should be bound to. (default: TextureTarget_TEXTURE_2D)
        //TextureType type; //!< Texel datatype. (default: TextureType_UNSIGNED_BYTE)

        Texture() {}
        void Read(Value& obj, Asset& r);
    };


    //! A light (from KHR_materials_common extension)
    struct Light : public Object
    {
        enum Type
        {
            Type_undefined,
            Type_ambient,
            Type_directional,
            Type_point,
            Type_spot
        };

        Type type;

        vec4 color;
        float distance;
        float constantAttenuation;
        float linearAttenuation;
        float quadraticAttenuation;
        float falloffAngle;
        float falloffExponent;

        Light() {}
        void Read(Value& obj, Asset& r);

        void SetDefaults();
    };

    //! Base class for LazyDict that acts as an interface
    class LazyDictBase
    {
    public:
        virtual ~LazyDictBase() {}

        virtual void AttachToDocument(Document& doc) = 0;
        virtual void DetachFromDocument() = 0;

        virtual void WriteObjects(AssetWriter& writer) = 0;
    };

    //! (Stub class that is specialized in glTFAssetWriter.h)
    template<class T>
    struct LazyDictWriter
    {
        static void Write(T& d, AssetWriter& w) {}
    };

    //! Manages lazy loading of the glTF top-level objects, and keeps a reference to them by ID
    //! It is the owner the loaded objects, so when it is destroyed it also deletes them
    template<class T>
    class LazyDict : public LazyDictBase
    {
        friend class Asset;
        friend class AssetWriter;

        typedef typename std::gltf_unordered_map< std::string, size_t > Dict;

        std::vector<T*>  mObjs;      //! The read objects
        Dict             mObjsById;  //! The read objects accesible by id
        const char*      mDictId;    //! ID of the dictionary object
        const char*      mExtId;     //! ID of the extension defining the dictionary
        Value*           mDict;      //! JSON dictionary object
        Asset&           mAsset;     //! The asset instance

        void AttachToDocument(Document& doc);
        void DetachFromDocument();

        void WriteObjects(AssetWriter& writer)
            { LazyDictWriter< LazyDict >::Write(*this, writer); }

        Ref<T> Add(T* obj);

    public:
        LazyDict(Asset& asset, const char* dictId, const char* extId = 0);
        ~LazyDict();

        Ref<T> Get(const char* id);
        Ref<T> Get(size_t i);

        Ref<T> Create(const char* id);
        Ref<T> Create(const std::string& id)
            { return Create(id.c_str()); }

        inline size_t Size() const
            { return mObjs.size(); }

        inline T& operator[](size_t i)
            { return *mObjs[i]; }

    };


    struct AssetMetadata
    {
        std::string copyright; //!< A copyright message suitable for display to credit the content creator.
        std::string generator; //!< Tool that generated this glTF model.Useful for debugging.
        bool premultipliedAlpha; //!< Specifies if the shaders were generated with premultiplied alpha. (default: false)

        struct {
            std::string api;     //!< Specifies the target rendering API (default: "WebGL")
            std::string version; //!< Specifies the target rendering API (default: "1.0.3")
        } profile; //!< Specifies the target rendering API and version, e.g., WebGL 1.0.3. (default: {})

        int version; //!< The glTF format version (should be 1)

        void Read(Document& doc);
    };

    //
    // glTF Asset class
    //

    //! Root object for a glTF asset
    class Asset
    {
        typedef std::gltf_unordered_map<std::string, int> IdMap;

        template<class T>
        friend class LazyDict;

        friend struct Buffer; // To access OpenFile

        friend class AssetWriter;

    private:
        IOSystem* mIOSystem;

        std::string mCurrentAssetDir;

        size_t mSceneLength;
        size_t mBodyOffset, mBodyLength;

        std::vector<LazyDictBase*> mDicts;

        IdMap mUsedIds;

        Ref<Buffer> mBodyBuffer;

        Asset(Asset&);
        Asset& operator=(const Asset&);

    public:

        //! Keeps info about the enabled extensions
        struct Extensions
        {
            bool KHR_binary_glTF;
            bool KHR_materials_common;

        } extensionsUsed;

        AssetMetadata asset;


        // Dictionaries for each type of object

        LazyDict<Accessor>    accessors;
        LazyDict<Animation>   animations;
        LazyDict<Buffer>      buffers;
        LazyDict<BufferView>  bufferViews;
        LazyDict<Camera>      cameras;
        LazyDict<Image>       images;
        LazyDict<Material>    materials;
        LazyDict<Mesh>        meshes;
        LazyDict<Node>        nodes;
        //LazyDict<Program>   programs;
        //LazyDict<Sampler>   samplers;
        LazyDict<Scene>       scenes;
        //LazyDict<Shader>    shaders;
        //LazyDict<Skin>      skins;
        //LazyDict<Technique> techniques;
        LazyDict<Texture>     textures;

        LazyDict<Light>       lights; // KHR_materials_common ext

        Ref<Scene> scene;

    public:
        Asset(IOSystem* io = 0)
            : mIOSystem(io)
            , accessors     (*this, "accessors")
            , animations    (*this, "animations")
            , buffers       (*this, "buffers")
            , bufferViews   (*this, "bufferViews")
            , cameras       (*this, "cameras")
            , images        (*this, "images")
            , materials     (*this, "materials")
            , meshes        (*this, "meshes")
            , nodes         (*this, "nodes")
            //, programs    (*this, "programs")
            //, samplers    (*this, "samplers")
            , scenes        (*this, "scenes")
            //, shaders     (*this, "shaders")
            //, skins       (*this, "skins")
            //, techniques  (*this, "techniques")
            , textures      (*this, "textures")
            , lights        (*this, "lights", "KHR_materials_common")
        {
            memset(&extensionsUsed, 0, sizeof(extensionsUsed));
            memset(&asset, 0, sizeof(asset));
        }

        //! Main function
        void Load(const std::string& file, bool isBinary = false);

        //! Enables the "KHR_binary_glTF" extension on the asset
        void SetAsBinary();

        //! Search for an available name, starting from the given strings
        std::string FindUniqueID(const std::string& str, const char* suffix);

        Ref<Buffer> GetBodyBuffer()
            { return mBodyBuffer; }

    private:
        void ReadBinaryHeader(IOStream& stream);

        void ReadExtensionsUsed(Document& doc);


        IOStream* OpenFile(std::string path, const char* mode, bool absolute = false);
    };

}

// Include the implementation of the methods
#include "glTFAsset.inl"

#endif
