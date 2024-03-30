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

#ifndef OPENSUBDIV3_HBRHOLEEDIT_H
#define OPENSUBDIV3_HBRHOLEEDIT_H

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrHoleEdit;

template <class T>
std::ostream& operator<<(std::ostream& out, const HbrHoleEdit<T>& path) {
    out << "edge path = (" << path.faceid << ' ';
    for (int i = 0; i < path.nsubfaces; ++i) {
        out << static_cast<int>(path.subfaces[i]) << ' ';
    }
    return out << ")";
}

template <class T>
class HbrHoleEdit : public HbrHierarchicalEdit<T> {

public:

    HbrHoleEdit(int _faceid, int _nsubfaces, unsigned char *_subfaces)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces) {
    }

    HbrHoleEdit(int _faceid, int _nsubfaces, int *_subfaces)
        : HbrHierarchicalEdit<T>(_faceid, _nsubfaces, _subfaces) {
    }

    virtual ~HbrHoleEdit() {}

    friend std::ostream& operator<< <T> (std::ostream& out, const HbrHoleEdit<T>& path);

    virtual void ApplyEditToFace(HbrFace<T>* face) {
        if (HbrHierarchicalEdit<T>::GetNSubfaces() == face->GetDepth()) {
            face->SetHole();
        }
    }
};


} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRHOLEEDIT_H */
