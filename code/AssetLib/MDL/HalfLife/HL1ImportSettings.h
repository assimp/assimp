/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

/** @file HL1ImportSettings.h
 *  @brief Half-Life 1 MDL loader configuration settings.
 */

#ifndef AI_HL1IMPORTSETTINGS_INCLUDED
#define AI_HL1IMPORTSETTINGS_INCLUDED

#include <string>

namespace Assimp {
namespace MDL {
namespace HalfLife {

struct HL1ImportSettings {
    HL1ImportSettings() :
        read_animations(false),
        read_animation_events(false),
        read_blend_controllers(false),
        read_sequence_groups_info(false),
        read_sequence_transitions(false),
        read_attachments(false),
        read_bone_controllers(false),
        read_hitboxes(false),
        read_textures(false),
        read_misc_global_info(false) {
    }

    bool read_animations;
    bool read_animation_events;
    bool read_blend_controllers;
    bool read_sequence_groups_info;
    bool read_sequence_transitions;
    bool read_attachments;
    bool read_bone_controllers;
    bool read_hitboxes;
    bool read_textures;
    bool read_misc_global_info;
};

} // namespace HalfLife
} // namespace MDL
} // namespace Assimp

#endif // AI_HL1IMPORTSETTINGS_INCLUDED
