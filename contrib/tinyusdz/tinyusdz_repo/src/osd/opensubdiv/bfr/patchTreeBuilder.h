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

#ifndef OPENSUBDIV3_BFR_PATCH_TREE_BUILDER_H
#define OPENSUBDIV3_BFR_PATCH_TREE_BUILDER_H

#include "../version.h"

#include "../bfr/patchTree.h"
#include "../far/topologyRefiner.h"
#include "../far/patchBuilder.h"
#include "../far/sparseMatrix.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {
namespace Bfr {

//
//  The PatchTreeBuilder class assemble a PatchTree from one or more
//  topological descriptions -- keeping the PatchTree class free from
//  the dependencies of whatever its topological source may be.
//
//  Having been originally conceived as part of Far, the PatchTree is
//  assembled from a given Far::TopologyRefiner, which is constructed
//  for a local neighborhood by another class.  Some of the internal
//  details of PatchTree assembly also include complexity from that
//  history when a PatchTree could be constructed from any face of any
//  given TopologyRefiner -- which is no longer the case. The public
//  now prevents such unsupported flexibility, but internal support
//  remains.
//
class PatchTreeBuilder {
public:
    //
    //  Minimize the number of shape approximating Options here (compared
    //  to the Far classes):
    //
    //  Note that the "interior patches" capability of PatchTree is not
    //  used in Bfr and so is never enabled. It is left available as a
    //  reminder of that ability for future use.
    //
    struct Options {
        enum BasisType { REGULAR, GREGORY, LINEAR };

        Options(int depth = 4) : irregularBasis((unsigned char) GREGORY),
                                 maxPatchDepthSharp((unsigned char) depth),
                                 maxPatchDepthSmooth(15),
                                 includeInteriorPatches(false),
                                 useDoublePrecision(false) { }

        unsigned char irregularBasis;
        unsigned char maxPatchDepthSharp;
        unsigned char maxPatchDepthSmooth;
        unsigned char includeInteriorPatches : 1;
        unsigned char useDoublePrecision     : 1;
    };

public:
    //
    //  Public interface intended for use by other builders requiring
    //  PatchTrees -- now reduced essentially to a single method:
    //
    PatchTreeBuilder(Far::TopologyRefiner & refiner, Options const & options);
    ~PatchTreeBuilder();

    const PatchTree * Build();

    PatchTree * GetPatchTree() const { return _patchTree; }

private:
    //  Internal struct for a patch in the refinement hierarchy:
    struct PatchFace {
        PatchFace(int levelArg, int faceArg, bool isReg = true) :
                face(faceArg), level((short)levelArg), isRegular(isReg) { }

        int   face;
        short level;
        short isRegular;
    };

    //  Internal methods to identify and assemble patches and the tree:
    bool rootFaceNeedsRefinement() const;
    bool testFaceAncestors() const;
    bool faceAncestorIsRoot(int level, int face) const;

    void identifyPatches();
    void initializePatches();
    void initializeQuadTree();

    //  Internal methods to assemble the matrix of stencils converting
    //  points of irregular patches from points in the refined levels:
    template <typename REAL>
    void initializeStencilMatrix();

    template <typename REAL>
    void getIrregularPatchConversion(PatchFace const & patchFace,
                                     Far::SparseMatrix<REAL> & convMatrix,
                                     std::vector<Far::Index> & srcPoints);

    template <typename REAL>
    void appendConversionStencilsToMatrix(int stencilIndexBase,
                                     Far::SparseMatrix<REAL> const & convMatrix,
                                     std::vector<Far::Index> const & srcPoints);

private:
    //  The PatchTree instance being assembled:
    PatchTree * _patchTree;

    //  Member variables supporting its assembly:
    Far::TopologyRefiner &    _faceRefiner;
    Far::Index                _faceAtRoot;
    std::vector<int>          _levelOffsets;
    std::vector<PatchFace>    _patchFaces;
    Far::PatchBuilder *       _patchBuilder;
};

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_PATCH_TREE_BUILDER_H */
