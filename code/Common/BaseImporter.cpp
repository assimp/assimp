/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
*/

/** @file  BaseImporter.cpp
 *  @brief Implementation of BaseImporter
 */

#include "FileSystemFilter.h"
#include "Importer.h"
#include <assimp/BaseImporter.h>
#include <assimp/ByteSwapper.h>
#include <assimp/ParsingUtils.h>
#include <assimp/importerdesc.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <cctype>
#include <ios>
#include <list>
#include <memory>
#include <sstream>

namespace {
// Checks whether the passed string is a gcs version.
bool IsGcsVersion(const std::string &s) {
    if (s.empty()) return false;
    return std::all_of(s.cbegin(), s.cend(), [](const char c) {
        // gcs only permits numeric characters.
        return std::isdigit(static_cast<int>(c));
    });
}

// Removes a possible version hash from a filename, as found for example in
// gcs uris (e.g. `gs://bucket/model.glb#1234`), see also
// https://github.com/GoogleCloudPlatform/gsutil/blob/c80f329bc3c4011236c78ce8910988773b2606cb/gslib/storage_url.py#L39.
std::string StripVersionHash(const std::string &filename) {
    const std::string::size_type pos = filename.find_last_of('#');
    // Only strip if the hash is behind a possible file extension and the part
    // behind the hash is a version string.
    if (pos != std::string::npos && pos > filename.find_last_of('.') &&
        IsGcsVersion(filename.substr(pos + 1))) {
        return filename.substr(0, pos);
    }
    return filename;
}
}  // namespace

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
BaseImporter::BaseImporter() AI_NO_EXCEPT
        : m_progress() {
    // empty
}

void BaseImporter::UpdateImporterScale(Importer *pImp) {
    ai_assert(pImp != nullptr);
    ai_assert(importerScale != 0.0);
    ai_assert(fileScale != 0.0);

    double activeScale = importerScale * fileScale;

    // Set active scaling
    pImp->SetPropertyFloat(AI_CONFIG_APP_SCALE_KEY, static_cast<float>(activeScale));

    ASSIMP_LOG_DEBUG("UpdateImporterScale scale set: ", activeScale);
}

// ------------------------------------------------------------------------------------------------
// Imports the given file and returns the imported data.
aiScene *BaseImporter::ReadFile(Importer *pImp, const std::string &pFile, IOSystem *pIOHandler) {

    m_progress = pImp->GetProgressHandler();
    if (nullptr == m_progress) {
        return nullptr;
    }

    ai_assert(m_progress);

    // Gather configuration properties for this run
    SetupProperties(pImp);

    // Construct a file system filter to improve our success ratio at reading external files
    FileSystemFilter filter(pFile, pIOHandler);

    // create a scene object to hold the data
    std::unique_ptr<aiScene> sc(new aiScene());

    // dispatch importing
    try {
        InternReadFile(pFile, sc.get(), &filter);

        // Calculate import scale hook - required because pImp not available anywhere else
        // passes scale into ScaleProcess
        UpdateImporterScale(pImp);

    } catch( const std::exception &err ) {
        // extract error description
        m_ErrorText = err.what();
        ASSIMP_LOG_ERROR(err.what());
        m_Exception = std::current_exception();
        return nullptr;
    }

    // return what we gathered from the import.
    return sc.release();
}

// ------------------------------------------------------------------------------------------------
void BaseImporter::SetupProperties(const Importer *) {
    // the default implementation does nothing
}

// ------------------------------------------------------------------------------------------------
void BaseImporter::GetExtensionList(std::set<std::string> &extensions) {
    const aiImporterDesc *desc = GetInfo();
    ai_assert(desc != nullptr);

    const char *ext = desc->mFileExtensions;
    ai_assert(ext != nullptr);

    const char *last = ext;
    do {
        if (!*ext || *ext == ' ') {
            extensions.insert(std::string(last, ext - last));
            ai_assert(ext - last > 0);
            last = ext;
            while (*last == ' ') {
                ++last;
            }
        }
    } while (*ext++);
}

