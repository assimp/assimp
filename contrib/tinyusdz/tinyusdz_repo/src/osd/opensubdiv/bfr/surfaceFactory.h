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

#ifndef OPENSUBDIV3_BFR_SURFACE_FACTORY_H
#define OPENSUBDIV3_BFR_SURFACE_FACTORY_H

#include "../version.h"

#include "../bfr/surface.h"
#include "../bfr/surfaceFactoryMeshAdapter.h"
#include "../sdc/options.h"
#include "../sdc/types.h"

#include <cstdint>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  Forward declarations of public and internal classes used by factories:
//
class SurfaceFactoryCache;
class FaceTopology;
class FaceSurface;

///
/// @brief Base class providing initialization of a Surface for each face
///        of a mesh
///
/// SurfaceFactory is an abstract class that provides the majority of
/// the implementation and the interface for a factory that initializes
/// instances of Surface for the faces of a mesh.
///
/// A subclass of SurfaceFactory is written to support a specific type
/// of connected mesh. The public interface of SurfaceFactory is both
/// inherited by and extended by the subclasses. Expected extensions to
/// the interface include one or more constructors (i.e. given a specific
/// instance of the subclass' mesh type) as well as other methods that
/// may involve the mesh's data types (primvars) in their native form.
///
/// By inheriting the SurfaceFactoryMeshAdapter interface, SurfaceFactory
/// requires its subclasses to implement the small suite of pure
/// virtual methods to complete the factory's implementation for the
/// subclass' mesh type. These methods provide the base factory with
/// topological information about faces of that mesh -- from which it
/// creates instances of Surface defining their limit surface.
///
/// The SurfaceFactory inherits rather than contains SurfaceFactoryMeshAdapter
/// as instances of SurfaceFactoryMeshAdapter serve no purpose on their own,
/// and the interface between the two is designed with the specific needs
/// of the SurfaceFactory. When customizing a subclass of SurfaceFactory
/// for a particular mesh type, this inheritance also avoids the need to
/// coordinate the subclass of SurfaceFactory with the separate subclass
/// of SurfaceFactoryMeshAdapter.
///
/// It must be emphasized that a subclass of SurfaceFactory is written to
/// support a specific type of "connected" mesh -- not simply a container
/// of data defining a mesh. The SurfaceFactoryMeshAdapter interface describes
/// the complete topological neighborhood around a specific face, and
/// without any connectivity between mesh components (e.g. given a vertex,
/// what are its incident faces?), satisfying these methods will be
/// impossible, or, at best, extremely inefficient.
///
/// Ultimately a subclass of SurfaceFactory is expected to be a lightweight
/// interface to a connected mesh -- lightweight in terms of both time and
/// memory usage. It's construction is expected to be trivial, after which
/// it can quickly and efficiently provide a Surface for one or more faces
/// of a mesh for immediate evaluation. So construction of an instance of
/// a subclass should involve no heavy pre-processing -- the greater the
/// overhead of a subclass constructor, the more it violates the intention
/// of the base class as a lightweight interface.
///
/// Instances of SurfaceFactory are initialized with a set of Options that
/// form part of the state of the factory and remain fixed for its lifetime.
/// Such options are intended to ensure that the instances of Surface that
/// it creates are consistent, as well as to enable/disable or otherwise
/// manage caching for construction efficiency -- either internally or
/// between itself and other factories (advanced).
///
class SurfaceFactory : public SurfaceFactoryMeshAdapter {
public:
    ///
    /// @brief Simple set of options assigned to instances of SurfaceFactory
    ///
    /// The Options class is a simple container specifying options for the
    /// construction of the SurfaceFactory to be applied during its lifetime.
    ///
    /// These options currently include choices to identify a default
    /// face-varying ID, to control caching behavior (on or off, use of
    /// external vs internal cache), and to control the accuracy of the
    /// resulting limit surface representations.
    ///
    class Options {
    public:
        Options() : _dfltFVarID(-1), _externCache(0), _enableCache(true),
                    _approxLevelSmooth(2), _approxLevelSharp(6) { }

        /// @brief Assign the default face-varying ID (none assigned by
        ///        default)
        Options & SetDefaultFVarID(FVarID id);
        /// @brief Return the default face-varying ID
        FVarID GetDefaultFVarID() const { return _dfltFVarID; }

