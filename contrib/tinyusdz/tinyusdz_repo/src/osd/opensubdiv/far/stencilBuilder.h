//
//   Copyright 2015 Pixar
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

#ifndef OPENSUBDIV3_FAR_STENCILBUILDER_H
#define OPENSUBDIV3_FAR_STENCILBUILDER_H

#include <vector>

#include "../version.h"
#include "../far/stencilTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {
namespace internal {

template <typename REAL> class WeightTable;

template <typename REAL>
class StencilBuilder {
public:
    StencilBuilder(int coarseVertCount, 
                   bool genCtrlVertStencils=true,
                   bool compactWeights=true);
    ~StencilBuilder();

    // TODO: noncopyable.

    size_t GetNumVerticesTotal() const;

    int GetNumVertsInStencil(size_t stencilIndex) const;

    void SetCoarseVertCount(int numVerts);

    // Mapping from stencil[i] to its starting offset in the sources[] and weights[] arrays;
    std::vector<int> const& GetStencilOffsets() const;

    // The number of contributing sources and weights in stencil[i]
    std::vector<int> const& GetStencilSizes() const;

    // The absolute source vertex offsets.
    std::vector<int> const& GetStencilSources() const;

    // The individual vertex weights, each weight is paired with one source.
    std::vector<REAL> const& GetStencilWeights() const;
    std::vector<REAL> const& GetStencilDuWeights() const;
    std::vector<REAL> const& GetStencilDvWeights() const;
    std::vector<REAL> const& GetStencilDuuWeights() const;
    std::vector<REAL> const& GetStencilDuvWeights() const;
    std::vector<REAL> const& GetStencilDvvWeights() const;

    // Vertex Facade.
    class Index {
    public:
        Index(StencilBuilder* owner, int index) 
            : _owner(owner)
            , _index(index)
        {}

        // Add with point/vertex weight only.
        void AddWithWeight(Index const & src, REAL weight);
        void AddWithWeight(StencilReal<REAL> const& src, REAL weight);

        // Add with first derivative.
        void AddWithWeight(StencilReal<REAL> const& src,
            REAL weight, REAL du, REAL dv);

        // Add with first and second derivatives.
        void AddWithWeight(StencilReal<REAL> const& src,
            REAL weight, REAL du, REAL dv, REAL duu, REAL duv, REAL dvv);

        Index operator[](int index) const {
            return Index(_owner, index+_index);
        }

        int GetOffset() const { return _index; }

        void Clear() {/*nothing to do here*/}
    private:
        StencilBuilder* _owner;
        int _index;
    };

private:
    WeightTable<REAL>* _weightTable;
};

} // end namespace internal
} // end namespace Far
} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv

#endif // FAR_STENCILBUILDER_H