// ------------------------------------------------------------------------------------------------
/*static*/ bool BaseImporter::SearchFileHeaderForToken(IOSystem *pIOHandler,
        const std::string &pFile,
        const char **tokens,
        std::size_t numTokens,
        unsigned int searchBytes /* = 200 */,
        bool tokensSol /* false */,
        bool noGraphBeforeTokens /* false */) {
    ai_assert(nullptr != tokens);
    ai_assert(0 != numTokens);
    ai_assert(0 != searchBytes);

    if (nullptr == pIOHandler) {
        return false;
    }

    std::unique_ptr<IOStream> pStream(pIOHandler->Open(pFile));
    if (pStream) {
        // read 200 characters from the file
        std::unique_ptr<char[]> _buffer(new char[searchBytes + 1 /* for the '\0' */]);
        char *buffer(_buffer.get());
        const size_t read(pStream->Read(buffer, 1, searchBytes));
        if (0 == read) {
            return false;
        }

        for (size_t i = 0; i < read; ++i) {
            buffer[i] = static_cast<char>(::tolower((unsigned char)buffer[i]));
        }

        // It is not a proper handling of unicode files here ...
        // ehm ... but it works in most cases.
        char *cur = buffer, *cur2 = buffer, *end = &buffer[read];
        while (cur != end) {
            if (*cur) {
                *cur2++ = *cur;
            }
            ++cur;
        }
        *cur2 = '\0';

        std::string token;
        for (unsigned int i = 0; i < numTokens; ++i) {
            ai_assert(nullptr != tokens[i]);
            const size_t len(strlen(tokens[i]));
            token.clear();
            const char *ptr(tokens[i]);
            for (size_t tokIdx = 0; tokIdx < len; ++tokIdx) {
                token.push_back(static_cast<char>(tolower(static_cast<unsigned char>(*ptr))));
                ++ptr;
            }
            const char *r = strstr(buffer, token.c_str());
            if (!r) {
                continue;
            }
            // We need to make sure that we didn't accidentally identify the end of another token as our token,
            // e.g. in a previous version the "gltf " present in some gltf files was detected as "f ", or a
            // Blender-exported glb file containing "Khronos glTF Blender I/O " was detected as "o "
            if (noGraphBeforeTokens && (r != buffer && isgraph(static_cast<unsigned char>(r[-1])))) {
                continue;
            }
            // We got a match, either we don't care where it is, or it happens to
            // be in the beginning of the file / line
            if (!tokensSol || r == buffer || r[-1] == '\r' || r[-1] == '\n') {
                ASSIMP_LOG_DEBUG("Found positive match for header keyword: ", tokens[i]);
                return true;
            }
        }
    }

    return false;
}

// ------------------------------------------------------------------------------------------------
// Simple check for file extension
/*static*/ bool BaseImporter::SimpleExtensionCheck(const std::string &pFile,
        const char *ext0,
        const char *ext1,
        const char *ext2,
        const char *ext3) {
    std::set<std::string> extensions;
    for (const char* ext : {ext0, ext1, ext2, ext3}) {
        if (ext == nullptr) continue;
        extensions.emplace(ext);
    }
    return HasExtension(pFile, extensions);
}

// ------------------------------------------------------------------------------------------------
// Check for file extension
/*static*/ bool BaseImporter::HasExtension(const std::string &pFile, const std::set<std::string> &extensions) {
    const std::string file = StripVersionHash(pFile);
    // CAUTION: Do not just search for the extension!
    // GetExtension() returns the part after the *last* dot, but some extensions
    // have dots inside them, e.g. ogre.mesh.xml. Compare the entire end of the
    // string.
    for (const std::string& ext : extensions) {
        // Yay for C++<20 not having std::string::ends_with()
        const std::string dotExt = "." + ext;
        if (dotExt.length() > file.length()) continue;
        // Possible optimization: Fetch the lowercase filename!
        if (0 == ASSIMP_stricmp(file.c_str() + file.length() - dotExt.length(), dotExt.c_str())) {
            return true;
        }
    }
    return false;
}

