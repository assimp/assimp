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
#include "../sdc/types.h"

#include "../sdc/bilinearScheme.h"
#include "../sdc/catmarkScheme.h"
#include "../sdc/loopScheme.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Sdc {

struct TraitsEntry {
    char const * _name;

    Split _splitType;
    int   _regularFaceSize;
    int   _regularVertexValence;
    int   _localNeighborhood;
};

static const TraitsEntry staticTraitsTable[3] = {
    { "bilinear", Scheme<SCHEME_BILINEAR>::GetTopologicalSplitType(),
                  Scheme<SCHEME_BILINEAR>::GetRegularFaceSize(),
                  Scheme<SCHEME_BILINEAR>::GetRegularVertexValence(),
                  Scheme<SCHEME_BILINEAR>::GetLocalNeighborhoodSize() },
    { "catmark",  Scheme<SCHEME_CATMARK>::GetTopologicalSplitType(),
                  Scheme<SCHEME_CATMARK>::GetRegularFaceSize(),
                  Scheme<SCHEME_CATMARK>::GetRegularVertexValence(),
                  Scheme<SCHEME_CATMARK>::GetLocalNeighborhoodSize() },
    { "loop",     Scheme<SCHEME_LOOP>::GetTopologicalSplitType(),
                  Scheme<SCHEME_LOOP>::GetRegularFaceSize(),
                  Scheme<SCHEME_LOOP>::GetRegularVertexValence(),
                  Scheme<SCHEME_LOOP>::GetLocalNeighborhoodSize() }
};

//
//  Static methods for SchemeTypeTraits:
//
char const*
SchemeTypeTraits::GetName(SchemeType schemeType) {

    return staticTraitsTable[schemeType]._name;
}

Split
SchemeTypeTraits::GetTopologicalSplitType(SchemeType schemeType) {

    return staticTraitsTable[schemeType]._splitType;
}

int
SchemeTypeTraits::GetRegularFaceSize(SchemeType schemeType) {

    return staticTraitsTable[schemeType]._regularFaceSize;
}

int
SchemeTypeTraits::GetRegularVertexValence(SchemeType schemeType) {

    return staticTraitsTable[schemeType]._regularVertexValence;
}

int
SchemeTypeTraits::GetLocalNeighborhoodSize(SchemeType schemeType) {

    return staticTraitsTable[schemeType]._localNeighborhood;
}

} // end namespace sdc

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
