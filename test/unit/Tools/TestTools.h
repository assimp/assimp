#pragma once

#include <gtest/gtest.h>
#include <cstdio>
#include <string>

namespace Assimp::Unittest {

    class TestTools final {
    public:
        TestTools() = default;
        ~TestTools() = default;
        static bool openFilestream(FILE **pFile, const char *filename, const char *mode);
    };

    inline bool TestTools::openFilestream(FILE **fs, const char *filename, const char *mode) {
#if defined(_WIN32)
        errno_t err{ 0 };
        err = fopen_s(fs, filename, mode);
        EXPECT_EQ(err, 0);
#else
        *fs = fopen(filename, mode);
#endif
        return fs != nullptr;
    }

} // namespace Assimp::Unittest
