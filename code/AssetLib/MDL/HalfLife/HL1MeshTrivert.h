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

/** @file HL1MeshTrivert.h
 *  @brief This file contains the class declaration for the
 *         HL1 mesh trivert class.
 */

#ifndef AI_HL1MESHTRIVERT_INCLUDED
#define AI_HL1MESHTRIVERT_INCLUDED

#include "HL1FileData.h"

namespace Assimp {
namespace MDL {
namespace HalfLife {

/* A class to help map model triverts to mesh triverts. */
struct HL1MeshTrivert {
    HL1MeshTrivert() :
            vertindex(-1),
            normindex(-1),
            s(0),
            t(0),
            localindex(-1) {
    }

    HL1MeshTrivert(short vertindex, short normindex, short s, short t, short localindex) :
            vertindex(vertindex),
            normindex(normindex),
            s(s),
            t(t),
            localindex(localindex) {
    }

    HL1MeshTrivert(const Trivert &a) :
            vertindex(a.vertindex),
            normindex(a.normindex),
            s(a.s),
            t(a.t),
            localindex(-1) {
    }

    inline bool operator==(const Trivert &a) const {
        return vertindex == a.vertindex &&
               normindex == a.normindex &&
               s == a.s &&
               t == a.t;
    }

    inline bool operator!=(const Trivert &a) const {
        return !(*this == a);
    }

    inline bool operator==(const HL1MeshTrivert &a) const {
        return localindex == a.localindex &&
               vertindex == a.vertindex &&
               normindex == a.normindex &&
               s == a.s &&
               t == a.t;
    }

    inline bool operator!=(const HL1MeshTrivert &a) const {
        return !(*this == a);
    }

    inline HL1MeshTrivert &operator=(const Trivert &other) {
        vertindex = other.vertindex;
        normindex = other.normindex;
        s = other.s;
        t = other.t;
        return *this;
    }

    short vertindex;
    short normindex;
    short s, t;
    short localindex;
};

struct HL1MeshFace {
    short v0, v1, v2;
};

} // namespace HalfLife
} // namespace MDL
} // namespace Assimp

#endif // AI_HL1MESHTRIVERT_INCLUDED
