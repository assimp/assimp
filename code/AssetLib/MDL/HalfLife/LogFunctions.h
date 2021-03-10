/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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

/** @file LogFunctions.h */

#ifndef AI_MDL_HALFLIFE_LOGFUNCTIONS_INCLUDED
#define AI_MDL_HALFLIFE_LOGFUNCTIONS_INCLUDED

#include <assimp/Logger.hpp>
#include <string>

namespace Assimp {
namespace MDL {
namespace HalfLife {

/**
 * \brief A function to log precise messages regarding limits exceeded.
 *
 * \param[in] subject Subject.
 * \param[in] current_amount Current amount.
 * \param[in] direct_object Direct object.
 *            LIMIT Limit constant.
 *
 * Example: Model has 100 textures, which exceeds the limit (50)
 *
 *          where \p subject is 'Model'
 *                \p current_amount is '100'
 *                \p direct_object is 'textures'
 *                LIMIT is '50'
 */
template <int LIMIT>
static inline void log_warning_limit_exceeded(
    const std::string &subject, int current_amount,
    const std::string &direct_object) {

    ASSIMP_LOG_WARN(MDL_HALFLIFE_LOG_HEADER
        + subject
        + " has "
        + std::to_string(current_amount) + " "
        + direct_object
        + ", which exceeds the limit ("
        + std::to_string(LIMIT)
        + ")");
}

/** \brief Same as above, but uses 'Model' as the subject. */
template <int LIMIT>
static inline void log_warning_limit_exceeded(int current_amount,
    const std::string &direct_object) {
    log_warning_limit_exceeded<LIMIT>("Model", current_amount, direct_object);
}

} // namespace HalfLife
} // namespace MDL
} // namespace Assimp

#endif // AI_MDL_HALFLIFE_LOGFUNCTIONS_INCLUDED
