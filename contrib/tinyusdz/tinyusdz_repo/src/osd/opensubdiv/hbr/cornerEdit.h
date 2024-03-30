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

#ifndef OPENSUBDIV3_HBRCORNEREDIT_H
#define OPENSUBDIV3_HBRCORNEREDIT_H

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrCornerEdit;

template <class T>
std::ostream& operator<<(std::ostream& out, const HbrCornerEdit<T>& path) {
    out << "vertex path = (" << path.faceid << ' ';
    for (int i = 0; i < path.nsubfaces; ++i) {
        out << static_cast<int>(path.subfaces[i]) << ' ';
    }
    return out << static_cast<int>(path.vertexid) << "), sharpness = " << path.sharpness;
}

template <class T>
class HbrCornerEdit : public HbrHierarchicalEdit<T> {

public:

    HbrCornerEdit(int _faceid, int _nsubfaces, unsigned char *_subfaces, unsigned char _vertexid, typename HbrHierarchicalEdit<T>::Operation _op, float _sharpness)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces), vertexid(_vertexid), op(_op), sharpness(_sharpness) {
    }

    HbrCornerEdit(int _faceid, int _nsubfaces, int *_subfaces, int _vertexid, typename HbrHierarchicalEdit<T>::Operation _op, float _sharpness)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces), vertexid(static_cast<unsigned char>(_vertexid)), op(_op), sharpness(_sharpness) {
    }

    virtual ~HbrCornerEdit() {}

    friend std::ostream& operator<< <T> (std::ostream& out, const HbrCornerEdit<T>& path);

    virtual void ApplyEditToFace(HbrFace<T>* face) {
        if (HbrHierarchicalEdit<T>::GetNSubfaces() == face->GetDepth()) {
            // Modify vertex sharpness. Note that we could actually do
            // this in ApplyEditToVertex as well!
            float sharp = 0.0f;
            if (op == HbrHierarchicalEdit<T>::Set) {
                sharp = sharpness;
            } else if (op == HbrHierarchicalEdit<T>::Add) {
                sharp = face->GetVertex(vertexid)->GetSharpness() + sharpness;
            } else if (op == HbrHierarchicalEdit<T>::Subtract) {
                sharp = face->GetVertex(vertexid)->GetSharpness() - sharpness;
            }
            if (sharp < HbrVertex<T>::k_Smooth) {
                sharp = HbrVertex<T>::k_Smooth;
            }
            if (sharp > HbrVertex<T>::k_InfinitelySharp) {
                sharp = HbrVertex<T>::k_InfinitelySharp;
            }
            face->GetVertex(vertexid)->SetSharpness(sharp);
        }
    }

private:
    // ID of the edge (you can think of this also as the id of the
    // origin vertex of the two-vertex length edge)
    const unsigned char vertexid;
    typename HbrHierarchicalEdit<T>::Operation op;
    // sharpness of the vertex edit
    const float sharpness;
};


} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRCORNEREDIT_H */
