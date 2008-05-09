/*
Free Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
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

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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


#ifndef OBJFILEMTLIMPORTER_H_INC
#define OBJFILEMTLIMPORTER_H_INC

namespace Assimp
{

/**
 *	@class	ObjFileMtlImporter
 *	@brief	Loads the material description from a mtl file.
 */
class ObjFileMtlImporter
{
public:
	//!	\brief	Default constructor
	ObjFileMtlImporter();
	
	//!	\brief	DEstructor
	~ObjFileMtlImporter();

private:
	//!	\brief	Copy constructor, empty.
	ObjFileMtlImporter(const ObjFileMtlImporter &rOther);
	
	//!	\brief	Assignment operator, returns only a reference of this instance.
	ObjFileMtlImporter &operator = (const ObjFileMtlImporter &rOther);

	void getColorRGBA();
	void getIlluminationModel();
	void getFloatValue();
	void createMaterial();
	void getTexture();
};

} // Namespace Assimp

#endif
