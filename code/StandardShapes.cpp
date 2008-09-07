/*
Open Asset Import Library (ASSIMP)
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

/** @file Implementation of the StandardShapes class
 */
#include "../include/aiTypes.h"
#include "../include/DefaultLogger.h"
#include "../include/aiAssert.h"

#include "StandardShapes.h"

namespace Assimp	{

// ------------------------------------------------------------------------------------------------
void StandardShapes::MakeSphere(
	aiVector3D&		center,
	float			radius,
	float			tess,
	std::vector<aiVector3D>& positions)
{
}

// ------------------------------------------------------------------------------------------------
void StandardShapes::MakeCone(
	aiVector3D&		center1,
	float			radius1,
	aiVector3D&		center2,
	float			radius2,
	float			tess, 
	std::vector<aiVector3D>& positions, 
	bool bOpened /*= false*/)
{
}

// ------------------------------------------------------------------------------------------------
void StandardShapes::MakeCircle(
	aiVector3D&		center, 
	aiVector3D&		normal, 
	float			radius,
	float			tess,
	std::vector<aiVector3D>& positions)
{
}

} // ! Assimp