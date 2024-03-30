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

#ifndef OPENSUBDIV3_FAR_STENCILTABLE_H
#define OPENSUBDIV3_FAR_STENCILTABLE_H

#include "../version.h"

#include "../far/types.h"

#include <cassert>
#include <cstring>
#include <vector>
#include <iostream>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

//  Forward declarations for friends:
class PatchTableBuilder;

template <typename REAL> class StencilTableFactoryReal;
template <typename REAL> class LimitStencilTableFactoryReal;

/// \brief Vertex stencil descriptor
///
/// Allows access and manipulation of a single stencil in a StencilTable.
///
template <typename REAL>
class StencilReal {
public:

    /// \brief Default constructor
    StencilReal() {}

    /// \brief Constructor
    ///
    /// @param size     Table pointer to the size of the stencil
    ///
    /// @param indices  Table pointer to the vertex indices of the stencil
    ///
    /// @param weights  Table pointer to the vertex weights of the stencil
    ///
    StencilReal(int * size, Index * indices, REAL * weights)
        : _size(size), _indices(indices), _weights(weights) { }

    /// \brief Copy constructor
    StencilReal(StencilReal const & other) {
        _size = other._size;
        _indices = other._indices;
        _weights = other._weights;
    }

    /// \brief Returns the size of the stencil
    int GetSize() const {
        return *_size;
    }

    /// \brief Returns the size of the stencil as a pointer
    int * GetSizePtr() const {
        return _size;
    }

    /// \brief Returns the control vertices' indices
    Index const * GetVertexIndices() const {
        return _indices;
    }

    /// \brief Returns the interpolation weights
    REAL const * GetWeights() const {
        return _weights;
    }

    /// \brief Advance to the next stencil in the table
    void Next() {
        int stride = *_size;
        ++_size;
        _indices += stride;
        _weights += stride;
    }

protected:
    friend class StencilTableFactoryReal<REAL>;
    friend class LimitStencilTableFactoryReal<REAL>;

    int * _size;
    Index * _indices;
    REAL  * _weights;
};

/// \brief Vertex stencil class wrapping the template for compatibility.
///
class Stencil : public StencilReal<float> {
protected:
    typedef StencilReal<float>   BaseStencil;

public:
    Stencil() : BaseStencil() { }
    Stencil(BaseStencil const & other) : BaseStencil(other) { }
    Stencil(int * size, Index * indices, float * weights)
        : BaseStencil(size, indices, weights) { }
};


/// \brief Table of subdivision stencils.
///
/// Stencils are the most direct method of evaluation of locations on the limit
/// of a surface. Every point of a limit surface can be computed by linearly
/// blending a collection of coarse control vertices.
///
/// A stencil assigns a series of control vertex indices with a blending weight
/// that corresponds to a unique parametric location of the limit surface. When
/// the control vertices move in space, the limit location can be very efficiently
/// recomputed simply by applying the blending weights to the series of coarse
/// control vertices.
///
template <typename REAL>
class StencilTableReal {
protected:
    StencilTableReal(int numControlVerts,
                    std::vector<int> const& offsets,
                    std::vector<int> const& sizes,
                    std::vector<int> const& sources,
                    std::vector<REAL> const& weights,
                    bool includeCoarseVerts,
                    size_t firstOffset);

public:

    virtual ~StencilTableReal() {};
    
    /// \brief Returns the number of stencils in the table
    int GetNumStencils() const {
        return (int)_sizes.size();
    }

    /// \brief Returns the number of control vertices indexed in the table
    int GetNumControlVertices() const {
        return _numControlVertices;
    }

    /// \brief Returns a Stencil at index i in the table
    StencilReal<REAL> GetStencil(Index i) const;

    /// \brief Returns the number of control vertices of each stencil in the table
    std::vector<int> const & GetSizes() const {
        return _sizes;
    }

    /// \brief Returns the offset to a given stencil (factory may leave empty)
    std::vector<Index> const & GetOffsets() const {
        return _offsets;
    }

    /// \brief Returns the indices of the control vertices
    std::vector<Index> const & GetControlIndices() const {
        return _indices;
    }

    /// \brief Returns the stencil interpolation weights
    std::vector<REAL> const & GetWeights() const {
        return _weights;
    }

