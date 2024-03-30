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

#include "../bfr/surfaceFactoryCache.h"
#include "../bfr/patchTree.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  Trivial constructor and destructor:
//
SurfaceFactoryCache::SurfaceFactoryCache() : _map() {
}

SurfaceFactoryCache::~SurfaceFactoryCache() {
    //  Potentially monitor usage on destruction
}


//
//  Internal methods to find and add map entries:
//
SurfaceFactoryCache::DataType
SurfaceFactoryCache::find(KeyType const & key) const {

    MapType::const_iterator itFound = _map.find(key);
    return (itFound != _map.end()) ? itFound->second : DataType(0);
}

SurfaceFactoryCache::DataType
SurfaceFactoryCache::add(KeyType const & key, DataType const & data) {

    MapType::const_iterator itFound = _map.find(key);
    return (itFound != _map.end()) ? itFound->second : (_map[key] = data);
}

//
//  Virtual method defaults -- intended to be overridden for thread-safety:
//
SurfaceFactoryCache::DataType
SurfaceFactoryCache::Find(KeyType const & key) const {

    return find(key);
}

SurfaceFactoryCache::DataType
SurfaceFactoryCache::Add(KeyType const & key, DataType const & data) {

    return add(key, data);
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv
