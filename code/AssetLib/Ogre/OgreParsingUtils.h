/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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

#ifndef AI_OGREPARSINGUTILS_H_INC
#define AI_OGREPARSINGUTILS_H_INC

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include <assimp/ParsingUtils.h>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <functional>
#include <sstream>

namespace Assimp {
namespace Ogre {


/// Returns if @c s ends with @c suffix. If @c caseSensitive is false, both strings will be lower cased before matching.
static inline bool EndsWith(const std::string &s, const std::string &suffix, bool caseSensitive = true) {
    if (s.empty() || suffix.empty()) {
        return false;
    } else if (s.length() < suffix.length()) {
        return false;
    }

    if (!caseSensitive) {
        return EndsWith(ai_tolower(s), ai_tolower(suffix), true);
    }

    size_t len = suffix.length();
    std::string sSuffix = s.substr(s.length() - len, len);

    return (ASSIMP_stricmp(sSuffix, suffix) == 0);
}

// Skips a line from current @ss position until a newline. Returns the skipped part.
static inline std::string SkipLine(std::stringstream &ss) {
    std::string skipped;
    getline(ss, skipped);
    return skipped;
}

// Skips a line and reads next element from @c ss to @c nextElement.
/** @return Skipped line content until newline. */
static inline std::string NextAfterNewLine(std::stringstream &ss, std::string &nextElement) {
    std::string skipped = SkipLine(ss);
    ss >> nextElement;
    return skipped;
}

} // namespace Ogre
} // namespace Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
#endif // AI_OGREPARSINGUTILS_H_INC