        /// @brief Enable or disable caching (default is true):
        Options & EnableCaching(bool on);
        /// @brief Return if caching is enable
        bool IsCachingEnabled() const { return _enableCache; }

        /// @brief Assign an external cache to override the internal
        Options & SetExternalCache(SurfaceFactoryCache * c);
        /// @brief Return any assigned external cache
        SurfaceFactoryCache * GetExternalCache() const { return _externCache; }

        //  Set refinement levels used to approximate the limit surface
        //  for smooth and sharp features (reasonable defaults assigned):
        /// @brief Assign maximum refinement level for smooth features
        Options & SetApproxLevelSmooth(int level);
        /// @brief Return maximum refinement level for smooth features
        int GetApproxLevelSmooth() const { return _approxLevelSmooth; }

        /// @brief Assign maximum refinement level for sharp features
        Options & SetApproxLevelSharp(int level);
        /// @brief Return maximum refinement level for sharp features
        int GetApproxLevelSharp() const { return _approxLevelSharp; }

    private:
        //  Member variables:
        FVarID _dfltFVarID;

        SurfaceFactoryCache * _externCache;

        unsigned char _enableCache : 1;
        unsigned char _approxLevelSmooth;
        unsigned char _approxLevelSharp;
    };

public:
    ~SurfaceFactory() override;

    //@{
    /// @name Simple queries of subdivision properties
    ///
    /// Simple queries to inspect subdivision properties.
    ///

    /// @brief Return the subdivision scheme
    Sdc::SchemeType GetSchemeType() const { return _subdivScheme; }

    /// @brief Return the set of subdivision options
    Sdc::Options GetSchemeOptions() const { return _subdivOptions; }
    //@}

public:
    //@{
    /// @name Simple queries influencing Surface construction
    ///
    /// Methods to quickly inspect faces that influence Surface construction.
    ///
    /// A small set of methods is useful to inspect faces in order to
    /// determine if their corresponding Surfaces should be initialized.
    /// The Surface initialization methods will fail when a limit surface
    /// does exist, so the methods here are intended for purposes when
    /// that simple failure on initialization is not suitable, e.g. to
    /// address some kind of pre-processing need prior to the initialization
    /// of any Surfaces.
    ///

    /// @brief Return if a specified face has a limit surface
    ///
    /// This method determines if a face has an associated limit surface,
    /// and so supports initialization of Surface for evaluation. This
    /// is usually the case, except when the face is tagged as a hole, or
    /// due to the use of uncommon boundary interpolation options (i.e.
    /// Sdc::Options::VTX_BOUNDARY_NONE). The test of a hole is trivial,
    /// but the boundary test is not when such uncommon options are used.
    ///
    bool FaceHasLimitSurface(Index faceIndex) const;

    /// @brief Return the Parameterization of a face with a limit surface
    ///
    /// This method simply returns the Parameterization of the specified
    /// face. It is presumed the face has an existing limit surface and
    /// so is a quick and simple accessor.
    ///
    Parameterization GetFaceParameterization(Index faceIndex) const;
    //@}

public:
    //@{
    /// @name Methods to initialize Surfaces
    ///
    /// Methods to initialize instances of Surface for a specific face.
    ///
    /// Given the different interpolation types for data associated with
    /// mesh vertices (i.e. vertex, varying and face-varying data), the
    /// topology of the limit surface potentially (likely) differs between
    /// them. So it is necessary to specify the type of data to be associated
    /// with the Surface. Methods exist to initialize a single surface for
    /// each of the three data types, and to initialize multiple surfaces
    /// for different data types at once (which will avoid repeated effort
    /// in a single-threaded context).
    /// 
    /// Failure of these initialization methods is expected (and so to be
    /// tested) when a face has no limit surface -- either due to it being
    /// a hole or through the use of less common boundary interpolation
    /// options. Failure is also possible if the subclass fails to provide
    /// a valid topological description of the face. (WIP - consider more
    /// extreme failure for these cases, e.g. possible assertions.)
    ///

