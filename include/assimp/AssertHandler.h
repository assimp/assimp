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

/** @file Provides facilities to replace the default assert handler. */

#ifndef INCLUDED_AI_ASSERTHANDLER_H
#define INCLUDED_AI_ASSERTHANDLER_H

#include <assimp/ai_assert.h>
#include <assimp/defs.h>

namespace Assimp {

// ---------------------------------------------------------------------------
/**
 *  @brief  Signature of functions which handle assert violations.
 */
using AiAssertHandler = void (*)(const char* failedExpression, const char* file, int line);

// ---------------------------------------------------------------------------
/**
 *  @brief  Set the assert handler.
 *  @param  handler  The assertion handler to use.
 */
ASSIMP_API void setAiAssertHandler(AiAssertHandler handler);

// ---------------------------------------------------------------------------
/** The assert handler which is set by default.
 *
 *  @brief  This issues a message to stderr and calls abort.
 *  @param failedExpression   The failed expression as a string.
 *  @param file               The name of the source file.
 *  @param line               The line in the source file.
 */
AI_WONT_RETURN ASSIMP_API void defaultAiAssertHandler(const char* failedExpression, const char* file, int line) AI_WONT_RETURN_SUFFIX;

// ---------------------------------------------------------------------------
/**
 *  @brief Dispatches an assert violation to the assert handler.
 *  @param failedExpression   The failed expression as a string.
 *  @param file               The name of the source file.
 *  @param line               The line in the source file.
 */
ASSIMP_API void aiAssertViolation(const char* failedExpression, const char* file, int line);

} // end of namespace Assimp

#endif // INCLUDED_AI_ASSERTHANDLER_H
