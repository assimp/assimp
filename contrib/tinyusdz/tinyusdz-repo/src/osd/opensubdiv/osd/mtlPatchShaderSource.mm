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

#include "../osd/mtlPatchShaderSource.h"
#include "../far/error.h"

#include <sstream>
#include <string>
#include <TargetConditionals.h>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

static std::string commonShaderSource(
#include "mtlPatchCommon.gen.h"
);
static std::string commonTessShaderSource(
#include "mtlPatchCommonTess.gen.h"
);
static std::string patchLegacyShaderSource(
#include "mtlPatchLegacy.gen.h"
);
static std::string patchBasisTypesShaderSource(
#include "patchBasisCommonTypes.gen.h"
);
static std::string patchBasisShaderSource(
#include "patchBasisCommon.gen.h"
);
static std::string patchBasisEvalShaderSource(
#include "patchBasisCommonEval.gen.h"
);
static std::string bsplineShaderSource(
#include "mtlPatchBSpline.gen.h"
);
static std::string boxSplineTriangleShaderSource(
#include "mtlPatchBoxSplineTriangle.gen.h"
);
static std::string gregoryShaderSource(
#include "mtlPatchGregory.gen.h"
);
static std::string gregoryBasisShaderSource(
#include "mtlPatchGregoryBasis.gen.h"
);
static std::string gregoryTriangleShaderSource(
#include "mtlPatchGregoryTriangle.gen.h"
);

static std::string
GetPatchTypeDefine(Far::PatchDescriptor::Type type,
                   Far::PatchDescriptor::Type fvarType =
                       Far::PatchDescriptor::NON_PATCH) {

    std::stringstream ss;
    switch(type) {
        case Far::PatchDescriptor::LINES:
            ss << "#define OSD_PATCH_LINES 1\n";
            break;
        case Far::PatchDescriptor::TRIANGLES:
            ss << "#define OSD_PATCH_TRIANGLES 1\n";
            break;
        case Far::PatchDescriptor::QUADS:
            ss << "#define OSD_PATCH_QUADS 1\n";
            break;
        case Far::PatchDescriptor::REGULAR:
            ss << "#define OSD_PATCH_BSPLINE 1\n"
                  "#define OSD_PATCH_REGULAR 1\n";
            break;
        case Far::PatchDescriptor::LOOP:
            ss << "#define OSD_PATCH_BOX_SPLINE_TRIANGLE 1\n";
            break;
        case Far::PatchDescriptor::GREGORY:
            ss << "#define OSD_PATCH_GREGORY 1\n";
            break;
        case Far::PatchDescriptor::GREGORY_BOUNDARY:
            ss << "#define OSD_PATCH_GREGORY_BOUNDARY 1\n";
            break;
        case Far::PatchDescriptor::GREGORY_BASIS:
            ss << "#define OSD_PATCH_GREGORY_BASIS 1\n";
            break;
        case Far::PatchDescriptor::GREGORY_TRIANGLE:
            ss << "#define OSD_PATCH_GREGORY_TRIANGLE 1\n";
            break;
        default:
            assert("Unknown Far::PatchDescriptor::Type" && 0);
            return "";
    }
    switch(fvarType) {
        case Far::PatchDescriptor::REGULAR:
            ss << "#define OSD_FACEVARYING_PATCH_REGULAR 1\n";
            break;
        case Far::PatchDescriptor::GREGORY_BASIS:
            ss << "#define OSD_FACEVARYING_PATCH_GREGORY_BASIS 1\n";
            break;
        default:
            return ss.str();
    }
    return ss.str();
}

