/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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
/** Helper structures for the Collada loader */

#include "ColladaHelper.h"

#include <assimp/ParsingUtils.h>
#include <assimp/commonMetaData.h>

namespace Assimp {
namespace Collada {

const MetaKeyPairVector MakeColladaAssimpMetaKeys() {
    MetaKeyPairVector result;
    result.emplace_back("authoring_tool", AI_METADATA_SOURCE_GENERATOR);
    result.emplace_back("copyright", AI_METADATA_SOURCE_COPYRIGHT);
    return result;
}

const MetaKeyPairVector &GetColladaAssimpMetaKeys() {
    static const MetaKeyPairVector result = MakeColladaAssimpMetaKeys();
    return result;
}

const MetaKeyPairVector MakeColladaAssimpMetaKeysCamelCase() {
    MetaKeyPairVector result = MakeColladaAssimpMetaKeys();
    for (auto &val : result) {
        ToCamelCase(val.first);
    }
    return result;
}

const MetaKeyPairVector &GetColladaAssimpMetaKeysCamelCase() {
    static const MetaKeyPairVector result = MakeColladaAssimpMetaKeysCamelCase();
    return result;
}

// ------------------------------------------------------------------------------------------------
// Convert underscore_separated to CamelCase: "authoring_tool" becomes "AuthoringTool"
void ToCamelCase(std::string &text) {
    if (text.empty())
        return;
    // Capitalise first character
    auto it = text.begin();
    (*it) = ai_toupper(*it);
    ++it;
    for (/*started above*/; it != text.end(); /*iterated below*/) {
        if ((*it) == '_') {
            it = text.erase(it);
            if (it != text.end())
                (*it) = ai_toupper(*it);
        } else {
            // Make lower case
            (*it) = ai_tolower(*it);
            ++it;
        }
    }
}

} // namespace Collada
} // namespace Assimp
