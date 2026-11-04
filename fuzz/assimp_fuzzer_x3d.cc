#include "fuzzer_common.h"
#include <assimp/postprocess.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <cstdint>

namespace {
class NoExternalIO final : public Assimp::IOSystem {
public:
    bool Exists(const char *) const override {
        return false;
    }

    char getOsSeparator() const override {
        return '/';
    }

    Assimp::IOStream *Open(const char *, const char * = "rb") override {
        return nullptr;
    }

    void Close(Assimp::IOStream *stream) override {
        delete stream;
    }
};
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t dataSize) {
    if (dataSize == 0 || dataSize > 1024 * 1024) {
        return 0;
    }

    Assimp::Importer importer;
    importer.SetIOHandler(new NoExternalIO());
    if (!AssimpFuzz::ForceFormat(importer, "x3d")) {
        return 0;
    }

    importer.ReadFileFromMemory(data, dataSize, aiProcess_ValidateDataStructure, "x3d");
    return 0;
}