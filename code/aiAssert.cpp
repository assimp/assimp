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
#include "AssimpPCH.h"
#include "../include/aiAssert.h"
#ifdef _WIN32
#ifndef __GNUC__
#  include "crtdbg.h"
#endif // ndef gcc
#endif // _WIN32

// Set a breakpoint using win32, else line, file and message will be returned and progam ends with 
// errrocode = 1
AI_WONT_RETURN void Assimp::aiAssert (const std::string &message, unsigned int uiLine, const std::string &file)
{
  // moved expression testing out of the function and into the macro to avoid constructing
  // two std::string on every single ai_assert test
//	if (!expression) 
  {
    // FIX (Aramis): changed std::cerr to std::cout that the message appears in VS' output window ...
	  std::cout << "File :" << file << ", line " << uiLine << " : " << message << std::endl;

#ifdef _WIN32
#ifndef __GNUC__
		// Set breakpoint
		__debugbreak();
#endif //ndef gcc
#else
		exit (1);
#endif
	}
}
