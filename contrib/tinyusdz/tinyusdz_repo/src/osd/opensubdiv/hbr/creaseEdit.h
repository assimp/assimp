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

#ifndef OPENSUBDIV3_HBRCREASEEDIT_H
#define OPENSUBDIV3_HBRCREASEEDIT_H

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrCreaseEdit;

template <class T>
std::ostream& operator<<(std::ostream& out, const HbrCreaseEdit<T>& path) {
    out << "edge path = (" << path.faceid << ' ';
    for (int i = 0; i < path.nsubfaces; ++i) {
        out << static_cast<int>(path.subfaces[i]) << ' ';
    }
    return out << static_cast<int>(path.edgeid) << "), sharpness = " << path.sharpness;
}

template <class T>
class HbrCreaseEdit : public HbrHierarchicalEdit<T> {

public:

    HbrCreaseEdit(int _faceid, int _nsubfaces, unsigned char *_subfaces, unsigned char _edgeid, typename HbrHierarchicalEdit<T>::Operation _op, float _sharpness)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces), edgeid(_edgeid), op(_op), sharpness(_sharpness) {
    }

    HbrCreaseEdit(int _faceid, int _nsubfaces, int *_subfaces, int _edgeid, typename HbrHierarchicalEdit<T>::Operation _op, float _sharpness)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces), edgeid(static_cast<unsigned char>(_edgeid)), op(_op), sharpness(_sharpness) {
    }

    virtual ~HbrCreaseEdit() {}

    friend std::ostream& operator<< <T> (std::ostream& out, const HbrCreaseEdit<T>& path);

    virtual void ApplyEditToFace(HbrFace<T>* face) {
        if (HbrHierarchicalEdit<T>::GetNSubfaces() == face->GetDepth()) {
            // Modify edge sharpness
            float sharp=0.0f;
            if (op == HbrHierarchicalEdit<T>::Set) {
                sharp = sharpness;
            } else if (op == HbrHierarchicalEdit<T>::Add) {
                sharp = face->GetEdge(edgeid)->GetSharpness() + sharpness;
            } else if (op == HbrHierarchicalEdit<T>::Subtract) {
                sharp = face->GetEdge(edgeid)->GetSharpness() - sharpness;
            }
            if (sharp < HbrHalfedge<T>::k_Smooth)
                sharp = HbrHalfedge<T>::k_Smooth;
            if (sharp > HbrHalfedge<T>::k_InfinitelySharp)
                sharp = HbrHalfedge<T>::k_InfinitelySharp;
            // We have to make sure the neighbor of the edge exists at
            // this point. Otherwise, if it comes into being late, it
            // will clobber the overriden sharpness and we will lose
            // the edit.
            face->GetEdge(edgeid)->GuaranteeNeighbor();
            face->GetEdge(edgeid)->SetSharpness(sharp);
        }
    }

private:
    // ID of the edge (you can think of this also as the id of the
    // origin vertex of the two-vertex length edge)
    const unsigned char edgeid;
    typename HbrHierarchicalEdit<T>::Operation op;
    // sharpness of the edge edit
    const float sharpness;
};


} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRCREASEEDIT_H */