static std::string
GetPatchTypeSource(Far::PatchDescriptor::Type type) {

    switch(type) {
        case Far::PatchDescriptor::QUADS:
        case Far::PatchDescriptor::TRIANGLES:
            return "";
        case Far::PatchDescriptor::REGULAR:
            return bsplineShaderSource;
        case Far::PatchDescriptor::LOOP:
            return boxSplineTriangleShaderSource;
        case Far::PatchDescriptor::GREGORY:
            return gregoryShaderSource;
        case Far::PatchDescriptor::GREGORY_BOUNDARY:
            return gregoryShaderSource;
        case Far::PatchDescriptor::GREGORY_BASIS:
            return gregoryBasisShaderSource;
        case Far::PatchDescriptor::GREGORY_TRIANGLE:
            return gregoryTriangleShaderSource;
        default:
            assert("Unknown Far::PatchDescriptor::Type" && 0);
            return "";
    }
}

/*static*/
std::string
MTLPatchShaderSource::GetCommonShaderSource() {
#if TARGET_OS_IOS || TARGET_OS_TV
    return std::string("#define OSD_METAL_IOS 1\n")
               .append(commonShaderSource)
               .append(commonTessShaderSource)
               .append(patchLegacyShaderSource);
#elif TARGET_OS_OSX
    return std::string("#define OSD_METAL_OSX 1\n")
               .append(commonShaderSource)
               .append(commonTessShaderSource)
               .append(patchLegacyShaderSource);
#endif
}

/*static*/
std::string
MTLPatchShaderSource::GetPatchBasisShaderSource() {
    std::stringstream ss;
    ss << "#define OSD_PATCH_BASIS_METAL 1\n";
#if defined(OPENSUBDIV_GREGORY_EVAL_TRUE_DERIVATIVES)
    ss << "#define OPENSUBDIV_GREGORY_EVAL_TRUE_DERIVATIVES 1\n";
#endif
    ss << patchBasisTypesShaderSource;
    ss << patchBasisShaderSource;
    ss << patchBasisEvalShaderSource;
    return ss.str();
}

/*static*/
std::string
MTLPatchShaderSource::GetVertexShaderSource(Far::PatchDescriptor::Type type) {
    std::stringstream ss;
    ss << GetPatchTypeDefine(type);
    ss << GetCommonShaderSource();
    ss << GetPatchTypeSource(type);
    return ss.str();
}

/*static*/
std::string
MTLPatchShaderSource::GetVertexShaderSource(Far::PatchDescriptor::Type type,
                                            Far::PatchDescriptor::Type fvarType) {
    std::stringstream ss;
    ss << GetPatchTypeDefine(type, fvarType);
    ss << GetCommonShaderSource();
    ss << GetPatchTypeSource(type);
    return ss.str();
}

/*static*/
std::string
MTLPatchShaderSource::GetHullShaderSource(Far::PatchDescriptor::Type type) {
    std::stringstream ss;
    ss << GetPatchTypeDefine(type);
    ss << GetCommonShaderSource();
    ss << GetPatchTypeSource(type);
    return ss.str();
}

/*static*/
std::string
MTLPatchShaderSource::GetHullShaderSource(Far::PatchDescriptor::Type type,
                                          Far::PatchDescriptor::Type fvarType) {
    std::stringstream ss;
    ss << GetPatchTypeDefine(type, fvarType);
    ss << GetCommonShaderSource();
    ss << GetPatchTypeSource(type);
    return ss.str();
}

/*static*/
std::string
MTLPatchShaderSource::GetDomainShaderSource(Far::PatchDescriptor::Type type) {
    std::stringstream ss;
    ss << GetPatchTypeDefine(type);
    ss << GetCommonShaderSource();
    ss << GetPatchTypeSource(type);
    return ss.str();
}

/*static*/
std::string
MTLPatchShaderSource::GetDomainShaderSource(Far::PatchDescriptor::Type type,
                                            Far::PatchDescriptor::Type fvarType) {
    std::stringstream ss;
    ss << GetPatchTypeDefine(type, fvarType);
    ss << GetCommonShaderSource();
    ss << GetPatchTypeSource(type);
    return ss.str();
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
