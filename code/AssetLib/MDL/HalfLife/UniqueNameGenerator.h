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

/** @file UniqueNameGenerator.h
 *  @brief Declaration of the unique name generator.
 */

#ifndef AI_UNIQUENAMEGENERATOR_INCLUDED
#define AI_UNIQUENAMEGENERATOR_INCLUDED

#include <string>
#include <vector>

namespace Assimp {
namespace MDL {
namespace HalfLife {

class UniqueNameGenerator {
public:
    UniqueNameGenerator();
    UniqueNameGenerator(const char *template_name);
    UniqueNameGenerator(const char *template_name, const char *separator);
    ~UniqueNameGenerator();

    inline void set_template_name(const char *template_name) {
        template_name_ = template_name;
    }
    inline void set_separator(const char *separator) {
        separator_ = separator;
    }

    void make_unique(std::vector<std::string> &names);

private:
    std::string template_name_;
    std::string separator_;
};

} // namespace HalfLife
} // namespace MDL
} // namespace Assimp

#endif // AI_UNIQUENAMEGENERATOR_INCLUDED