// ------------------------------------------------------------------------------------------------
// Get file extension from path
std::string BaseImporter::GetExtension(const std::string &pFile) {
    const std::string file = StripVersionHash(pFile);
    std::string::size_type pos = file.find_last_of('.');

    // no file extension at all
    if (pos == std::string::npos) {
        return std::string();
    }

    // thanks to Andy Maloney for the hint
    std::string ret = file.substr(pos + 1);
    ret = ai_tolower(ret);

    return ret;
}


// ------------------------------------------------------------------------------------------------
// Check for magic bytes at the beginning of the file.
/* static */ bool BaseImporter::CheckMagicToken(IOSystem *pIOHandler, const std::string &pFile,
        const void *_magic, std::size_t num, unsigned int offset, unsigned int size) {
    ai_assert(size <= 16);
    ai_assert(_magic);

    if (!pIOHandler) {
        return false;
    }
    const char *magic = reinterpret_cast<const char *>(_magic);
    std::unique_ptr<IOStream> pStream(pIOHandler->Open(pFile));
    if (pStream) {

        // skip to offset
        pStream->Seek(offset, aiOrigin_SET);

        // read 'size' characters from the file
        union {
            char data[16];
            uint16_t data_u16[8];
            uint32_t data_u32[4];
        };
        if (size != pStream->Read(data, 1, size)) {
            return false;
        }

        for (unsigned int i = 0; i < num; ++i) {
            // also check against big endian versions of tokens with size 2,4
            // that's just for convenience, the chance that we cause conflicts
            // is quite low and it can save some lines and prevent nasty bugs
            if (2 == size) {
                uint16_t magic_u16;
                memcpy(&magic_u16, magic, 2);
                if (data_u16[0] == magic_u16 || data_u16[0] == ByteSwap::Swapped(magic_u16)) {
                    return true;
                }
            } else if (4 == size) {
                uint32_t magic_u32;
                memcpy(&magic_u32, magic, 4);
                if (data_u32[0] == magic_u32 || data_u32[0] == ByteSwap::Swapped(magic_u32)) {
                    return true;
                }
            } else {
                // any length ... just compare
                if (!memcmp(magic, data, size)) {
                    return true;
                }
            }
            magic += size;
        }
    }
    return false;
}

#include "utf8.h"

// ------------------------------------------------------------------------------------------------
// Convert to UTF8 data
void BaseImporter::ConvertToUTF8(std::vector<char> &data) {
    //ConversionResult result;
    if (data.size() < 8) {
        throw DeadlyImportError("File is too small");
    }

    // UTF 8 with BOM
    if ((uint8_t)data[0] == 0xEF && (uint8_t)data[1] == 0xBB && (uint8_t)data[2] == 0xBF) {
        ASSIMP_LOG_DEBUG("Found UTF-8 BOM ...");

        std::copy(data.begin() + 3, data.end(), data.begin());
        data.resize(data.size() - 3);
        return;
    }

    // UTF 32 BE with BOM
    if (*((uint32_t *)&data.front()) == 0xFFFE0000) {
        if (data.size() % sizeof(uint32_t) != 0) {
            throw DeadlyImportError("Not valid UTF-32 BE");
        }

        // swap the endianness ..
        for (uint32_t *p = (uint32_t *)&data.front(), *end = (uint32_t *)&data.back(); p <= end; ++p) {
            AI_SWAP4P(p);
        }
    }

    // UTF 32 LE with BOM
    if (*((uint32_t *)&data.front()) == 0x0000FFFE) {
        if (data.size() % sizeof(uint32_t) != 0) {
            throw DeadlyImportError("Not valid UTF-32 LE");
        }
        ASSIMP_LOG_DEBUG("Found UTF-32 BOM ...");

        std::vector<char> output;
        auto *ptr = (uint32_t *)&data[0];
        uint32_t *end = ptr + (data.size() / sizeof(uint32_t)) + 1;
        utf8::utf32to8(ptr, end, back_inserter(output));
        return;
    }

    // UTF 16 BE with BOM
    if (*((uint16_t *)&data.front()) == 0xFFFE) {
        // Check to ensure no overflow can happen
        if (data.size() % sizeof(uint16_t) != 0) {
            throw DeadlyImportError("Not valid UTF-16 BE");
        }
        // swap the endianness ..
        for (uint16_t *p = (uint16_t *)&data.front(), *end = (uint16_t *)&data.back(); p <= end; ++p) {
            ByteSwap::Swap2(p);
        }
    }

    // UTF 16 LE with BOM
    if (*((uint16_t *)&data.front()) == 0xFEFF) {
        if (data.size() % sizeof(uint16_t) != 0) {
            throw DeadlyImportError("Not valid UTF-16 LE");
        }
        ASSIMP_LOG_DEBUG("Found UTF-16 BOM ...");

        std::vector<unsigned char> output;
        utf8::utf16to8(data.begin(), data.end(), back_inserter(output));
        return;
    }
}

