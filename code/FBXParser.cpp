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

#include "ParsingUtils.h"
#include "fast_atof.h"

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

namespace Assimp {
namespace FBX {

// ------------------------------------------------------------------------------------------------
Element::Element(const Token& key_token, Parser& parser)
: key_token(key_token)
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
	 // no need to delete tokens, they are owned by the parser
}

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
		elements.insert(ElementMap::value_type(str,new_Element(*n,parser)));

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


// ------------------------------------------------------------------------------------------------
uint64_t ParseTokenAsID(const Token& t, const char*& err_out)
{
	err_out = NULL;

	if (t.Type() != TokenType_DATA) {
		err_out = "expected TOK_DATA token";
		return 0L;
	}

	// XXX: should use size_t here
	unsigned int length = static_cast<unsigned int>(t.end() - t.begin());
	ai_assert(length > 0);

	const char* out;
	const uint64_t id = strtoul10_64(t.begin(),&out,&length);
	if (out > t.end()) {
		err_out = "failed to parse ID";
		return 0L;
	}

	return id;
}


// ------------------------------------------------------------------------------------------------
size_t ParseTokenAsDim(const Token& t, const char*& err_out)
{
	// same as ID parsing, except there is a trailing asterisk
	err_out = NULL;

	if (t.Type() != TokenType_DATA) {
		err_out = "expected TOK_DATA token";
		return 0;
	}

	if(*t.begin() != '*') {
		err_out = "expected asterisk before array dimension";
		return 0;
	}

	// XXX: should use size_t here
	unsigned int length = static_cast<unsigned int>(t.end() - t.begin());
	if(length == 0) {
		err_out = "expected valid integer number after asterisk";
		return 0;
	}

	const char* out;
	const size_t id = static_cast<size_t>(strtoul10_64(t.begin() + 1,&out,&length));
	if (out > t.end()) {
		err_out = "failed to parse ID";
		return 0;
	}

	return id;
}


// ------------------------------------------------------------------------------------------------
float ParseTokenAsFloat(const Token& t, const char*& err_out)
{
	err_out = NULL;

	if (t.Type() != TokenType_DATA) {
		err_out = "expected TOK_DATA token";
		return 0.0f;
	}

	const char* inout = t.begin();

	// need to copy the input string to a temporary buffer
	// first - next in the fbx token stream comes ',', 
	// which fast_atof could interpret as decimal point.
#define MAX_FLOAT_LENGTH 31
	char temp[MAX_FLOAT_LENGTH + 1];
	const size_t length = static_cast<size_t>(t.end()-t.begin());
	std::copy(t.begin(),t.end(),temp);
	temp[std::min(static_cast<size_t>(MAX_FLOAT_LENGTH),length)] = '\0';

	return fast_atof(temp);
}


// ------------------------------------------------------------------------------------------------
int ParseTokenAsInt(const Token& t, const char*& err_out)
{
	err_out = NULL;

	if (t.Type() != TokenType_DATA) {
		err_out = "expected TOK_DATA token";
		return 0;
	}

	ai_assert(static_cast<size_t>(t.end() - t.begin()) > 0);

	const char* out;
	const int intval = strtol10(t.begin(),&out);
	if (out != t.end()) {
		err_out = "failed to parse ID";
		return 0;
	}

	return intval;
}


// ------------------------------------------------------------------------------------------------
std::string ParseTokenAsString(const Token& t, const char*& err_out)
{
	err_out = NULL;

	if (t.Type() != TokenType_DATA) {
		err_out = "expected TOK_DATA token";
		return "";
	}

	const size_t length = static_cast<size_t>(t.end() - t.begin());
	if(length < 2) {
		err_out = "token is too short to hold a string";
		return "";
	}

	const char* s = t.begin(), *e = t.end() - 1;
	if (*s != '\"' || *e != '\"') {
		err_out = "expected double quoted string";
		return "";
	}

	return std::string(s+1,length-2);
}

} // !FBX
} // !Assimp

#endif