    /// @brief Initialize a Surface for vertex data
    ///
    /// @param  faceIndex Index of face with limit surface of interest
    /// @param  surface   Surface to initialize for vertex data
    /// @return           True if the face has a limit surface and it was
    ///                   successfully constructed
    ///
    template <typename REAL>
    bool InitVertexSurface(Index faceIndex, Surface<REAL> * surface) const;

    /// @brief Initialize a Surface for varying data
    ///
    /// @param  faceIndex Index of face with limit surface of interest
    /// @param  surface   Surface to initialize for varying data
    /// @return           True if the face has a limit surface and it was
    ///                   successfully constructed
    ///
    template <typename REAL>
    bool InitVaryingSurface(Index faceIndex, Surface<REAL> * surface) const;

    /// @brief Initialize a Surface for the default face-varying data
    ///
    /// For this variant, no explicit face-varying ID is specified. The
    /// default is determined from the Options with which the SurfaceFactory
    /// was created (assignment of that default is required).
    ///
    /// @param  faceIndex Index of face with limit surface of interest
    /// @param  surface   Surface to initialize for face-varying data
    /// @return           True if the face has a limit surface, the default
    ///                   face-varying ID was valid, and its Surface was
    ///                   successfully constructed
    ///
    template <typename REAL>
    bool InitFaceVaryingSurface(Index faceIndex, Surface<REAL> * surface) const;

    /// @brief Initialize a Surface for specified face-varying data
    ///
    /// @param  faceIndex Index of face with limit surface of interest
    /// @param  surface   Surface to initialize for face-varying data
    /// @param  fvarID    Identifier of a specific set of face-varying data
    /// @return           True if the face has a limit surface, the given
    ///                   face-varying ID was valid, and its Surface was
    ///                   successfully constructed
    ///
    template <typename REAL>
    bool InitFaceVaryingSurface(Index faceIndex, Surface<REAL> * surface,
                                                 FVarID          fvarID) const;

    ///
    /// @brief Initialize multiple Surfaces at once
    ///
    /// This method initializes multiple Surfaces at once -- for any
    /// combination of the three different data interpolation types.
    /// Its use is recommended when two are more surfaces are known to
    /// be non-linear, which will avoid the repeated effort if each 
    /// Surface is individually initialized.
    ///
    /// Arguments are ordered here to satisfy common cases easily with
    /// the use of optional arguments for less common cases.
    ///
    /// @param  faceIndex    Index of face with limit surfaces of interest
    /// @param  vtxSurface   Surface to initialize for vertex data
    /// @param  fvarSurfaces Surface array to initialize for face-varying data
    /// @param  fvarIDs      Array of face-varying IDs corresponding to the
    ///                      face-varying Surfaces to be initialized
    ///                      (optional -- defaults to an integer sequence
    ///                      [0 .. fvarCount-1] if absent)
    /// @param  fvarCount    Size of array of face-varying Surfaces (optional)
    /// @param  varSurface   Surface to initialize for varying data (optional)
    /// @return              True if the face has a limit surface, any given
    ///                      face-varying IDs were valid, and all Surfaces
    ///                      were successfully constructed.
    ///
    template <typename REAL>
    bool InitSurfaces(Index faceIndex, Surface<REAL> * vtxSurface,
                                       Surface<REAL> * fvarSurfaces,
                                       FVarID const    fvarIDs[] = 0,
                                       int             fvarCount = 0,
                                       Surface<REAL> * varSurface = 0) const;
    //@}

    //@{
    /// @name Methods to construct Surfaces
    ///
    /// Methods to both allocate and initialize a single Surface.
    //
    //      WIP - considering removing these since non-essential
    //

    /// @brief Construct a Surface for vertex data
    template <typename REAL=float>
    Surface<REAL> * CreateVertexSurface(Index faceIndex) const;

    /// @brief Construct a Surface for varying data
    template <typename REAL=float>
    Surface<REAL> * CreateVaryingSurface(Index faceIndex) const;

    /// @brief Construct a Surface for the default face-varying data
    template <typename REAL=float>
    Surface<REAL> * CreateFaceVaryingSurface(Index faceIndex) const;

    /// @brief Construct a Surface for specified face-varying data
    template <typename REAL=float>
    Surface<REAL> * CreateFaceVaryingSurface(Index faceIndex, FVarID id) const;
    //@}

protected:
    //@{
    /// @name Protected methods supporting subclass construction
    ///
    /// Protected methods supporting subclass construction.
    ///