    /// \brief Returns the stencil at index i in the table
    StencilReal<REAL> operator[] (Index index) const;

    /// \brief Updates point values based on the control values
    ///
    /// \note The destination buffers are assumed to have allocated at least
    ///       \c GetNumStencils() elements.
    ///
    /// @param srcValues  Buffer with primvar data for the control vertices
    ///
    /// @param dstValues  Destination buffer for the interpolated primvar data
    ///
    /// @param start      Index of first destination value to update
    ///
    /// @param end        Index of last destination value to update
    ///
    template <class T, class U>
    void UpdateValues(T const &srcValues, U &dstValues, Index start=-1, Index end=-1) const {
        this->update(srcValues, dstValues, _weights, start, end);
    }

    template <class T1, class T2, class U>
    void UpdateValues(T1 const &srcBase, int numBase, T2 const &srcRef,
        U &dstValues, Index start=-1, Index end=-1) const {
        this->update(srcBase, numBase, srcRef, dstValues, _weights, start, end);
    }

    //  Pointer interface for backward compatibility
    template <class T, class U>
    void UpdateValues(T const *src, U *dst, Index start=-1, Index end=-1) const {
        this->update(src, dst, _weights, start, end);
    }
    template <class T1, class T2, class U>
    void UpdateValues(T1 const *srcBase, int numBase, T2 const *srcRef,
        U *dst, Index start=-1, Index end=-1) const {
        this->update(srcBase, numBase, srcRef, dst, _weights, start, end);
    }

    /// \brief Clears the stencils from the table
    void Clear();

protected:

    // Update values by applying cached stencil weights to new control values
    template <class T, class U>
    void update( T const &srcValues, U &dstValues,
        std::vector<REAL> const & valueWeights, Index start, Index end) const;
    template <class T1, class T2, class U>
    void update( T1 const &srcBase, int numBase, T2 const &srcRef, U &dstValues,
        std::vector<REAL> const & valueWeights, Index start, Index end) const;

    // Populate the offsets table from the stencil sizes in _sizes (factory helper)
    void generateOffsets();

    // Resize the table arrays (factory helper)
    void resize(int nstencils, int nelems);

    // Reserves the table arrays (factory helper)
    void reserve(int nstencils, int nelems);

    // Reallocates the table arrays to remove excess capacity (factory helper)
    void shrinkToFit();

    // Performs any final operations on internal tables (factory helper)
    void finalize();

protected:
    StencilTableReal() : _numControlVertices(0) {}
    StencilTableReal(int numControlVerts)
        : _numControlVertices(numControlVerts) 
    { }

    friend class StencilTableFactoryReal<REAL>;
    friend class Far::PatchTableBuilder;

    int _numControlVertices;              // number of control vertices

    std::vector<int>           _sizes;    // number of coefficients for each stencil
    std::vector<Index>         _offsets,  // offset to the start of each stencil
                               _indices;  // indices of contributing coarse vertices
    std::vector<REAL>         _weights;  // stencil weight coefficients
};

/// \brief Stencil table class wrapping the template for compatibility.
///
class StencilTable : public StencilTableReal<float> {
protected:
    typedef StencilTableReal<float>   BaseTable;

public:
    Stencil GetStencil(Index index) const {
        return Stencil(BaseTable::GetStencil(index));
    }
    Stencil operator[] (Index index) const {
        return Stencil(BaseTable::GetStencil(index));
    }

protected:
    StencilTable() : BaseTable() { }
    StencilTable(int numControlVerts) : BaseTable(numControlVerts) { }
    StencilTable(int numControlVerts,
                 std::vector<int> const& offsets,
                 std::vector<int> const& sizes,
                 std::vector<int> const& sources,
                 std::vector<float> const& weights,
                 bool includeCoarseVerts,
                 size_t firstOffset)
        : BaseTable(numControlVerts, offsets,
                sizes, sources, weights, includeCoarseVerts, firstOffset) { }
};


/// \brief Limit point stencil descriptor
///
template <typename REAL>
class LimitStencilReal : public StencilReal<REAL> {
public:

