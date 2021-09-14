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

#pragma once
#ifndef AI_INCLUDED_EXCEPTIONAL_H
#define AI_INCLUDED_EXCEPTIONAL_H

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/DefaultIOStream.h>
#include <assimp/TinyFormatter.h>
#include <stdexcept>

using std::runtime_error;

#ifdef _MSC_VER
#pragma warning(disable : 4275)
#endif

// ---------------------------------------------------------------------------
/**
 *  The base-class for all other exceptions
 */
class ASSIMP_API DeadlyErrorBase : public runtime_error {
protected:
    /// @brief The class constructor with the formatter.
    /// @param f    The formatter.
    DeadlyErrorBase(Assimp::Formatter::format f);

    /// @brief The class constructor with the parameter ellipse.
    /// @tparam ...T    The type for the ellipse
    /// @tparam U       The other type
    /// @param f        The formatter
    /// @param u        One parameter
    /// @param ...args  The rest
    template<typename... T, typename U>
    DeadlyErrorBase(Assimp::Formatter::format f, U&& u, T&&... args) :
            DeadlyErrorBase(std::move(f << std::forward<U>(u)), std::forward<T>(args)...) {}
};

// ---------------------------------------------------------------------------
/** FOR IMPORTER PLUGINS ONLY: Simple exception class to be thrown if an
 *  unrecoverable error occurs while importing. Loading APIs return
 *  nullptr instead of a valid aiScene then.  */
class ASSIMP_API DeadlyImportError : public DeadlyErrorBase {
public:
    /// @brief The class constructor with the message.
    /// @param message  The message
    DeadlyImportError(const char *message) :
            DeadlyErrorBase(Assimp::Formatter::format(), std::forward<const char*>(message)) {
        // empty
    }

    /// @brief The class constructor with the parameter ellipse.
    /// @tparam ...T    The type for the ellipse
    /// @param ...args  The args
    template<typename... T>
    explicit DeadlyImportError(T&&... args) :
            DeadlyErrorBase(Assimp::Formatter::format(), std::forward<T>(args)...) {
        // empty
    }

#if defined(_MSC_VER) && defined(__clang__)
    DeadlyImportError(DeadlyImportError& other) = delete;
#endif
};

// ---------------------------------------------------------------------------
/** FOR EXPORTER PLUGINS ONLY: Simple exception class to be thrown if an
 *  unrecoverable error occurs while exporting. Exporting APIs return
 *  nullptr instead of a valid aiScene then.  */
class ASSIMP_API DeadlyExportError : public DeadlyErrorBase {
public:
    /** Constructor with arguments */
    template<typename... T>
    explicit DeadlyExportError(T&&... args) :
            DeadlyErrorBase(Assimp::Formatter::format(), std::forward<T>(args)...) {}

#if defined(_MSC_VER) && defined(__clang__)
    DeadlyExportError(DeadlyExportError& other) = delete;
#endif
};

#ifdef _MSC_VER
#pragma warning(default : 4275)
#endif

// ---------------------------------------------------------------------------
template <typename T>
struct ExceptionSwallower {
    T operator()() const {
        return T();
    }
};

// ---------------------------------------------------------------------------
template <typename T>
struct ExceptionSwallower<T *> {
    T *operator()() const {
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
template <>
struct ExceptionSwallower<aiReturn> {
    aiReturn operator()() const {
        try {
            throw;
        } catch (std::bad_alloc &) {
            return aiReturn_OUTOFMEMORY;
        } catch (...) {
            return aiReturn_FAILURE;
        }
    }
};

// ---------------------------------------------------------------------------
template <>
struct ExceptionSwallower<void> {
    void operator()() const {
        return;
    }
};

#define ASSIMP_BEGIN_EXCEPTION_REGION() \
    {                                   \
        try {

#define ASSIMP_END_EXCEPTION_REGION_WITH_ERROR_STRING(type, ASSIMP_END_EXCEPTION_REGION_errorString, ASSIMP_END_EXCEPTION_REGION_exception)     \
    }                                                                                                                                           \
    catch (const DeadlyImportError &e) {                                                                                                        \
        ASSIMP_END_EXCEPTION_REGION_errorString = e.what();                                                                                     \
        ASSIMP_END_EXCEPTION_REGION_exception = std::current_exception();                                                                       \
        return ExceptionSwallower<type>()();                                                                                                    \
    }                                                                                                                                           \
    catch (...) {                                                                                                                               \
        ASSIMP_END_EXCEPTION_REGION_errorString = "Unknown exception";                                                                          \
        ASSIMP_END_EXCEPTION_REGION_exception = std::current_exception();                                                                       \
        return ExceptionSwallower<type>()();                                                                                                    \
    }                                                                                                                                           \
}

#define ASSIMP_END_EXCEPTION_REGION(type)    \
    }                                        \
    catch (...) {                            \
        return ExceptionSwallower<type>()(); \
    }                                        \
    }

#endif // AI_INCLUDED_EXCEPTIONAL_H
