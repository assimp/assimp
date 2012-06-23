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

/** @file  FBXTokenizer.cpp
 *  @brief Implementation of the FBX broadphase lexer
 */
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_FBX_IMPORTER

#include "FBXTokenizer.h"
#include "ParsingUtils.h"

namespace Assimp {
namespace FBX {

// ------------------------------------------------------------------------------------------------
Token::Token(const char* sbegin, const char* send, TokenType type, unsigned int line, unsigned int column)
	: sbegin(sbegin)
	, send(send)
	, type(type)
	, line(line)
	, column(column)
{
	ai_assert(sbegin);
	ai_assert(send);
}


// ------------------------------------------------------------------------------------------------
Token::~Token()
{
}


// ------------------------------------------------------------------------------------------------
void Tokenize(TokenList& output_tokens, const char* input)
{
	ai_assert(input);

	// line and column numbers numbers are one-based
	unsigned int line = 1;
	unsigned int column = 1;

	bool comment = false;
	
	for (const char* cur = input;*cur;++cur,++column) {
		const char c = *cur;

		if (IsLineEnd(c)) {
			comment = false;

			column = 0;
			++line;

			continue;
		}

		if(comment) {
			continue;
		}

		switch(c)
		{
		case ';':
			comment = true;
			continue;

		case '{':
			output_tokens.push_back(boost::make_shared<Token>(cur,cur+1,TokenType_OPEN_BRACKET,line,column));
			break;

		case '}':
			output_tokens.push_back(boost::make_shared<Token>(cur,cur+1,TokenType_CLOSE_BRACKET,line,column));
			break;
		
		case ',':
			output_tokens.push_back(boost::make_shared<Token>(cur,cur+1,TokenType_COMMA,line,column));
			break;
		}

		
		if (IsSpaceOrNewLine(c)) {
			//
		}
		// XXX parse key and data elements
	}
}

} // !FBX
} // !Assimp

#endif