    /// \brief Constructor
    ///
    /// @param size       Table pointer to the size of the stencil
    ///
    /// @param indices    Table pointer to the vertex indices of the stencil
    ///
    /// @param weights    Table pointer to the vertex weights of the stencil
    ///
    /// @param duWeights  Table pointer to the 'u' derivative weights
    ///
    /// @param dvWeights  Table pointer to the 'v' derivative weights
    ///
    /// @param duuWeights Table pointer to the 'uu' derivative weights
    ///
    /// @param duvWeights Table pointer to the 'uv' derivative weights
    ///
    /// @param dvvWeights Table pointer to the 'vv' derivative weights
    ///
    LimitStencilReal( int* size,
                      Index * indices,
                      REAL * weights,
                      REAL * duWeights=0,
                      REAL * dvWeights=0,
                      REAL * duuWeights=0,
                      REAL * duvWeights=0,
                      REAL * dvvWeights=0)
        : StencilReal<REAL>(size, indices, weights),
          _duWeights(duWeights),
          _dvWeights(dvWeights),
          _duuWeights(duuWeights),
          _duvWeights(duvWeights),
          _dvvWeights(dvvWeights) {
    }

    /// \brief Returns the u derivative weights
    REAL const * GetDuWeights() const {
        return _duWeights;
    }

    /// \brief Returns the v derivative weights
    REAL const * GetDvWeights() const {
        return _dvWeights;
    }

    /// \brief Returns the uu derivative weights
    REAL const * GetDuuWeights() const {
        return _duuWeights;
    }

    /// \brief Returns the uv derivative weights
    REAL const * GetDuvWeights() const {
        return _duvWeights;
    }

    /// \brief Returns the vv derivative weights
    REAL const * GetDvvWeights() const {
        return _dvvWeights;
    }

    /// \brief Advance to the next stencil in the table
    void Next() {
       int stride = *this->_size;
       ++this->_size;
       this->_indices += stride;
       this->_weights += stride;
       if (_duWeights) _duWeights += stride;
       if (_dvWeights) _dvWeights += stride;
       if (_duuWeights) _duuWeights += stride;
       if (_duvWeights) _duvWeights += stride;
       if (_dvvWeights) _dvvWeights += stride;
    }

private:

    friend class StencilTableFactoryReal<REAL>;
    friend class LimitStencilTableFactoryReal<REAL>;

    REAL  * _duWeights,  // pointer to stencil u derivative limit weights
          * _dvWeights,  // pointer to stencil v derivative limit weights
          * _duuWeights, // pointer to stencil uu derivative limit weights
          * _duvWeights, // pointer to stencil uv derivative limit weights
          * _dvvWeights; // pointer to stencil vv derivative limit weights
};

/// \brief Limit point stencil class wrapping the template for compatibility.
///
class LimitStencil : public LimitStencilReal<float> {
protected:
    typedef LimitStencilReal<float>   BaseStencil;

public:
    LimitStencil(BaseStencil const & other) : BaseStencil(other) { }
    LimitStencil(int* size, Index * indices, float * weights,
                 float * duWeights=0, float * dvWeights=0,
                 float * duuWeights=0, float * duvWeights=0, float * dvvWeights=0)
        : BaseStencil(size, indices, weights,
                 duWeights, dvWeights, duuWeights, duvWeights, dvvWeights) { }
};


/// \brief Table of limit subdivision stencils.
///
template <typename REAL>
class LimitStencilTableReal : public StencilTableReal<REAL> {
protected:
    LimitStencilTableReal(
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
                    size_t firstOffset);

public:

    /// \brief Returns a LimitStencil at index i in the table
    LimitStencilReal<REAL> GetLimitStencil(Index i) const;

    /// \brief Returns the limit stencil at index i in the table
    LimitStencilReal<REAL> operator[] (Index index) const;

    /// \brief Returns the 'u' derivative stencil interpolation weights
    std::vector<REAL> const & GetDuWeights() const {
        return _duWeights;
    }

    /// \brief Returns the 'v' derivative stencil interpolation weights
    std::vector<REAL> const & GetDvWeights() const {
        return _dvWeights;
    }

    /// \brief Returns the 'uu' derivative stencil interpolation weights
    std::vector<REAL> const & GetDuuWeights() const {
        return _duuWeights;
    }

    /// \brief Returns the 'uv' derivative stencil interpolation weights
    std::vector<REAL> const & GetDuvWeights() const {
        return _duvWeights;
    }

