/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team


All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/
/// \file X3DImporter_Macro.hpp
/// \brief Useful macrodefines.
/// \date 2015-2016
/// \author smal.root@gmail.com

#ifndef X3DIMPORTER_MACRO_HPP_INCLUDED
#define X3DIMPORTER_MACRO_HPP_INCLUDED

#include <assimp/XmlParser.h>
#include "X3DImporter.hpp"
#include <string>

namespace Assimp {

/// Used for regular checking while attribute "USE" is defined.
/// \param [in] pNode - pugi xml node to read.
/// \param [in] pDEF - string holding "DEF" value.
/// \param [in] pUSE - string holding "USE" value.
/// \param [in] pType - type of element to find.
/// \param [out] pNE - pointer to found node element.
inline X3DNodeElementBase *X3DImporter::MACRO_USE_CHECKANDAPPLY(XmlNode &node, const std::string &pDEF, const std::string &pUSE, X3DElemType pType, X3DNodeElementBase *pNE) {
    checkNodeMustBeEmpty(node);
    if (!pDEF.empty())
        Assimp::Throw_DEF_And_USE(node.name());
    if (!FindNodeElement(pUSE, pType, &pNE))
        Assimp::Throw_USE_NotFound(node.name(), pUSE);
    ai_assert(nullptr != mNodeElementCur);
    mNodeElementCur->Children.push_back(pNE); /* add found object as child to current element */

    return pNE;
}

} // namespace Assimp

/// \def MACRO_ATTRREAD_CHECKUSEDEF_RET
/// Compact variant for checking "USE" and "DEF".
/// \param [in] pNode - pugi xml node to read.
/// \param [out] pDEF_Var - output variable name for "DEF" value.
/// \param [out] pUSE_Var - output variable name for "USE" value.
#define MACRO_ATTRREAD_CHECKUSEDEF_RET(pNode, pDEF_Var, pUSE_Var) \
    do {                                                          \
    XmlParser::getStdStrAttribute(pNode, "DEF", pDEF_Var);        \
    XmlParser::getStdStrAttribute(pNode, "USE", pUSE_Var);        \
    } while (false)

/// \def MACRO_FACE_ADD_QUAD_FA(pCCW, pOut, pIn, pP1, pP2, pP3, pP4)
/// Add points as quad. Means that pP1..pP4 set in CCW order.
#define MACRO_FACE_ADD_QUAD_FA(pCCW, pOut, pIn, pP1, pP2, pP3, pP4) \
    do {                                                            \
        if (pCCW) {                                                 \
            pOut.push_back(pIn[pP1]);                               \
            pOut.push_back(pIn[pP2]);                               \
            pOut.push_back(pIn[pP3]);                               \
            pOut.push_back(pIn[pP4]);                               \
        } else {                                                    \
            pOut.push_back(pIn[pP4]);                               \
            pOut.push_back(pIn[pP3]);                               \
            pOut.push_back(pIn[pP2]);                               \
            pOut.push_back(pIn[pP1]);                               \
        }                                                           \
    } while (false)

/// \def MACRO_FACE_ADD_QUAD(pCCW, pOut, pP1, pP2, pP3, pP4)
/// Add points as quad. Means that pP1..pP4 set in CCW order.
#define MACRO_FACE_ADD_QUAD(pCCW, pOut, pP1, pP2, pP3, pP4) \
    do {                                                    \
        if (pCCW) {                                         \
            pOut.push_back(pP1);                            \
            pOut.push_back(pP2);                            \
            pOut.push_back(pP3);                            \
            pOut.push_back(pP4);                            \
        } else {                                            \
            pOut.push_back(pP4);                            \
            pOut.push_back(pP3);                            \
            pOut.push_back(pP2);                            \
            pOut.push_back(pP1);                            \
        }                                                   \
    } while (false)

#endif // X3DIMPORTER_MACRO_HPP_INCLUDED