    ///
    /// @brief Constructor to be used by subclasses
    ///
    /// Construction requires specification of the subdivision scheme and
    /// options associated with the mesh (as is the case with other classes
    /// in Far). These will typically reflect the settings in the mesh but
    /// can also be used to override them -- as determined by the subclass.
    /// Common uses of overrides are to assign a subdivision scheme to a
    /// simple polygonal mesh, or to change the face-varying interpolation
    /// for the faster linear interpolation of UVs.
    ///
    SurfaceFactory(Sdc::SchemeType      schemeType,
                   Sdc::Options const & schemeOptions,
                   Options      const & limitOptions);

    /// @brief Subclass to identify an internal cache for use by base class
    void setInternalCache(SurfaceFactoryCache * cache);

    SurfaceFactory(SurfaceFactory const &) = delete;
    SurfaceFactory & operator=(SurfaceFactory const &) = delete;
    //@}

private:
    //  Supporting internal methods:
    void setSubdivisionOptions(Sdc::SchemeType, Sdc::Options const & options);
    void setFactoryOptions(Options const & factoryOptions);

    bool faceHasLimitSimple(Index faceIndex, int faceSize) const;

    bool faceHasLimitNeighborhood(Index faceIndex) const;
    bool faceHasLimitNeighborhood(FaceTopology const & faceTopology) const;

    class SurfaceSet;

    bool populateAllSurfaces(      Index faceIndex, SurfaceSet * sSetPtr) const;
    bool populateLinearSurfaces(   Index faceIndex, SurfaceSet * sSetPtr) const;
    bool populateNonLinearSurfaces(Index faceIndex, SurfaceSet * sSetPtr) const;

    bool initSurfaces(Index faceIndex, internal::SurfaceData * vtxSurface,
                                       internal::SurfaceData * varSurface,
                                       internal::SurfaceData * fvarSurfaces,
                                       int           fvarCount,
                                       FVarID const  fvarIDs[]) const;

    //  Methods to assemble topology and corresponding indices for entire face:
    bool isFaceNeighborhoodRegular(Index          faceIndex,
                                   FVarID const * fvarPtrOrVtx,
                                   Index          indices[]) const;

    bool initFaceNeighborhoodTopology(Index          faceIndex,
                                      FaceTopology * topology) const;

    bool gatherFaceNeighborhoodTopology(Index          faceIndex,
                                        FaceTopology * topology) const;

    int gatherFaceNeighborhoodIndices(Index                faceIndex,
                                      FaceTopology const & topology,
                                      FVarID       const * fvarPtrOrVtx,
                                      Index                indices[]) const;

    //  Methods to assemble Surfaces for the different categories of patch:
    typedef internal::SurfaceData SurfaceType;

    void assignLinearSurface(SurfaceType  * surfacePtr,
                             Index          faceIndex,
                             FVarID const * fvarPtrOrVtx) const;

    void assignRegularSurface(SurfaceType * surfacePtr,
                              Index const   surfacePatchPoints[]) const;

    void assignRegularSurface(SurfaceType       * surfacePtr,
                              FaceSurface const & surfaceDescription) const;

    void assignIrregularSurface(SurfaceType       * surfacePtr,
                                FaceSurface const & surfaceDescription) const;

    void copyNonLinearSurface(SurfaceType       * surfacePtr,
                              SurfaceType const & surfaceSource,
                              FaceSurface const & surfaceDescription) const;

private:
    //  Members describing options and subdivision properties (very little
    //  memory and low initialization cost)
    Sdc::SchemeType _subdivScheme;
    Sdc::Options    _subdivOptions;
    Options         _factoryOptions;

    //  Members related to subdivision topology, options and limit tests:
    unsigned int _linearScheme      : 1;
    unsigned int _linearFVarInterp  : 1;

    unsigned int _testNeighborhoodForLimit       : 1;
    unsigned int _rejectSmoothBoundariesForLimit : 1;
    unsigned int _rejectIrregularFacesForLimit   : 1;

    int  _regFaceSize;

    //  Members related to caching:
    SurfaceFactoryCache mutable * _topologyCache;
};

