/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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

/** @file  FBXDocumentUtil.h
 *  @brief FBX internal utilities used by the DOM reading code
 */
#ifndef INCLUDED_AI_FBX_DOCUMENT_UTIL_H
#define INCLUDED_AI_FBX_DOCUMENT_UTIL_H

namespace Assimp {
namespace FBX {
namespace Util {

void DOMError(const std::string& message, const Token& token);
void DOMError(const std::string& message, const Element* element = NULL);

// extract required compound scope
const Scope& GetRequiredScope(const Element& el);
// get token at a particular index
const Token& GetRequiredToken(const Element& el, unsigned int index);

// wrapper around ParseTokenAsID() with DOMError handling
uint64_t ParseTokenAsID(const Token& t);
// wrapper around ParseTokenAsDim() with DOMError handling
size_t ParseTokenAsDim(const Token& t);
// wrapper around ParseTokenAsFloat() with DOMError handling
float ParseTokenAsFloat(const Token& t);
// wrapper around ParseTokenAsInt() with DOMError handling
int ParseTokenAsInt(const Token& t);
// wrapper around ParseTokenAsString() with DOMError handling
std::string ParseTokenAsString(const Token& t);


// extract a required element from a scope, abort if the element cannot be found
const Element& GetRequiredElement(const Scope& sc, const std::string& index, const Element* element = NULL);

// read an array of float3 tuples
void ReadVectorDataArray(std::vector<aiVector3D>& out, const Element& el);

// read an array of color4 tuples
void ReadVectorDataArray(std::vector<aiColor4D>& out, const Element& el);

// read an array of float2 tuples
void ReadVectorDataArray(std::vector<aiVector2D>& out, const Element& el);

// read an array of ints
void ReadVectorDataArray(std::vector<int>& out, const Element& el);

// read an array of uints
void ReadVectorDataArray(std::vector<unsigned int>& out, const Element& el);

} //!Util
} //!FBX
} //!Assimp

#endif
