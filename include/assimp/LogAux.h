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

/** @file  LogAux.h
 *  @brief Common logging usage patterns for importer implementations
 */
#pragma once
#ifndef INCLUDED_AI_LOGAUX_H
#define INCLUDED_AI_LOGAUX_H

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/TinyFormatter.h>
#include <assimp/Exceptional.h>
#include <assimp/DefaultLogger.hpp>

namespace Assimp {

/// @brief Logger class, which will exten the class by log-functions.
/// @tparam TDeriving 
template<class TDeriving>
class LogFunctions {
public:
    // ------------------------------------------------------------------------------------------------
    template<typename... T>
    static void ThrowException(T&&... args)
    {
        throw DeadlyImportError(Prefix(), std::forward<T>(args)...);
    }

    // ------------------------------------------------------------------------------------------------
    template<typename... T>
    static void LogWarn(T&&... args) {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_WARN(Prefix(), std::forward<T>(args)...);
        }
    }

    // ------------------------------------------------------------------------------------------------
    template<typename... T>
    static void LogError(T&&... args)  {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_ERROR(Prefix(), std::forward<T>(args)...);
        }
    }

    // ------------------------------------------------------------------------------------------------
    template<typename... T>
    static void LogInfo(T&&... args)  {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_INFO(Prefix(), std::forward<T>(args)...);
        }
    }

    // ------------------------------------------------------------------------------------------------
    template<typename... T>
    static void LogDebug(T&&... args)  {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_DEBUG(Prefix(), std::forward<T>(args)...);
        }
    }

    // ------------------------------------------------------------------------------------------------
    template<typename... T>
    static void LogVerboseDebug(T&&... args)  {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_VERBOSE_DEBUG(Prefix(), std::forward<T>(args)...);
        }
    }

private:
    static const char* Prefix();
};

} // ! Assimp

#endif // INCLUDED_AI_LOGAUX_H
