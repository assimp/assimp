//
//   Copyright 2021 Pixar
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

#ifndef OPENSUBDIV3_BFR_SURFACE_DATA_H
#define OPENSUBDIV3_BFR_SURFACE_DATA_H

#include "../version.h"

#include "../bfr/parameterization.h"
#include "../bfr/irregularPatchType.h"

#include "../vtr/stackBuffer.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {
namespace internal {

//
//  SurfaceData is a simple internal class that encapsulates all member
//  variables of a Surface -- allowing the SurfaceFactory to initialize
//  a Surface independent of its final type.
//
//  Since internal, and access to instances of SurfaceData is restricted
//  by other means, all accessors and modifiers are made public (though
//  only the SurfaceFactory modifies an instance).
//
class SurfaceData {
public:
    SurfaceData();
    SurfaceData(SurfaceData const & src) { *this = src; }
    SurfaceData & operator=(SurfaceData const & src);
    ~SurfaceData() { invalidate(); }

public:
    //  Simple accessors used by both Surface and SurfaceFactory:
    typedef int Index;

    int           getNumCVs()    const { return (int)_cvIndices.GetSize(); }
    Index const * getCVIndices() const { return &_cvIndices[0]; }

    Parameterization  getParam()        const { return _param; }
    bool              isValid()         const { return _isValid; }
    bool              isDouble()        const { return _isDouble; }
    bool              isRegular()       const { return _isRegular; }
    bool              isLinear()        const { return _isLinear; }
    unsigned char     getRegPatchType() const { return _regPatchType; }
    unsigned char     getRegPatchMask() const { return _regPatchMask; }

    //  Local types and accessors for references to irregular patches:
    typedef internal::IrregularPatchType      IrregPatchType;
    typedef internal::IrregularPatchSharedPtr IrregPatchPtr;

    bool                   hasIrregPatch()    const { return _irregPatch != 0; }
    IrregPatchType const & getIrregPatch()    const { return *_irregPatch; }
    IrregPatchPtr          getIrregPatchPtr() const { return _irregPatch; }

public:
    //  Modifiers used by SurfaceFactory to assemble a Surface:
    void invalidate();
    void reinitialize() { if (isValid()) invalidate(); }

    Index * getCVIndices() { return &_cvIndices[0]; }
    Index * resizeCVs(int size) {
        _cvIndices.SetSize(size);
        return &_cvIndices[0];
    }

    void setParam(Parameterization p)   { _param = p; }
    void setValid(bool on)              { _isValid = on; }
    void setDouble(bool on)             { _isDouble = on; }
    void setRegular(bool on)            { _isRegular = on; }
    void setLinear(bool on)             { _isLinear = on; }
    void setRegPatchType(int t)         { _regPatchType = (unsigned char) t; }
    void setRegPatchMask(int m)         { _regPatchMask = (unsigned char) m; }

    void setIrregPatchPtr(IrregPatchPtr const & ptr) { _irregPatch = ptr; }

private:
    //  Member variables -- try to avoid redundancy and/or wasted space
    //  here as some may choose to cache all Surfaces of a mesh:
    typedef Vtr::internal::StackBuffer<Index,20,true> CVIndexArray;

    CVIndexArray _cvIndices;

    Parameterization _param;

    unsigned char _isValid   : 1;
    unsigned char _isDouble  : 1;
    unsigned char _isRegular : 1;
    unsigned char _isLinear  : 1;

    unsigned char _regPatchType;
    unsigned char _regPatchMask;

    IrregPatchPtr _irregPatch;
};

} // end namespace internal
} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_SURFACE_DATA */
