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

/** @file UniqueNameGenerator.cpp
 *  @brief Implementation for the unique name generator.
 */

#include "UniqueNameGenerator.h"
#include <algorithm>
#include <list>
#include <map>
#include <numeric>

namespace Assimp {
namespace MDL {
namespace HalfLife {

UniqueNameGenerator::UniqueNameGenerator() :
    template_name_("unnamed"),
    separator_("_") {
}

UniqueNameGenerator::UniqueNameGenerator(const char *template_name) :
    template_name_(template_name),
    separator_("_") {
}

UniqueNameGenerator::UniqueNameGenerator(const char *template_name, const char *separator) :
    template_name_(template_name),
    separator_(separator) {
}

UniqueNameGenerator::~UniqueNameGenerator() {
}

void UniqueNameGenerator::make_unique(std::vector<std::string> &names) {
    struct DuplicateInfo {
        DuplicateInfo() :
            indices(),
            next_id(0) {
        }

        std::list<size_t> indices;
        size_t next_id;
    };

    std::vector<size_t> empty_names_indices;
    std::vector<size_t> template_name_duplicates;
    std::map<std::string, DuplicateInfo> names_to_duplicates;

    const std::string template_name_with_separator(template_name_ + separator_);

    auto format_name = [&](const std::string &base_name, size_t id) -> std::string {
        return base_name + separator_ + std::to_string(id);
    };

    auto generate_unique_name = [&](const std::string &base_name) -> std::string {
        auto *duplicate_info = &names_to_duplicates[base_name];

        std::string new_name = "";

        bool found_identical_name;
        bool tried_with_base_name_only = false;
        do {
            // Assume that no identical name exists.
            found_identical_name = false;

            if (!tried_with_base_name_only) {
                // First try with only the base name.
                new_name = base_name;
            } else {
                // Create the name expected to be unique.
                new_name = format_name(base_name, duplicate_info->next_id);
            }

            // Check in the list of duplicates for an identical name.
            for (size_t i = 0;
                    i < names.size() &&
                    !found_identical_name;
                    ++i) {
                if (new_name == names[i])
                    found_identical_name = true;
            }

            if (tried_with_base_name_only)
                ++duplicate_info->next_id;

            tried_with_base_name_only = true;

        } while (found_identical_name);

        return new_name;
    };

    for (size_t i = 0; i < names.size(); ++i) {
        // Check for empty names.
        if (names[i].find_first_not_of(' ') == std::string::npos) {
            empty_names_indices.push_back(i);
            continue;
        }

        /* Check for potential duplicate.
        a) Either if this name is the same as the template name or
        b) <template name><separator> is found at the beginning. */
        if (names[i] == template_name_ ||
                names[i].substr(0, template_name_with_separator.length()) == template_name_with_separator)
            template_name_duplicates.push_back(i);

        // Map each unique name to it's duplicate.
        if (names_to_duplicates.count(names[i]) == 0)
            names_to_duplicates.insert({ names[i], DuplicateInfo()});
        else
            names_to_duplicates[names[i]].indices.push_back(i);
    }

    // Make every non-empty name unique.
    for (auto it = names_to_duplicates.begin();
            it != names_to_duplicates.end(); ++it) {
        for (auto it2 = it->second.indices.begin();
                it2 != it->second.indices.end();
                ++it2)
            names[*it2] = generate_unique_name(it->first);
    }

    // Generate a unique name for every empty string.
    if (template_name_duplicates.size()) {
        // At least one string ressembles to <template name>.
        for (auto it = empty_names_indices.begin();
                it != empty_names_indices.end(); ++it)
            names[*it] = generate_unique_name(template_name_);
    } else {
        // No string alike <template name> exists.
        size_t i = 0;
        for (auto it = empty_names_indices.begin();
                it != empty_names_indices.end(); ++it, ++i)
            names[*it] = format_name(template_name_, i);
    }
}

} // namespace HalfLife
} // namespace MDL
} // namespace Assimp
