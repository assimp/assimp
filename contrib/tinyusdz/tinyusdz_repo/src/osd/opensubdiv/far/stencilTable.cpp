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

#include "../version.h"
#include "../far/stencilTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {


namespace {
    template <typename REAL>
    void
    copyStencilData(int numControlVerts,
                    bool includeCoarseVerts,
                    size_t firstOffset,
                    std::vector<int> const*   offsets,
                    std::vector<int> *       _offsets,
                    std::vector<int> const*   sizes,
                    std::vector<int> *       _sizes,
                    std::vector<int> const*   sources,
                    std::vector<int> *       _sources,
                    std::vector<REAL> const*  weights,
                    std::vector<REAL> *      _weights,
                    std::vector<REAL> const*  duWeights=NULL,
                    std::vector<REAL> *      _duWeights=NULL,
                    std::vector<REAL> const*  dvWeights=NULL,
                    std::vector<REAL> *      _dvWeights=NULL,
                    std::vector<REAL> const*  duuWeights=NULL,
                    std::vector<REAL> *      _duuWeights=NULL,
                    std::vector<REAL> const*  duvWeights=NULL,
                    std::vector<REAL> *      _duvWeights=NULL,
                    std::vector<REAL> const*  dvvWeights=NULL,
                    std::vector<REAL> *      _dvvWeights=NULL) {
        size_t start = includeCoarseVerts ? 0 : firstOffset;

        _offsets->resize(offsets->size());
        _sizes->resize(sizes->size());
        _sources->resize(sources->size());
        _weights->resize(weights->size());
        if (_duWeights)
            _duWeights->resize(duWeights->size());
        if (_dvWeights)
            _dvWeights->resize(dvWeights->size());
        if (_duuWeights)
            _duuWeights->resize(duuWeights->size());
        if (_duvWeights)
            _duvWeights->resize(duvWeights->size());
        if (_dvvWeights)
            _dvvWeights->resize(dvvWeights->size());

        // The stencils are probably not in order, so we must copy/sort them.
        // Note here that loop index 'i' represents stencil_i for vertex_i.
        int curOffset = 0;

        size_t stencilCount = 0,
               weightCount = 0;

        for ( size_t i=start; i<offsets->size(); i++ ) {
            // Once we've copied out all the control verts, jump to the offset
            // where the actual stencils begin.
            if (includeCoarseVerts && (int)i == numControlVerts)
                i = firstOffset;

            // Copy the stencil.
            int sz = (*sizes)[i];
            int off = (*offsets)[i];

            (*_offsets)[stencilCount] = curOffset;
            (*_sizes)[stencilCount] = sz;

            std::memcpy(&(*_sources)[curOffset],
                        &(*sources)[off], sz*sizeof(int));
            std::memcpy(&(*_weights)[curOffset],
                        &(*weights)[off], sz*sizeof(REAL));

            if (_duWeights && !_duWeights->empty()) {
                std::memcpy(&(*_duWeights)[curOffset],
                            &(*duWeights)[off], sz*sizeof(REAL));
            }
            if (_dvWeights && !_dvWeights->empty()) {
                std::memcpy(&(*_dvWeights)[curOffset],
                        &(*dvWeights)[off], sz*sizeof(REAL));
            }

            if (_duuWeights && !_duuWeights->empty()) {
                std::memcpy(&(*_duuWeights)[curOffset],
                        &(*duuWeights)[off], sz*sizeof(REAL));
            }
            if (_duvWeights && !_duvWeights->empty()) {
                std::memcpy(&(*_duvWeights)[curOffset],
                        &(*duvWeights)[off], sz*sizeof(REAL));
            }
            if (_dvvWeights && !_dvvWeights->empty()) {
                std::memcpy(&(*_dvvWeights)[curOffset],
                        &(*dvvWeights)[off], sz*sizeof(REAL));
            }

            curOffset += sz;
            stencilCount++;
            weightCount += sz;
        }

        _offsets->resize(stencilCount);
        _sizes->resize(stencilCount);
        _sources->resize(weightCount);

        if (_duWeights && !_duWeights->empty())
            _duWeights->resize(weightCount);
        if (_dvWeights && !_dvWeights->empty())
            _dvWeights->resize(weightCount);

        if (_duuWeights && !_duuWeights->empty())
            _duuWeights->resize(weightCount);
        if (_duvWeights && !_duvWeights->empty())
            _duvWeights->resize(weightCount);
        if (_dvvWeights && !_dvvWeights->empty())
            _dvvWeights->resize(weightCount);
    }
};

template <typename REAL>
StencilTableReal<REAL>::StencilTableReal(int numControlVerts,
                           std::vector<int> const& offsets,
                           std::vector<int> const& sizes,
                           std::vector<int> const& sources,
                           std::vector<REAL> const& weights,
                           bool includeCoarseVerts,
                           size_t firstOffset)
    : _numControlVertices(numControlVerts) {
    copyStencilData(numControlVerts,
                    includeCoarseVerts,
                    firstOffset,
                    &offsets, &_offsets,
                    &sizes, &_sizes,
                    &sources, &_indices,
                    &weights, &_weights);
}

template <typename REAL>
void
StencilTableReal<REAL>::Clear() {
    _numControlVertices=0;
    _sizes.clear();
    _offsets.clear();
    _indices.clear();
    _weights.clear();
}

template <typename REAL>
LimitStencilTableReal<REAL>::LimitStencilTableReal(
                                     int numControlVerts,
                                     std::vector<int> const& offsets,
                                     std::vector<int> const& sizes,
                                     std::vector<int> const& sources,
                                     std::vector<REAL> const& weights,
                                     std::vector<REAL> const& duWeights,
                                     std::vector<REAL> const& dvWeights,
                                     std::vector<REAL> const& duuWeights,
                                     std::vector<REAL> const& duvWeights,
                                     std::vector<REAL> const& dvvWeights,
                                     bool includeCoarseVerts,
                                     size_t firstOffset)
    : StencilTableReal<REAL>(numControlVerts) {
    copyStencilData(numControlVerts,
                    includeCoarseVerts,
                    firstOffset,
                    &offsets, &this->_offsets,
                    &sizes, &this->_sizes,
                    &sources, &this->_indices,
                    &weights, &this->_weights,
                    &duWeights, &_duWeights,
                    &dvWeights, &_dvWeights,
                    &duuWeights, &_duuWeights,
                    &duvWeights, &_duvWeights,
                    &dvvWeights, &_dvvWeights);
}

template <typename REAL>
void
LimitStencilTableReal<REAL>::Clear() {
    StencilTableReal<REAL>::Clear();
    _duWeights.clear();
    _dvWeights.clear();
    _duuWeights.clear();
    _duvWeights.clear();
    _dvvWeights.clear();
}


//
//  Explicit instantiation for float and double:
//
template class StencilReal<float>;
template class StencilReal<double>;

template class LimitStencilReal<float>;
template class LimitStencilReal<double>;

template class StencilTableReal<float>;
template class StencilTableReal<double>;

template class LimitStencilTableReal<float>;
template class LimitStencilTableReal<double>;

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv
