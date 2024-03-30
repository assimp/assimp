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
#ifndef OPENSUBDIV3_FAR_PTEX_INDICES_H
#define OPENSUBDIV3_FAR_PTEX_INDICES_H

#include "../version.h"

#include "../far/topologyRefiner.h"
#include "../far/types.h"

#include <vector>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

///
/// \brief Object used to compute and query ptex face indices.
///
/// Given a refiner, constructing a PtexIndices object builds the mapping
/// from coarse faces to ptex ids.  Once built, the object can be used to
/// query the mapping.
///
class PtexIndices {

public:

    /// \brief Constructor
    PtexIndices(TopologyRefiner const &refiner);

    /// \brief Destructor
    ~PtexIndices();

    //@{
    ///
    /// Ptex
    ///

    /// \brief Returns the number of ptex faces in the mesh
    ///
    int GetNumFaces() const;

    /// \brief Returns the ptex face index given a coarse face 'f' or -1
    ///
    int GetFaceId(Index f) const;

    /// \brief Returns ptex face adjacency information for a given coarse face
    ///
    /// @param refiner   refiner used to build this PtexIndices object.
    ///
    /// @param face      coarse face index
    ///
    /// @param quadrant  quadrant index if 'face' is not a quad (the local ptex
    ///                  sub-face index). Must be less than the number of face
    ///                  vertices.
    ///
    /// @param adjFaces  ptex face indices of adjacent faces
    ///
    /// @param adjEdges  ptex edge indices of adjacent faces
    ///
    void GetAdjacency(
        TopologyRefiner const &refiner,
        int face, int quadrant,
        int adjFaces[4], int adjEdges[4]) const;

    //@}

private:

    void initializePtexIndices(TopologyRefiner const &refiner);

private:

    std::vector<Index> _ptexIndices;
};


} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_FAR_PTEX_INDICES_H */