// ------------------------------------------------------------------------------------------------
// Convert to UTF8 data to ISO-8859-1
void BaseImporter::ConvertUTF8toISO8859_1(std::string &data) {
    size_t size = data.size();
    size_t i = 0, j = 0;

    while (i < size) {
        if ((unsigned char)data[i] < (size_t)0x80) {
            data[j] = data[i];
        } else if (i < size - 1) {
            if ((unsigned char)data[i] == 0xC2) {
                data[j] = data[++i];
            } else if ((unsigned char)data[i] == 0xC3) {
                data[j] = ((unsigned char)data[++i] + 0x40);
            } else {
                std::stringstream stream;
                stream << "UTF8 code " << std::hex << data[i] << data[i + 1] << " can not be converted into ISA-8859-1.";
                ASSIMP_LOG_ERROR(stream.str());

                data[j++] = data[i++];
                data[j] = data[i];
            }
        } else {
            ASSIMP_LOG_ERROR("UTF8 code but only one character remaining");

            data[j] = data[i];
        }

        i++;
        j++;
    }

    data.resize(j);
}

// ------------------------------------------------------------------------------------------------
void BaseImporter::TextFileToBuffer(IOStream *stream,
        std::vector<char> &data,
        TextFileMode mode) {
    ai_assert(nullptr != stream);

    const size_t fileSize = stream->FileSize();
    if (mode == FORBID_EMPTY) {
        if (!fileSize) {
            throw DeadlyImportError("File is empty");
        }
    }

    data.reserve(fileSize + 1);
    data.resize(fileSize);
    if (fileSize > 0) {
        if (fileSize != stream->Read(&data[0], 1, fileSize)) {
            throw DeadlyImportError("File read error");
        }

        ConvertToUTF8(data);
    }

    // append a binary zero to simplify string parsing
    data.push_back(0);
}

// ------------------------------------------------------------------------------------------------
namespace Assimp {
// Represents an import request
struct LoadRequest {
    LoadRequest(const std::string &_file, unsigned int _flags, const BatchLoader::PropertyMap *_map, unsigned int _id) :
            file(_file),
            flags(_flags),
            refCnt(1),
            scene(nullptr),
            loaded(false),
            id(_id) {
        if (_map) {
            map = *_map;
        }
    }

    bool operator==(const std::string &f) const {
        return file == f;
    }

    const std::string file;
    unsigned int flags;
    unsigned int refCnt;
    aiScene *scene;
    bool loaded;
    BatchLoader::PropertyMap map;
    unsigned int id;
};
} // namespace Assimp

// ------------------------------------------------------------------------------------------------
// BatchLoader::pimpl data structure
struct Assimp::BatchData {
    BatchData(IOSystem *pIO, bool validate) :
            pIOSystem(pIO), pImporter(nullptr), next_id(0xffff), validate(validate) {
        ai_assert(nullptr != pIO);

        pImporter = new Importer();
        pImporter->SetIOHandler(pIO);
    }

