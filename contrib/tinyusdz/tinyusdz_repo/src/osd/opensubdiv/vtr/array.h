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
#ifndef OPENSUBDIV3_VTR_ARRAY_INTERFACE_H
#define OPENSUBDIV3_VTR_ARRAY_INTERFACE_H

#include "../version.h"

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {

//
//  This class provides a simple array-like interface -- a subset std::vector's interface -- for
//  a sequence of elements stored in contiguous memory.  It provides a unified representation for
//  referencing data on the stack, all or a subset of std::vector<>, or anywhere else in memory.
//
//  Note that its members are head/size rather than begin/end as in std::vector -- we frequently
//  need only the size for many queries, and that is most often what is stored elsewhere in other
//  classes, so we hope to reduce unnecessary address arithmetic constructing the interface and
//  accessing the size.  The size type is also specifically 32-bit (rather than size_t) to match
//  internal usage and avoid unnecessary conversion to/from 64-bit.
//
//  Question:
//      Naming is at issue here...  formerly called ArrayInterface until that was shot down it has
//  been simplified to Array but needs to be distanced from std::array as it DOES NOT store its
//  own memory and is simply an interface to memory stored elsewhere.
//
template <typename TYPE>
class ConstArray {

public:
    typedef TYPE value_type;
    typedef int  size_type;

    typedef TYPE const& const_reference;
    typedef TYPE const* const_iterator;

    typedef TYPE& reference;
    typedef TYPE* iterator;

public:

    ConstArray() : _begin(0), _size(0) { }

    ConstArray(value_type const * ptr, size_type sizeArg) :
        _begin(ptr), _size(sizeArg) { }

    size_type size() const { return _size; }

    bool empty() const { return _size==0; }

    const_reference operator[](int index) const { return _begin[index]; }
    const_iterator  begin() const               { return _begin; }
    const_iterator  end() const                 { return _begin + _size; }

    size_type FindIndexIn4Tuple(value_type value) const {
        assert(_size>=4);
        if (value == _begin[0]) return 0;
        if (value == _begin[1]) return 1;
        if (value == _begin[2]) return 2;
        if (value == _begin[3]) return 3;
        assert("FindIndexIn4Tuple() did not find expected value!" == 0);
        return -1;
    }

    size_type FindIndex(value_type value) const {
        for (size_type i=0; i<size(); ++i) {
            if (value==_begin[i]) {
                return i;
            }
        }
        return -1;
    }

protected:
    value_type const * _begin;
    size_type _size;
};

template <typename TYPE>
class Array : public ConstArray<TYPE> {

public:
    typedef TYPE value_type;
    typedef int  size_type;

    typedef TYPE const& const_reference;

    typedef TYPE& reference;
    typedef TYPE* iterator;

public:

    Array() : ConstArray<TYPE>() { }
    
    Array(value_type * ptr, size_type sizeArg) : ConstArray<TYPE>(ptr, sizeArg) { }

public:

    const_reference operator[](int index) const {
        return ConstArray<TYPE>::_begin[index];
    }

    reference operator[](int index) {
        return const_cast<reference>(ConstArray<TYPE>::_begin[index]);
    }

    iterator begin() {
        return const_cast<iterator>(ConstArray<TYPE>::_begin);
    }

    iterator end() {
        return const_cast<iterator>(ConstArray<TYPE>::_begin +
            ConstArray<TYPE>::_size);
    }
};

} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_VTR_ARRAY_INTERFACE_H */
