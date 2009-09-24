/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
 */

#pragma once

//managed includes
#include "mIOStream.h"

//native includes
#include "IOSystem.h"

using namespace System;

namespace AssimpNET
{
	public ref class IOSystem abstract
	{
	public:
		IOSystem(void);
		IOSystem(Assimp::IOSystem* native);
		virtual ~IOSystem(void);
	
		virtual void 	Close (IOStream^ pFile) = 0;
		bool ComparePaths (const String^ one, const String^ second);
		virtual bool ComparePaths (array<char>^ one, array<char>^ second);
		virtual bool Exists(array<char>^ pFile) = 0;
		bool Exists(const String^ pFile);
		virtual char getOsSeperator() = 0;
		IOStream^ Open(const String^ pFile, const String^ pMode);
		virtual IOStream^ Open(array<char>^ pFile, array<char>^ pMode) = 0;

		Assimp::IOSystem* getNative();
		private:
		Assimp::IOSystem *p_native;
	};
}//namespace