    /// \brief Returns the 'vv' derivative stencil interpolation weights
    std::vector<REAL> const & GetDvvWeights() const {
        return _dvvWeights;
    }

    /// \brief Updates derivative values based on the control values
    ///
    /// \note The destination buffers ('uderivs' & 'vderivs') are assumed to
    ///       have allocated at least \c GetNumStencils() elements.
    ///
    /// @param srcValues  Buffer with primvar data for the control vertices
    ///
    /// @param uderivs    Destination buffer for the interpolated 'u'
    ///                   derivative primvar data
    ///
    /// @param vderivs    Destination buffer for the interpolated 'v'
    ///                   derivative primvar data
    ///
    /// @param start      Index of first destination derivative to update
    ///
    /// @param end        Index of last destination derivative to update
    ///
    template <class T, class U>
    void UpdateDerivs(T const & srcValues, U & uderivs, U & vderivs,
        int start=-1, int end=-1) const {

        this->update(srcValues, uderivs, _duWeights, start, end);
        this->update(srcValues, vderivs, _dvWeights, start, end);
    }

    template <class T1, class T2, class U>
    void UpdateDerivs(T1 const & srcBase, int numBase, T2 const & srcRef,
        U & uderivs, U & vderivs, int start=-1, int end=-1) const {

        this->update(srcBase, numBase, srcRef, uderivs, _duWeights, start, end);
        this->update(srcBase, numBase, srcRef, vderivs, _dvWeights, start, end);
    }

    //  Pointer interface for backward compatibility
    template <class T, class U>
    void UpdateDerivs(T const *src, U *uderivs, U *vderivs,
        int start=-1, int end=-1) const {

        this->update(src, uderivs, _duWeights, start, end);
        this->update(src, vderivs, _dvWeights, start, end);
    }
    template <class T1, class T2, class U>
    void UpdateDerivs(T1 const *srcBase, int numBase, T2 const *srcRef,
        U *uderivs, U *vderivs, int start=-1, int end=-1) const {

        this->update(srcBase, numBase, srcRef, uderivs, _duWeights, start, end);
        this->update(srcBase, numBase, srcRef, vderivs, _dvWeights, start, end);
    }

    /// \brief Updates 2nd derivative values based on the control values
    ///
    /// \note The destination buffers ('uuderivs', 'uvderivs', & 'vderivs') are
    ///       assumed to have allocated at least \c GetNumStencils() elements.
    ///
    /// @param srcValues  Buffer with primvar data for the control vertices
    ///
    /// @param uuderivs   Destination buffer for the interpolated 'uu'
    ///                   derivative primvar data
    ///
    /// @param uvderivs   Destination buffer for the interpolated 'uv'
    ///                   derivative primvar data
    ///
    /// @param vvderivs   Destination buffer for the interpolated 'vv'
    ///                   derivative primvar data
    ///
    /// @param start      Index of first destination derivative to update
    ///
    /// @param end        Index of last destination derivative to update
    ///
    template <class T, class U>
    void Update2ndDerivs(T const & srcValues,
        U & uuderivs, U & uvderivs, U & vvderivs,
        int start=-1, int end=-1) const {

        this->update(srcValues, uuderivs, _duuWeights, start, end);
        this->update(srcValues, uvderivs, _duvWeights, start, end);
        this->update(srcValues, vvderivs, _dvvWeights, start, end);
    }

    template <class T1, class T2, class U>
    void Update2ndDerivs(T1 const & srcBase, int numBase, T2 const & srcRef,
        U & uuderivs, U & uvderivs, U & vvderivs, int start=-1, int end=-1) const {

        this->update(srcBase, numBase, srcRef, uuderivs, _duuWeights, start, end);
        this->update(srcBase, numBase, srcRef, uvderivs, _duvWeights, start, end);
        this->update(srcBase, numBase, srcRef, vvderivs, _dvvWeights, start, end);
    }

