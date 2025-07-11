/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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
#pragma once
#ifdef _WIN32
#  ifndef _CRT_NONSTDC_NO_DEPRECATE
#    define _CRT_NONSTDC_NO_DEPRECATE
#  endif // _CRT_NONSTDC_NO_DEPRECATE
#  ifndef _CRT_SECURE_NO_WARNINGS
#     define _CRT_SECURE_NO_WARNINGS
#  endif
#endif

#include <cstdio>
#include <cstdlib>
#include <gtest/gtest.h>

#if defined(_MSC_VER) || defined(__MINGW64__) || defined(__MINGW32__)
#   define TMP_PATH "./"
#elif defined(__GNUC__) || defined(__clang__)
#   define TMP_PATH "/var/tmp/"
#endif

#if defined(_MSC_VER)

#include <io.h>
inline FILE* MakeTmpFile(char* tmplate, size_t len, std::string &tmpName) {
    size_t tmpLen = len + 1;
    char *pathtemplate = new char[tmpLen];
    strcpy_s(pathtemplate, tmpLen, tmplate);
    int err_code = _mktemp_s(pathtemplate, tmpLen);
    EXPECT_EQ(err_code, 0);
    EXPECT_NE(pathtemplate, nullptr);
    if(pathtemplate == nullptr) {
        delete[] pathtemplate;
        return nullptr;
    }
    errno_t err;
    FILE *fs{nullptr};
    err = fopen_s(&fs, pathtemplate, "w+");
    EXPECT_EQ(0, err);
    tmpName = pathtemplate;
    EXPECT_NE(fs, nullptr);
    delete[] pathtemplate;

    return fs;
}
#elif defined(__GNUC__) || defined(__clang__)
inline FILE *MakeTmpFile(char *tmplate, size_t len, std::string &tmpName) {
    auto fd = mkstemp(tmplate);
    EXPECT_NE(-1, fd);
    if(fd == -1) {
        return nullptr;
    }
    auto fs = fdopen(fd, "w+");
    EXPECT_NE(nullptr, fs);
    tmpName += tmplate;

    return fs;
}
#endif
