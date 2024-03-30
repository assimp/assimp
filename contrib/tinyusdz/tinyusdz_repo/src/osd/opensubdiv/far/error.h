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

#ifndef OPENSUBDIV3_FAR_ERROR_H
#define OPENSUBDIV3_FAR_ERROR_H

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

typedef enum {
    FAR_NO_ERROR,               ///< No error. Move along.
    FAR_FATAL_ERROR,            ///< Issue a fatal error and end the program.
    FAR_INTERNAL_CODING_ERROR,  ///< Issue an internal programming error, but continue execution.
    FAR_CODING_ERROR,           ///< Issue a generic programming error, but continue execution.
    FAR_RUNTIME_ERROR           ///< Issue a generic runtime error, but continue execution.
} ErrorType;


/// \brief The error callback function type (default is "printf")
typedef void (*ErrorCallbackFunc)(ErrorType err, const char *message);

/// \brief Sets the error callback function (default is "printf")
///
/// \note This function is not thread-safe !
///
/// @param func function pointer to the callback function
///
void SetErrorCallback(ErrorCallbackFunc func);


/// \brief The warning callback function type (default is "printf")
typedef void (*WarningCallbackFunc)(const char *message);

/// \brief Sets the warning callback function (default is "printf")
///
/// \note This function is not thread-safe !
///
/// @param func function pointer to the callback function
///
void SetWarningCallback(WarningCallbackFunc func);


//
//  The following are intended for internal use only (and will eventually
//  be moved within namespace internal)
//

/// \brief Sends an OSD error with a message (internal use only)
///
/// @param err     the error type
///
/// @param format  the format of the message (followed by arguments)
///
void Error(ErrorType err, const char *format, ...);

/// \brief Sends an OSD warning message (internal use only)
///
/// @param format  the format of the message (followed by arguments)
///
void Warning(const char *format, ...);


} // end namespace

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif // OPENSUBDIV3_FAR_ERROR_H
