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
#ifndef OPENSUBDIV3_VTR_TYPES_H
#define OPENSUBDIV3_VTR_TYPES_H

#include "../version.h"

#include "../vtr/array.h"

#include <vector>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {

//
//  A few types (and constants) for use within Vtr and potentially by its
//  clients (appropriately exported and retyped)
//

//
//  Integer type and constants to index the vectors of components.  Note that we
//  can't use specific width integer types like uint32_t, etc. as use of stdint
//  is not portable.
//
//  The convention throughout the OpenSubdiv code is to use "int" in most places,
//  with "unsigned int" being limited to a few cases (why?).  So we continue that
//  trend here and use "int" for topological indices (with -1 indicating "invalid")
//  despite the fact that we lose half the range compared to using "uint" (with ~0
//  as invalid).
//
typedef int Index;

static const Index INDEX_INVALID = -1;

inline bool IndexIsValid(Index index) { return (index != INDEX_INVALID); }

//
//  Integer type and constants used to index one component within another.  Ideally
//  this is just 2 bits once refinement reduces faces to tris or quads -- and so
//  could potentially be combined with an Index -- but we need something larger for
//  the N-sided face.
//
typedef unsigned short  LocalIndex;

//  Declared as "int" since it's intended for more general use
static const int VALENCE_LIMIT = ((1 << 16) - 1);  // std::numeric_limits<LocalIndex>::max()

//
//  Collections of integer types in variable or fixed sized arrays.  Note that the use
//  of "vector" in the name indicates a class that wraps an std::vector (typically a
//  member variable) which is fully resizable and owns its own storage, whereas "array"
//  wraps a vtr::Array which uses a fixed block of pre-allocated memory.
//
typedef std::vector<Index>  IndexVector;

typedef Array<Index>             IndexArray;
typedef ConstArray<Index>        ConstIndexArray;

typedef Array<LocalIndex>        LocalIndexArray;
typedef ConstArray<LocalIndex>   ConstLocalIndexArray;


} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_VTR_TYPES_H */
