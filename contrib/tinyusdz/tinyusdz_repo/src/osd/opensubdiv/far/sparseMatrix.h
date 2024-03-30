//
//   Copyright 2017 DreamWorks Animation LLC.
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

#ifndef OPENSUBDIV3_FAR_SPARSE_MATRIX_H
#define OPENSUBDIV3_FAR_SPARSE_MATRIX_H

#include "../version.h"

#include "../vtr/array.h"

#include <algorithm>


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

//
//  SparseMatrix
//
//  The SparseMatrix class is used by the PatchBuilder to store coefficients
//  for a set of patch points derived from some other set of points -- usually
//  the refined points in a subdivision level.  The compressed sparse row
//  format (CSR) is used as it provides us with stencils for points that
//  correspond to rows and so can be more directly and efficiently copied.
//
//  It has potential for other uses and so may eventually warrant a seperate
//  header file of its own.  For now, in keeping with the trend of exposing
//  classes only where used, it is defined with the PatchBuilder.
//
//  We may also want to explore the possibility of being able to assign
//  static buffers as members here -- allowing common matrices to be set
//  directly rather than repeatedly replicated.
//
template <typename REAL>
class SparseMatrix {
public:
    typedef int  column_type;
    typedef REAL element_type;

public:
    //  Declaration and access methods:
    SparseMatrix() : _numRows(0), _numColumns(0), _numElements(0) { }

    int GetNumRows() const { return _numRows; }
    int GetNumColumns() const { return _numColumns; }
    int GetNumElements() const { return _numElements; }
    int GetCapacity() const;

    int GetRowSize(int rowIndex) const {
        return _rowOffsets[rowIndex + 1] - _rowOffsets[rowIndex];
    }

    Vtr::ConstArray<column_type>  GetRowColumns( int rowIndex) const {
        return Vtr::ConstArray<column_type>(&_columns[_rowOffsets[rowIndex]],
                                            GetRowSize(rowIndex));
    }
    Vtr::ConstArray<element_type> GetRowElements(int rowIndex) const {
        return Vtr::ConstArray<element_type>(&_elements[_rowOffsets[rowIndex]],
                                             GetRowSize(rowIndex));
    }

    Vtr::ConstArray<column_type>  GetColumns() const {
        return Vtr::ConstArray<column_type>(&_columns[0], GetNumElements());
    }
    Vtr::ConstArray<element_type> GetElements() const {
        return Vtr::ConstArray<element_type>(&_elements[0], GetNumElements());
    }

public:
    //  Modification methods
    void Resize(int numRows, int numColumns, int numNonZeroEntriesToReserve);
    void Copy(SparseMatrix const & srcMatrix);
    void Swap(SparseMatrix & otherMatrix);

    void SetRowSize(int rowIndex, int size);

    Vtr::Array<column_type>  SetRowColumns( int rowIndex) {
        return Vtr::Array<column_type>(&_columns[_rowOffsets[rowIndex]],
                                       GetRowSize(rowIndex));
    }
    Vtr::Array<element_type> SetRowElements(int rowIndex) {
        return Vtr::Array<element_type>(&_elements[_rowOffsets[rowIndex]],
                                        GetRowSize(rowIndex));
    }

private:
    //  Simple dimensions:
    int _numRows;
    int _numColumns;
    int _numElements;

    std::vector<int> _rowOffsets;  // remember one more entry here than rows

    //  XXXX (barfowl) - Note that the use of std::vector for the columns and
    //  element arrays was causing performance issues in the incremental
    //  resizing of consecutive rows, so we've been exploring alternatives...
    std::vector<column_type>  _columns;
    std::vector<element_type> _elements;
};

template <typename REAL>
inline int
SparseMatrix<REAL>::GetCapacity() const {

    return (int) _elements.size();
}

template <typename REAL>
inline void
SparseMatrix<REAL>::Resize(int numRows, int numCols, int numElementsToReserve) {

    _numRows     = numRows;
    _numColumns  = numCols;
    _numElements = 0;

    _rowOffsets.resize(0);
    _rowOffsets.resize(_numRows + 1, -1);
    _rowOffsets[0] = 0;

    if (numElementsToReserve > GetCapacity()) {
        _columns.resize(numElementsToReserve);
        _elements.resize(numElementsToReserve);
    }
}
template <typename REAL>
inline void
SparseMatrix<REAL>::SetRowSize(int rowIndex, int rowSize) {

    assert(_rowOffsets[rowIndex] == _numElements);

    int & newVectorSize = _rowOffsets[rowIndex + 1];
    newVectorSize = _rowOffsets[rowIndex] + rowSize;

    _numElements = newVectorSize;
    if (newVectorSize > GetCapacity()) {
        _columns.resize(newVectorSize);
        _elements.resize(newVectorSize);
    }
}

template <typename REAL>
inline void
SparseMatrix<REAL>::Copy(SparseMatrix const & src) {

    _numRows    = src._numRows;
    _numColumns = src._numColumns;

    _rowOffsets = src._rowOffsets;

    _numElements = src._numElements;

    _columns  = src._columns;
    _elements = src._elements;
}

template <typename REAL>
inline void
SparseMatrix<REAL>::Swap(SparseMatrix & other) {

    std::swap(_numRows, other._numRows);
    std::swap(_numColumns, other._numColumns);
    std::swap(_numElements, other._numElements);

    _rowOffsets.swap(other._rowOffsets);
    _columns.swap(other._columns);
    _elements.swap(other._elements);
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_FAR_SPARSE_MATRIX_H */
