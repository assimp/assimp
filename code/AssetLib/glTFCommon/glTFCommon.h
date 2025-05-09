/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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
#ifndef AI_GLFTCOMMON_H_INC
#define AI_GLFTCOMMON_H_INC

#ifndef ASSIMP_BUILD_NO_GLTF_IMPORTER

#include <assimp/Exceptional.h>
#include <assimp/DefaultLogger.hpp>

#include <algorithm>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/rapidjson.h>

// clang-format off

#ifdef ASSIMP_API
#   include <assimp/ByteSwapper.h>
#   include <assimp/DefaultIOSystem.h>
#   include <memory>
#else
#   include <memory>
#   define AI_SWAP4(p)
#   define ai_assert
#endif

#if _MSC_VER > 1500 || (defined __GNUC___)
#   define ASSIMP_GLTF_USE_UNORDERED_MULTIMAP
#else
#   define gltf_unordered_map map
#endif

#ifdef ASSIMP_GLTF_USE_UNORDERED_MULTIMAP
#   include <unordered_map>
#   if defined(_MSC_VER) && _MSC_VER <= 1600
#       define gltf_unordered_map tr1::unordered_map
#   else
#       define gltf_unordered_map unordered_map
#   endif
#endif
// clang-format on


namespace glTFCommon {

using rapidjson::Document;
using rapidjson::Value;

#ifdef ASSIMP_API
using Assimp::IOStream;
using Assimp::IOSystem;
using std::shared_ptr;
#else
using std::shared_ptr;

typedef std::runtime_error DeadlyImportError;
typedef std::runtime_error DeadlyExportError;

enum aiOrigin {
    aiOrigin_SET = 0,
    aiOrigin_CUR = 1,
    aiOrigin_END = 2
};

class IOSystem;

class IOStream {
public:
    IOStream(FILE *file) :
            f(file) {}
    ~IOStream() {
        fclose(f);
    }

    size_t Read(void *b, size_t sz, size_t n) { return fread(b, sz, n, f); }
    size_t Write(const void *b, size_t sz, size_t n) { return fwrite(b, sz, n, f); }
    int Seek(size_t off, aiOrigin orig) { return fseek(f, off, int(orig)); }
    size_t Tell() const { return ftell(f); }