    //  Pointer interface for backward compatibility
    template <class T, class U>
    void Update2ndDerivs(T const *src, T *uuderivs, U *uvderivs, U *vvderivs,
        int start=-1, int end=-1) const {

        this->update(src, uuderivs, _duuWeights, start, end);
        this->update(src, uvderivs, _duvWeights, start, end);
        this->update(src, vvderivs, _dvvWeights, start, end);
    }
    template <class T1, class T2, class U>
    void Update2ndDerivs(T1 const *srcBase, int numBase, T2 const *srcRef,
        U *uuderivs, U *uvderivs, U *vvderivs, int start=-1, int end=-1) const {

        this->update(srcBase, numBase, srcRef, uuderivs, _duuWeights, start, end);
        this->update(srcBase, numBase, srcRef, uvderivs, _duvWeights, start, end);
        this->update(srcBase, numBase, srcRef, vvderivs, _dvvWeights, start, end);
    }

    /// \brief Clears the stencils from the table
    void Clear();

private:
    friend class LimitStencilTableFactoryReal<REAL>;

    // Resize the table arrays (factory helper)
    void resize(int nstencils, int nelems);

private:
    std::vector<REAL>   _duWeights,   // u  derivative limit stencil weights
                        _dvWeights,   // v  derivative limit stencil weights
                        _duuWeights,  // uu derivative limit stencil weights
                        _duvWeights,  // uv derivative limit stencil weights
                        _dvvWeights;  // vv derivative limit stencil weights
};

/// \brief Limit stencil table class wrapping the template for compatibility.
///
class LimitStencilTable : public LimitStencilTableReal<float> {
protected:
    typedef LimitStencilTableReal<float>   BaseTable;

public:
    LimitStencil GetLimitStencil(Index index) const {
        return LimitStencil(BaseTable::GetLimitStencil(index));
    }
    LimitStencil operator[] (Index index) const {
        return LimitStencil(BaseTable::GetLimitStencil(index));
    }

protected:
    LimitStencilTable(int numControlVerts,
                    std::vector<int> const& offsets,
                    std::vector<int> const& sizes,
                    std::vector<int> const& sources,
                    std::vector<float> const& weights,
                    std::vector<float> const& duWeights,
                    std::vector<float> const& dvWeights,
                    std::vector<float> const& duuWeights,
                    std::vector<float> const& duvWeights,
                    std::vector<float> const& dvvWeights,
                    bool includeCoarseVerts,
                    size_t firstOffset)
        : BaseTable(numControlVerts,
                    offsets, sizes, sources, weights,
                    duWeights, dvWeights, duuWeights, duvWeights, dvvWeights,
                    includeCoarseVerts, firstOffset) { }
};


// Update values by applying cached stencil weights to new control values
template <typename REAL>
template <class T1, class T2, class U> void
StencilTableReal<REAL>::update(T1 const &srcBase, int numBase,
    T2 const &srcRef, U &dstValues,
    std::vector<REAL> const &valueWeights, Index start, Index end) const {

    int const * sizes = &_sizes.at(0);
    Index const * indices = &_indices.at(0);
    REAL const * weights = &valueWeights.at(0);

    if (start > 0) {
        assert(start < (Index)_offsets.size());
        sizes += start;
        indices += _offsets[start];
        weights += _offsets[start];
    } else {
        start = 0;
    }

    int nstencils = ((end < start) ? GetNumStencils() : end) - start;

    for (int i = 0; i < nstencils; ++i, ++sizes) {
        dstValues[start + i].Clear();
        for (int j = 0; j < *sizes; ++j, ++indices, ++weights) {
            if (*indices < numBase) {
                dstValues[start + i].AddWithWeight(srcBase[*indices], *weights);
            } else {
                dstValues[start + i].AddWithWeight(srcRef[*indices - numBase], *weights);
            }
        }
    }
}
template <typename REAL>
template <class T, class U> void
StencilTableReal<REAL>::update(T const &srcValues, U &dstValues,
    std::vector<REAL> const &valueWeights, Index start, Index end) const {

    int const * sizes = &_sizes.at(0);
    Index const * indices = &_indices.at(0);
    REAL const * weights = &valueWeights.at(0);

    if (start > 0) {
        assert(start < (Index)_offsets.size());
        sizes += start;
        indices += _offsets[start];
        weights += _offsets[start];
    } else {
        start = 0;
    }

    int nstencils = ((end < start) ? GetNumStencils() : end) - start;

    for (int i = 0; i < nstencils; ++i, ++sizes) {
        dstValues[start + i].Clear();
        for (int j = 0; j < *sizes; ++j, ++indices, ++weights) {
            dstValues[start + i].AddWithWeight(srcValues[*indices], *weights);
        }
    }
}

