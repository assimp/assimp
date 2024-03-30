//
//   Copyright 2014 DreamWorks Animation LLC.
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

#ifndef OPENSUBDIV3_FAR_TYPES_H
#define OPENSUBDIV3_FAR_TYPES_H

#include "../version.h"

#include "../vtr/types.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

//
//  Typedefs for indices that are inherited from the Vtr level -- eventually
//  these primitive Vtr types may be declared at a lower, more public level.
//
typedef Vtr::Index       Index;
typedef Vtr::LocalIndex  LocalIndex;

typedef Vtr::IndexArray       IndexArray;
typedef Vtr::LocalIndexArray  LocalIndexArray;

typedef Vtr::ConstIndexArray       ConstIndexArray;
typedef Vtr::ConstLocalIndexArray  ConstLocalIndexArray;

inline bool IndexIsValid(Index index) { return Vtr::IndexIsValid(index); }

static const Index INDEX_INVALID = Vtr::INDEX_INVALID;
static const int   VALENCE_LIMIT = Vtr::VALENCE_LIMIT;

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_FAR_TYPES_H */
