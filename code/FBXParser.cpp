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

/** @file  FBXParser.cpp
 *  @brief Implementation of the FBX parser and the rudimentary DOM that we use
 */
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_FBX_IMPORTER

#include "FBXTokenizer.h"
#include "FBXParser.h"
#include "FBXUtil.h"

using namespace Assimp;
using namespace Assimp::FBX;

namespace {

// ------------------------------------------------------------------------------------------------
// signal parsing error, this is always unrecoverable. Throws DeadlyImportError.
void ParseError(const std::string& message, TokenPtr token)
{
	throw DeadlyImportError(token ? Util::AddTokenText("FBX-Parse",message,token) : ("FBX-Parse " + message));
}

}

// ------------------------------------------------------------------------------------------------
Element::Element(Parser& parser)
{
	TokenPtr n = NULL;
	do {
		n = parser.AdvanceToNextToken();
		if(!n) {
			ParseError("unexpected end of file, expected closing bracket",parser.LastToken());
		}

		if (n->Type() == TokenType_DATA) {
			tokens.push_back(n);

			n = parser.AdvanceToNextToken();
			if(!n) {
				ParseError("unexpected end of file, expected bracket, comma or key",parser.LastToken());
			}

			const TokenType ty = n->Type();
			if (ty != TokenType_OPEN_BRACKET && ty != TokenType_CLOSE_BRACKET && ty != TokenType_COMMA && ty != TokenType_KEY) {
				ParseError("unexpected token; expected bracket, comma or key",n);
			}
		}

		if (n->Type() == TokenType_OPEN_BRACKET) {
			compound.reset(new Scope(parser));

			// current token should be a TOK_CLOSE_BRACKET
			n = parser.CurrentToken();
			ai_assert(n);

			if (n->Type() != TokenType_CLOSE_BRACKET) {
				ParseError("expected closing bracket",n);
			}

			parser.AdvanceToNextToken();
			return;
		}
	}
	while(n->Type() != TokenType_KEY && n->Type() != TokenType_CLOSE_BRACKET);
}

// ------------------------------------------------------------------------------------------------
Element::~Element()
{
	std::for_each(tokens.begin(),tokens.end(),Util::delete_fun<Token>());
}
#include <Windows.h>
// ------------------------------------------------------------------------------------------------
Scope::Scope(Parser& parser,bool topLevel)
{
	if(!topLevel) {
		TokenPtr t = parser.CurrentToken();
		if (t->Type() != TokenType_OPEN_BRACKET) {
			ParseError("expected open bracket",t);
		}	
	}

	TokenPtr n = parser.AdvanceToNextToken();
	if(n == NULL) {
		ParseError("unexpected end of file",NULL);
	}

	// note: empty scopes are allowed
	while(n->Type() != TokenType_CLOSE_BRACKET)	{
		if (n->Type() != TokenType_KEY) {
			ParseError("unexpected token, expected TOK_KEY",n);
		}

		const std::string& str = n->StringContents();
		elements.insert(ElementMap::value_type(str,new_Element(parser)));

		// Element() should stop at the next Key token (or right after a Close token)
		n = parser.CurrentToken();
		if(n == NULL) {
			if (topLevel) {
				return;
			}
			ParseError("unexpected end of file",parser.LastToken());
		}
	}
}

// ------------------------------------------------------------------------------------------------
Scope::~Scope()
{
	BOOST_FOREACH(ElementMap::value_type& v, elements) {
		delete v.second;
	}
}


// ------------------------------------------------------------------------------------------------
Parser::Parser (const TokenList& tokens)
: tokens(tokens)
, cursor(tokens.begin())
, current()
, last()
{
	root.reset(new Scope(*this,true));
}


// ------------------------------------------------------------------------------------------------
Parser::~Parser()
{
}


// ------------------------------------------------------------------------------------------------
TokenPtr Parser::AdvanceToNextToken()
{
	last = current;
	if (cursor == tokens.end()) {
		current = NULL;
	}
	else {
		current = *cursor++;
	}
	return current;
}


// ------------------------------------------------------------------------------------------------
TokenPtr Parser::CurrentToken() const
{
	return current;
}


// ------------------------------------------------------------------------------------------------
TokenPtr Parser::LastToken() const
{
	return last;
}


#endif

