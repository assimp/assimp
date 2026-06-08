/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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

/** @file Profiler.h
 *  @brief Utility to measure the respective runtime of each import step
 */
#pragma once
#ifndef AI_INCLUDED_PROFILER_H
#define AI_INCLUDED_PROFILER_H

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <chrono>
#include <assimp/DefaultLogger.hpp>
#include <assimp/TinyFormatter.h>

#include <map>

namespace Assimp::Profiling {

using namespace Formatter;

// ------------------------------------------------------------------------------------------------
/// @brief Simple wrapper around boost::timer to simplify reporting. 
///
/// Timings are automatically dumped to the log file.
class Profiler {
public:
    /// @brief The class constructor.
    Profiler() = default;

    /// @brief The class destructor.
    ~Profiler() = default;

    /// @brief Starts a named timer.
    /// @param region    The profiling region name.
    void BeginRegion(const std::string& region) {
        mRegions[region] = std::chrono::system_clock::now();
        ASSIMP_LOG_DEBUG("START `",region,"`");
    }

    /// @brief End a specific named timer and write its end time to the log.
    /// @param region    The profiling region name.
    void EndRegion(const std::string& region) {
        if (auto it = mRegions.find(region); it == mRegions.end()) {
            return;
        }

        auto elapsedSeconds = std::chrono::system_clock::now() - mRegions[region];
        ASSIMP_LOG_DEBUG("END   `",region,"`, dt= ", elapsedSeconds.count()," s");
    }

private:
    using RegionMap = std::map<std::string,std::chrono::time_point<std::chrono::system_clock>>;
    RegionMap mRegions{};
};

} // namespace Assimp::Profiling

#endif // AI_INCLUDED_PROFILER_H