    size_t FileSize() {
        long p = Tell(), len = (Seek(0, aiOrigin_END), Tell());
        return size_t((Seek(p, aiOrigin_SET), len));
    }

private:
    FILE *f;
};
#endif

// Vec/matrix types, as raw float arrays
typedef float(vec3)[3];
typedef float(vec4)[4];
typedef float(mat4)[16];

inline void CopyValue(const glTFCommon::vec3 &v, aiColor4D &out) {
    out.r = v[0];
    out.g = v[1];
    out.b = v[2];
    out.a = 1.0;
}

inline void CopyValue(const glTFCommon::vec4 &v, aiColor4D &out) {
    out.r = v[0];
    out.g = v[1];
    out.b = v[2];
    out.a = v[3];
}

inline void CopyValue(const glTFCommon::vec4 &v, aiColor3D &out) {
    out.r = v[0];
    out.g = v[1];
    out.b = v[2];
}

inline void CopyValue(const glTFCommon::vec3 &v, aiColor3D &out) {
    out.r = v[0];
    out.g = v[1];
    out.b = v[2];
}

inline void CopyValue(const glTFCommon::vec3 &v, aiVector3D &out) {
    out.x = v[0];
    out.y = v[1];
    out.z = v[2];
}

inline void CopyValue(const glTFCommon::vec4 &v, aiQuaternion &out) {
    out.x = v[0];
    out.y = v[1];
    out.z = v[2];
    out.w = v[3];
}

inline void CopyValue(const glTFCommon::mat4 &v, aiMatrix4x4 &o) {
    o.a1 = v[0];
    o.b1 = v[1];
    o.c1 = v[2];
    o.d1 = v[3];
    o.a2 = v[4];
    o.b2 = v[5];
    o.c2 = v[6];
    o.d2 = v[7];
    o.a3 = v[8];
    o.b3 = v[9];
    o.c3 = v[10];
    o.d3 = v[11];
    o.a4 = v[12];
    o.b4 = v[13];
    o.c4 = v[14];
    o.d4 = v[15];
}

#if _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4310)
#endif // _MSC_VER

inline std::string getCurrentAssetDir(const std::string &pFile) {
    int pos = std::max(int(pFile.rfind('/')), int(pFile.rfind('\\')));
    if (pos == int(std::string::npos)) {
        return std::string();
    }

    return pFile.substr(0, pos + 1);
}
#if _MSC_VER
#    pragma warning(pop)
#endif // _MSC_VER

namespace Util {

void EncodeBase64(const uint8_t *in, size_t inLength, std::string &out);

size_t DecodeBase64(const char *in, size_t inLength, uint8_t *&out);

inline size_t DecodeBase64(const char *in, uint8_t *&out) {
    return DecodeBase64(in, strlen(in), out);
}

struct DataURI {
    const char *mediaType;
    const char *charset;
    bool base64;
    const char *data;
    size_t dataLength;
};

//! Check if a uri is a data URI
bool ParseDataURI(const char *const_uri, size_t uriLen, DataURI &out);

} // namespace Util

#define CHECK_EXT(EXT) \
    if (exts.find(#EXT) != exts.end()) extensionsUsed.EXT = true;

//! Helper struct to represent values that might not be present
template <class T>
struct Nullable {
    T value;
    bool isPresent;

    Nullable() :
            isPresent(false) {}
    Nullable(T &val) :
            value(val),
            isPresent(true) {}
};

//! A reference to one top-level object, which is valid
//! until the Asset instance is destroyed
template <class T>
class Ref {
    std::vector<T *> *vector;
    unsigned int index;

public:
    Ref() :
            vector(nullptr),
            index(0) {}
    Ref(std::vector<T *> &vec, unsigned int idx) :
            vector(&vec),
            index(idx) {}

    inline unsigned int GetIndex() const { return index; }

    operator bool() const { return vector != nullptr && index < vector->size(); }

    T *operator->() { return (*vector)[index]; }

    T &operator*() { return *((*vector)[index]); }
};

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

template <class T>
struct ReadHelper<Nullable<T>> {
    static bool Read(Value &val, Nullable<T> &out) {
        return out.isPresent = ReadHelper<T>::Read(val, out.value);
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
inline static bool ReadValue(Value &val, T &out) {
    return ReadHelper<T>::Read(val, out);
}

template <class T>
inline static bool ReadMember(Value &obj, const char *id, T &out) {
    if (!obj.IsObject()) {
        return false;
    }
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
    if (!val.IsObject()) {
        return nullptr;
    }
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd()) ? &it->value : nullptr;
}

template <int N>
inline void throwUnexpectedTypeError(const char (&expectedTypeName)[N], const char *memberId, const char *context, const char *extraContext) {
    std::string fullContext = context;
    if (extraContext && (strlen(extraContext) > 0)) {
        fullContext = fullContext + " (" + extraContext + ")";
    }
    throw DeadlyImportError("Member \"", memberId, "\" was not of type \"", expectedTypeName, "\" when reading ", fullContext);
}

// Look-up functions with type checks. Context and extra context help the user identify the problem if there's an error.

inline Value *FindStringInContext(Value &val, const char *memberId, const char *context, const char *extraContext = nullptr) {
    if (!val.IsObject()) {
        return nullptr;
    }
    Value::MemberIterator it = val.FindMember(memberId);
    if (it == val.MemberEnd()) {
        return nullptr;
    }
    if (!it->value.IsString()) {
        throwUnexpectedTypeError("string", memberId, context, extraContext);
    }
    return &it->value;
}

inline Value *FindNumberInContext(Value &val, const char *memberId, const char *context, const char *extraContext = nullptr) {
    if (!val.IsObject()) {
        return nullptr;
    }
    Value::MemberIterator it = val.FindMember(memberId);
    if (it == val.MemberEnd()) {
        return nullptr;
    }
    if (!it->value.IsNumber()) {
        throwUnexpectedTypeError("number", memberId, context, extraContext);
    }
    return &it->value;
}

inline Value *FindUIntInContext(Value &val, const char *memberId, const char *context, const char *extraContext = nullptr) {
    if (!val.IsObject()) {
        return nullptr;
    }
    Value::MemberIterator it = val.FindMember(memberId);
    if (it == val.MemberEnd()) {
        return nullptr;
    }
    if (!it->value.IsUint()) {
        throwUnexpectedTypeError("uint", memberId, context, extraContext);
    }
    return &it->value;
}

inline Value *FindArrayInContext(Value &val, const char *memberId, const char *context, const char *extraContext = nullptr) {
    if (!val.IsObject()) {
        return nullptr;
    }
    Value::MemberIterator it = val.FindMember(memberId);
    if (it == val.MemberEnd()) {
        return nullptr;
    }
    if (!it->value.IsArray()) {
        throwUnexpectedTypeError("array", memberId, context, extraContext);
    }
    return &it->value;
}

inline Value *FindObjectInContext(Value &val, const char * memberId, const char *context, const char *extraContext = nullptr) {
    if (!val.IsObject()) {
        return nullptr;
    }
    Value::MemberIterator it = val.FindMember(memberId);
    if (it == val.MemberEnd()) {
        return nullptr;
    }
    if (!it->value.IsObject()) {
        ASSIMP_LOG_ERROR("Member \"", memberId, "\" was not of type \"", context, "\" when reading ", extraContext);
        return nullptr;
   }
    return &it->value;
}

inline Value *FindExtensionInContext(Value &val, const char *extensionId, const char *context, const char *extraContext = nullptr) {
    if (Value *extensionList = FindObjectInContext(val, "extensions", context, extraContext)) {
        if (Value *extension = FindObjectInContext(*extensionList, extensionId, context, extraContext)) {
            return extension;
        }
    }
    return nullptr;
}

// Overloads when the value is the document.

inline Value *FindString(Document &doc, const char *memberId) {
    return FindStringInContext(doc, memberId, "the document");
}

inline Value *FindNumber(Document &doc, const char *memberId) {
    return FindNumberInContext(doc, memberId, "the document");
}

inline Value *FindUInt(Document &doc, const char *memberId) {
    return FindUIntInContext(doc, memberId, "the document");
}

inline Value *FindArray(Document &val, const char *memberId) {
    return FindArrayInContext(val, memberId, "the document");
}

inline Value *FindObject(Document &doc, const char *memberId) {
    return FindObjectInContext(doc, memberId, "the document");
}

inline Value *FindExtension(Value &val, const char *extensionId) {
    return FindExtensionInContext(val, extensionId, "the document");
}

inline Value *FindString(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd() && it->value.IsString()) ? &it->value : nullptr;
}

inline Value *FindObject(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd() && it->value.IsObject()) ? &it->value : nullptr;
}

inline Value *FindArray(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd() && it->value.IsArray()) ? &it->value : nullptr;
}

inline Value *FindNumber(Value &val, const char *id) {
    Value::MemberIterator it = val.FindMember(id);
    return (it != val.MemberEnd() && it->value.IsNumber()) ? &it->value : nullptr;
}

} // namespace glTFCommon

#endif // ASSIMP_BUILD_NO_GLTF_IMPORTER

#endif // AI_GLFTCOMMON_H_INC
