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

#ifndef OPENSUBDIV3_BFR_REFINER_SURFACE_FACTORY_H
#define OPENSUBDIV3_BFR_REFINER_SURFACE_FACTORY_H

#include "../version.h"

#include "../bfr/surfaceFactory.h"
#include "../bfr/surfaceFactoryCache.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {
    class TopologyRefiner;
}

namespace Bfr {

///
/// @brief Intermediate subclass of SurfaceFactory with Far::TopologyRefiner
///        as the mesh
///
/// RefinerSurfaceFactoryBase is an intermediate subclass of SurfaceFactory
/// using Far::TopologyRefiner as the connected mesh representation.
///
/// The SurfaceFactoryMeshAdapter interface for TopologyRefiner is provided
/// in full, along with some public extensions specific to TopologyRefiner.
///
/// Additional caching expectations of SurfaceFactory are NOT specified
/// here. These are deferred to subclasses to implement different behaviors
/// of the factory's internal caching. A template for such subclasses is 
/// additionally provided -- allowing clients desiring a thread-safe cache
/// to simply declare a subclass for a preferred thread-safe type.
///
class RefinerSurfaceFactoryBase : public SurfaceFactory {
public:
    //@{
    /// @name Construction and initialization
    ///
    /// Construction and initialization
    ///

    RefinerSurfaceFactoryBase(Far::TopologyRefiner const & mesh,
                              Options const & options);

    ~RefinerSurfaceFactoryBase() override = default;
    //@}

    //@{
    /// @name Simple queries related to Far::TopologyRefiner
    ///
    /// Simple queries related to Far::TopologyRefiner
    ///

    /// @brief Return the instance of the mesh
    Far::TopologyRefiner const & GetMesh() const { return _mesh; }

    /// @brief Return the number of faces
    int GetNumFaces() const { return _numFaces; }

    /// @brief Return the number of face-varying channels
    int GetNumFVarChannels() const { return _numFVarChannels; }
    //@}

protected:
    /// @cond PROTECTED
    //
    //  Virtual overrides to satisfy the SurfaceFactoryMeshAdapter interface:
    //
    bool isFaceHole( Index faceIndex) const override;
    int  getFaceSize(Index faceIndex) const override;

    int getFaceVertexIndices(Index faceIndex,
                        Index vertexIndices[]) const override;
    int getFaceFVarValueIndices(Index faceIndex,
                        FVarID fvarID, Index fvarValueIndices[]) const override;

    int populateFaceVertexDescriptor(Index faceIndex, int faceVertex,
                        VertexDescriptor * vertexDescriptor) const override;

    int getFaceVertexIncidentFaceVertexIndices(
                        Index faceIndex, int faceVertex,
                        Index vertexIndices[]) const override;
    int getFaceVertexIncidentFaceFVarValueIndices(
                        Index faceIndex, int faceVertex,
                        FVarID fvarID, Index fvarValueIndices[]) const override;

    //  Optional SurfaceFactoryMeshAdapter overrides for regular patches:
    bool getFaceNeighborhoodVertexIndicesIfRegular(
                        Index faceIndex,
                        Index vertexIndices[]) const override;

    bool getFaceNeighborhoodFVarValueIndicesIfRegular(
                        Index faceIndex,
                        FVarID fvarID, Index fvarValueIndices[]) const override;
    /// @endcond

private:
    //
    //  Internal supporting methods:
    //
    int getFaceVaryingChannel(FVarID fvarID) const;

    int getFaceVertexPointIndices(Index faceIndex, int faceVertex,
                                  Index indices[], int vtxOrFVarChannel) const;

    int getFacePatchPointIndices(Index faceIndex,
                                 Index indices[], int vtxOrFVarChannel) const;

private:
    //  Additional members for the subclass:
    Far::TopologyRefiner const & _mesh;

    int _numFaces;
    int _numFVarChannels;
};


//
/// @brief Template for concrete subclasses of RefinerSurfaceFactoryBase
///
/// This class template is used to declare concrete subclasses of
/// RefinerSurfaceFactoryBase with the additional support of an internal
/// cache used by the base class. With an instance of a thread-safe
/// subclass of SurfaceFactoryCache declared as a member, the resulting
/// factory will be thread-safe.
///
/// @tparam CACHE_TYPE  A subclass of SurfaceFactoryCache
///
/// Note a default template parameter uses the base SurfaceFactoryCache
/// for convenience, but which is not thread-safe.
///
template <class CACHE_TYPE = SurfaceFactoryCache>
class RefinerSurfaceFactory : public RefinerSurfaceFactoryBase {
public:
    RefinerSurfaceFactory(Far::TopologyRefiner const & mesh,
                          Options const & options = Options()) :
            RefinerSurfaceFactoryBase(mesh, options),
            _localCache() {

        SurfaceFactory::setInternalCache(&_localCache);
    }
    ~RefinerSurfaceFactory() override = default;

private:
    CACHE_TYPE _localCache;
};

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_REFINER_SURFACE_FACTORY_H */