template <typename REAL>
inline void
StencilTableReal<REAL>::generateOffsets() {
    Index offset=0;
    int noffsets = (int)_sizes.size();
    _offsets.resize(noffsets);
    for (int i=0; i<(int)_sizes.size(); ++i ) {
        _offsets[i]=offset;
        offset+=_sizes[i];
    }
}

template <typename REAL>
inline void
StencilTableReal<REAL>::resize(int nstencils, int nelems) {
    _sizes.resize(nstencils);
    _indices.resize(nelems);
    _weights.resize(nelems);
}

template <typename REAL>
inline void
StencilTableReal<REAL>::reserve(int nstencils, int nelems) {
    _sizes.reserve(nstencils);
    _indices.reserve(nelems);
    _weights.reserve(nelems);
}

template <typename REAL>
inline void
StencilTableReal<REAL>::shrinkToFit() {
    std::vector<int>(_sizes).swap(_sizes);
    std::vector<Index>(_indices).swap(_indices);
    std::vector<REAL>(_weights).swap(_weights);
}

template <typename REAL>
inline void
StencilTableReal<REAL>::finalize() {
    shrinkToFit();
    generateOffsets();
}

// Returns a Stencil at index i in the table
template <typename REAL>
inline StencilReal<REAL>
StencilTableReal<REAL>::GetStencil(Index i) const {
    assert((! _offsets.empty()) && i<(int)_offsets.size());

    Index ofs = _offsets[i];

    return StencilReal<REAL>(const_cast<int*>(&_sizes[i]),
                             const_cast<Index*>(&_indices[ofs]),
                             const_cast<REAL*>(&_weights[ofs]));
}

template <typename REAL>
inline StencilReal<REAL>
StencilTableReal<REAL>::operator[] (Index index) const {
    return GetStencil(index);
}

template <typename REAL>
inline void
LimitStencilTableReal<REAL>::resize(int nstencils, int nelems) {
    StencilTableReal<REAL>::resize(nstencils, nelems);
    _duWeights.resize(nelems);
    _dvWeights.resize(nelems);
}

// Returns a LimitStencil at index i in the table
template <typename REAL>
inline LimitStencilReal<REAL>
LimitStencilTableReal<REAL>::GetLimitStencil(Index i) const {
    assert((! this->GetOffsets().empty()) && i<(int)this->GetOffsets().size());

    Index ofs = this->GetOffsets()[i];

    if (!_duWeights.empty() && !_dvWeights.empty() &&
        !_duuWeights.empty() && !_duvWeights.empty() && !_dvvWeights.empty()) {
        return LimitStencilReal<REAL>(
                             const_cast<int *>(&this->GetSizes()[i]),
                             const_cast<Index *>(&this->GetControlIndices()[ofs]),
                             const_cast<REAL *>(&this->GetWeights()[ofs]),
                             const_cast<REAL *>(&GetDuWeights()[ofs]),
                             const_cast<REAL *>(&GetDvWeights()[ofs]),
                             const_cast<REAL *>(&GetDuuWeights()[ofs]),
                             const_cast<REAL *>(&GetDuvWeights()[ofs]),
                             const_cast<REAL *>(&GetDvvWeights()[ofs]) );
    } else if (!_duWeights.empty() && !_dvWeights.empty()) {
        return LimitStencilReal<REAL>(
                             const_cast<int *>(&this->GetSizes()[i]),
                             const_cast<Index *>(&this->GetControlIndices()[ofs]),
                             const_cast<REAL *>(&this->GetWeights()[ofs]),
                             const_cast<REAL *>(&GetDuWeights()[ofs]),
                             const_cast<REAL *>(&GetDvWeights()[ofs]) );
    } else {
        return LimitStencilReal<REAL>(
                             const_cast<int *>(&this->GetSizes()[i]),
                             const_cast<Index *>(&this->GetControlIndices()[ofs]),
                             const_cast<REAL *>(&this->GetWeights()[ofs]) );
    }
}

template <typename REAL>
inline LimitStencilReal<REAL>
LimitStencilTableReal<REAL>::operator[] (Index index) const {
    return GetLimitStencil(index);
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif // OPENSUBDIV3_FAR_STENCILTABLE_H