//
//  Inline methods for Options:
//
inline SurfaceFactory::Options &
SurfaceFactory::Options::SetDefaultFVarID(FVarID id) {
    _dfltFVarID = id;
    return *this;
}
inline SurfaceFactory::Options &
SurfaceFactory::Options::EnableCaching(bool on) {
    _enableCache = on;
    return *this;
}
inline SurfaceFactory::Options & 
SurfaceFactory::Options::SetExternalCache(SurfaceFactoryCache * c) {
    _externCache = c;
    return *this;
}
inline SurfaceFactory::Options &
SurfaceFactory::Options::SetApproxLevelSmooth(int level) {
    _approxLevelSmooth = (unsigned char) level;
    return *this;
}
inline SurfaceFactory::Options &
SurfaceFactory::Options::SetApproxLevelSharp(int level) {
    _approxLevelSharp = (unsigned char) level;
    return *this;
}

//
//  Inline methods to initializes Surfaces:
//
template <typename REAL>
inline bool
SurfaceFactory::InitVertexSurface(Index face, Surface<REAL> * s) const {

    return initSurfaces(face, &s->getSurfaceData(), 0, 0, 0, 0);
}
template <typename REAL>
inline bool
SurfaceFactory::InitVaryingSurface(Index face, Surface<REAL> * s) const {

    return initSurfaces(face, 0, &s->getSurfaceData(), 0, 0, 0);
}
template <typename REAL>
inline bool
SurfaceFactory::InitFaceVaryingSurface(Index face, Surface<REAL> * s,
                                                   FVarID fvarID) const {
    return initSurfaces(face, 0, 0, &s->getSurfaceData(), 1, &fvarID);
}
template <typename REAL>
inline bool
SurfaceFactory::InitFaceVaryingSurface(Index face, Surface<REAL> * s) const {
    FVarID dfltID = _factoryOptions.GetDefaultFVarID();
    return initSurfaces(face, 0, 0, &s->getSurfaceData(), 1, &dfltID);
}

template <typename REAL>
inline bool
SurfaceFactory::InitSurfaces(Index faceIndex, Surface<REAL> * vtxSurface,
        Surface<REAL> * fvarSurfaces, FVarID const fvarIDs[], int fvarCount,
        Surface<REAL> * varSurface) const {

    bool useDfltFVarID = fvarSurfaces && (fvarIDs == 0) && (fvarCount == 0);
    FVarID dfltFVarID = useDfltFVarID ? _factoryOptions.GetDefaultFVarID() : 0;

    return initSurfaces(faceIndex,
                        vtxSurface    ? &vtxSurface->getSurfaceData()   : 0,
                        varSurface    ? &varSurface->getSurfaceData()   : 0,
                        fvarSurfaces  ? &fvarSurfaces->getSurfaceData() : 0,
                        fvarCount     ? fvarCount : (fvarSurfaces != 0),
                        useDfltFVarID ? &dfltFVarID : fvarIDs);
}

//
//  Inline methods to allocate and initialize Surfaces:
//
template <typename REAL>
inline Surface<REAL> *
SurfaceFactory::CreateVertexSurface(Index faceIndex) const {
    Surface<REAL> * s = new Surface<REAL>();
    if (InitVertexSurface<REAL>(faceIndex, s)) return s;
    delete s;
    return 0;
}
template <typename REAL>
inline Surface<REAL> *
SurfaceFactory::CreateVaryingSurface(Index faceIndex) const {
    Surface<REAL> * s = new Surface<REAL>();
    if (InitVaryingSurface<REAL>(faceIndex, s)) return s;
    delete s;
    return 0;
}
template <typename REAL>
inline Surface<REAL> *
SurfaceFactory::CreateFaceVaryingSurface(Index faceIndex, FVarID fvarID) const {
    Surface<REAL> * s = new Surface<REAL>();
    if (InitFaceVaryingSurface<REAL>(faceIndex, s, fvarID)) return s;
    delete s;
    return 0;
}
template <typename REAL>
inline Surface<REAL> *
SurfaceFactory::CreateFaceVaryingSurface(Index face) const {
    FVarID dfltID = _factoryOptions.GetDefaultFVarID();
    return CreateFaceVaryingSurface<REAL>(face, dfltID);
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_SURFACE_FACTORY_H */
