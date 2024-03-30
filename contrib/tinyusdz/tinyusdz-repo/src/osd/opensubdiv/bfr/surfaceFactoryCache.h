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

#ifndef OPENSUBDIV3_BFR_SURFACE_FACTORY_CACHE_H
#define OPENSUBDIV3_BFR_SURFACE_FACTORY_CACHE_H

#include "../version.h"

#include "../bfr/irregularPatchType.h"

#include <map>
#include <cstdint>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

///
/// @brief Container used internally by SurfaceFactory to store reusable
///        information
///
/// SurfaceFactoryCache is a container for storing/caching instances of
/// the internal representation of complex patches used by SurfaceFactory
/// so that they can be quickly identified and retrieved for reuse.
///
/// It is intended for internal use by SurfaceFactory.  Public access is
/// available but limited to construction only -- allowing an instance to
/// be reused by assigning it to more than one SurfaceFactory.
///
//
//  Initial/expected use requires simple searches of and additions to the
//  cache by the SurfaceFactory or its Builders.  Longer term, with the
//  possibility of instances of caches being shared between meshes and
//  factories, additional options and/or public methods may be warranted
//  to limit what is cached or to prune the cache if it gets too large.
//
class SurfaceFactoryCache {
public:
    SurfaceFactoryCache();
    virtual ~SurfaceFactoryCache();

    SurfaceFactoryCache(SurfaceFactoryCache const &) = delete;
    SurfaceFactoryCache & operator=(SurfaceFactoryCache const &) = delete;

protected:
    /// @cond PROTECTED
    //  Access restricted to the Factory, its Builders, etc.
    friend class SurfaceFactory;

    typedef std::uint64_t                     KeyType;
    typedef internal::IrregularPatchSharedPtr DataType;
    /// @endcond PROTECTED

protected:
    /// @cond PROTECTED
    size_t Size() const { return _map.size(); }

    //
    //  Potential overrides by subclasses for thread-safety:
    //
    virtual DataType Find(KeyType const & key) const;
    virtual DataType Add(KeyType const & key, DataType const & data);

    //
    //  Common implementation used by all subclasses:
    //
    DataType find(KeyType const & key) const;
    DataType add(KeyType const & key, DataType const & data);
    /// @endcond PROTECTED

private:
    typedef std::map<KeyType, DataType> MapType;

    MapType _map;
};

///
/// @brief Template for declaring thread-safe subclasses of SurfaceFactoryCache
///
/// SurfaceFactoryCacheThreaded extends SurfaceFactoryCache by protecting
/// access to the cache to ensure thread-safe operation. A mutex type and
/// associated locks are specified to declare a subclass with appropriately
/// protected read and write access.
///
/// @tparam MUTEX_TYPE            A mutex type with supported lock guards
/// @tparam READ_LOCK_GUARD_TYPE  A scoped lock guard allowing potentially
///                               shared access for read operations.
/// @tparam WRITE_LOCK_GUARD_TYPE A scoped lock guard allowing exclusive
///                               access for write operations.
///
//  Separate read and write locks are provided to support mutex types
//  allowing shared (read) or exclusive (write) access.
// 
template <class MUTEX_TYPE, class READ_LOCK_GUARD_TYPE,
                            class WRITE_LOCK_GUARD_TYPE>
class SurfaceFactoryCacheThreaded : public SurfaceFactoryCache {
public:
    SurfaceFactoryCacheThreaded() : SurfaceFactoryCache() { }
    ~SurfaceFactoryCacheThreaded() override = default;

protected:
    /// @cond PROTECTED
    //
    //  Virtual overrides from base:
    //
    DataType Find(KeyType const & key) const override {
        READ_LOCK_GUARD_TYPE lockGuard(_mutex);
        return find(key);
    }

    DataType Add(KeyType const & key, DataType const & data) override {
        WRITE_LOCK_GUARD_TYPE lockGuard(_mutex);
        return add(key, data);
    }
    /// @endcond PROTECTED

private:
    MUTEX_TYPE mutable _mutex;
};

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_SURFACE_FACTORY_CACHE_H */