    ~BatchData() {
        pImporter->SetIOHandler(nullptr); /* get pointer back into our possession */
        delete pImporter;
    }

    // IO system to be used for all imports
    IOSystem *pIOSystem;

    // Importer used to load all meshes
    Importer *pImporter;

    // List of all imports
    std::list<LoadRequest> requests;

    // Base path
    std::string pathBase;

    // Id for next item
    unsigned int next_id;

    // Validation enabled state
    bool validate;
};

typedef std::list<LoadRequest>::iterator LoadReqIt;

// ------------------------------------------------------------------------------------------------
BatchLoader::BatchLoader(IOSystem *pIO, bool validate) {
    ai_assert(nullptr != pIO);

    m_data = new BatchData(pIO, validate);
}

// ------------------------------------------------------------------------------------------------
BatchLoader::~BatchLoader() {
    // delete all scenes what have not been polled by the user
    for (LoadReqIt it = m_data->requests.begin(); it != m_data->requests.end(); ++it) {
        delete (*it).scene;
    }
    delete m_data;
}

// ------------------------------------------------------------------------------------------------
void BatchLoader::setValidation(bool enabled) {
    m_data->validate = enabled;
}

// ------------------------------------------------------------------------------------------------
bool BatchLoader::getValidation() const {
    return m_data->validate;
}

// ------------------------------------------------------------------------------------------------
unsigned int BatchLoader::AddLoadRequest(const std::string &file,
        unsigned int steps /*= 0*/, const PropertyMap *map /*= nullptr*/) {
    ai_assert(!file.empty());

    // check whether we have this loading request already
    for (LoadReqIt it = m_data->requests.begin(); it != m_data->requests.end(); ++it) {
        // Call IOSystem's path comparison function here
        if (m_data->pIOSystem->ComparePaths((*it).file, file)) {
            if (map) {
                if (!((*it).map == *map)) {
                    continue;
                }
            } else if (!(*it).map.empty()) {
                continue;
            }

            (*it).refCnt++;
            return (*it).id;
        }
    }

    // no, we don't have it. So add it to the queue ...
    m_data->requests.emplace_back(file, steps, map, m_data->next_id);
    return m_data->next_id++;
}

// ------------------------------------------------------------------------------------------------
aiScene *BatchLoader::GetImport(unsigned int which) {
    for (LoadReqIt it = m_data->requests.begin(); it != m_data->requests.end(); ++it) {
        if ((*it).id == which && (*it).loaded) {
            aiScene *sc = (*it).scene;
            if (!(--(*it).refCnt)) {
                m_data->requests.erase(it);
            }
            return sc;
        }
    }
    return nullptr;
}

// ------------------------------------------------------------------------------------------------
void BatchLoader::LoadAll() {
    // no threaded implementation for the moment
    for (LoadReqIt it = m_data->requests.begin(); it != m_data->requests.end(); ++it) {
        // force validation in debug builds
        unsigned int pp = (*it).flags;
        if (m_data->validate) {
            pp |= aiProcess_ValidateDataStructure;
        }

        // setup config properties if necessary
        ImporterPimpl *pimpl = m_data->pImporter->Pimpl();
        pimpl->mFloatProperties = (*it).map.floats;
        pimpl->mIntProperties = (*it).map.ints;
        pimpl->mStringProperties = (*it).map.strings;
        pimpl->mMatrixProperties = (*it).map.matrices;

        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_INFO("%%% BEGIN EXTERNAL FILE %%%");
            ASSIMP_LOG_INFO("File: ", (*it).file);
        }
        m_data->pImporter->ReadFile((*it).file, pp);
        (*it).scene = m_data->pImporter->GetOrphanedScene();
        (*it).loaded = true;

        ASSIMP_LOG_INFO("%%% END EXTERNAL FILE %%%");
    }
}
