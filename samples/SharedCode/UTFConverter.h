/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team



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

#ifndef ASSIMP_SAMPLES_SHARED_CODE_UTFCONVERTER_H
#define ASSIMP_SAMPLES_SHARED_CODE_UTFCONVERTER_H

#include <string>
#include <locale>
#include <codecvt>

namespace AssimpSamples {
namespace SharedCode {

// Used to convert between multibyte and unicode strings.
class UTFConverter {
    using UTFConverterImpl = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;
public:
    UTFConverter(const char* s) :
        s_(s),
        ws_(impl_.from_bytes(s)) {
    }
    UTFConverter(const wchar_t* s) :
        s_(impl_.to_bytes(s)),
        ws_(s) {
    }
    UTFConverter(const std::string& s) :
        s_(s),
        ws_(impl_.from_bytes(s)) {
    }
    UTFConverter(const std::wstring& s) :
        s_(impl_.to_bytes(s)),
        ws_(s) {
    }
    inline const char* c_str() const {
        return s_.c_str();
    }
    inline const std::string& str() const {
        return s_;
    }
    inline const wchar_t* c_wstr() const {
        return ws_.c_str();
    }
private:
    static UTFConverterImpl impl_;
    std::string s_;
    std::wstring ws_;
};

}
}

#endif // ASSIMP_SAMPLES_SHARED_CODE_UTFCONVERTER_H
