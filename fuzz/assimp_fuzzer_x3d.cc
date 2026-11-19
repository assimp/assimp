#include "fuzzer_common.h"
#include <assimp/postprocess.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <cstdint>

namespace {
class NoExternalIO final : public Assimp::IOSystem {
public:
    // Keep the non-virtual base overloads (Exists/Open taking std::string)
    // visible instead of hidden by the overrides below.
    using Assimp::IOSystem::Exists;
    using Assimp::IOSystem::Open;

    bool Exists(const char *) const override {
        return false;
    }

    char getOsSeparator() const override {
        return '/';
    }

    // Always fails: the fuzz target must not touch the file system, all data
    // is provided in-memory through ReadFileFromMemory().
    Assimp::IOStream *Open(const char *, const char * = "rb") override {
        return nullptr;
    }

    // Open() never returns a stream, so Close() is never called with a stream
    // this IOSystem created. Intentionally do nothing: deleting the pointer
    // here would risk freeing a stream owned by another IOSystem (e.g. the
    // MemoryIOSystem used by ReadFileFromMemory) that was already released.
    void Close(Assimp::IOStream *stream) override {
        (void)stream;
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