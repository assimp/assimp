//
//   Copyright 2013 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#include "../far/error.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

//
//  Statics for the publicly assignable callbacks and the methods to
//  assign them (disable static assignment warnings when doing so):
//
static ErrorCallbackFunc errorFunc = 0;
static WarningCallbackFunc warningFunc = 0;

#ifdef __INTEL_COMPILER
#pragma warning disable 1711
#endif

void SetErrorCallback(ErrorCallbackFunc func) {
    errorFunc = func;
}

void SetWarningCallback(WarningCallbackFunc func) {
    warningFunc = func;
}

#ifdef __INTEL_COMPILER
#pragma warning enable 1711
#endif


//
//  The default error and warning callbacks eventually belong in the
//  internal namespace:
//
void Error(ErrorType err, const char *format, ...) {

    static char const * errorTypeLabel[] = {
        "No Error",
        "Fatal Error",
        "Coding Error (internal)",
        "Coding Error",
        "Error"
    };

    assert(err!=FAR_NO_ERROR);

    char message[10240];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(message, 10240, format, argptr);
    va_end(argptr);

    if (errorFunc) {
        errorFunc(err, message);
    } else {
        printf("%s: %s\n", errorTypeLabel[err], message);
    }
}

void Warning(const char *format, ...) {

    char message[10240];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(message, 10240, format, argptr);
    va_end(argptr);

    if (warningFunc) {
        warningFunc(message);
    } else {
        fprintf(stdout, "Warning: %s\n", message);
    }
}

} // end namespace

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
