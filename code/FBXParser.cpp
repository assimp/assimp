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
	// signal parse error, this is always unrecoverable. Throws DeadlyImportError.
	void ParseError(const std::string& message, const Token& token)
	{
		throw DeadlyImportError(Util::AddTokenText("FBX-Parser",message,&token));
	}

	// ------------------------------------------------------------------------------------------------
	void ParseError(const std::string& message, const Element* element = NULL)
	{
		if(element) {
			ParseError(message,element->KeyToken());
		}
		throw DeadlyImportError("FBX-Parser " + message);
	}


	// ------------------------------------------------------------------------------------------------
	// print warning, do return
	void ParseWarning(const std::string& message, const Token& token)
	{
		if(DefaultLogger::get()) {
			DefaultLogger::get()->warn(Util::AddTokenText("FBX-Parser",message,&token));
		}
	}

	// ------------------------------------------------------------------------------------------------
	void ParseWarning(const std::string& message, const Element* element = NULL)
	{
		if(element) {
			ParseWarning(message,element->KeyToken());
			return;
		}
		if(DefaultLogger::get()) {
			DefaultLogger::get()->warn("FBX-Parser: " + message);
		}
	}

	// ------------------------------------------------------------------------------------------------
	void ParseError(const std::string& message, TokenPtr token)
	{
		if(token) {
			ParseError(message, *token);
		}
		ParseError(message);
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
		ParseError("unexpected end of file");
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
Parser::Parser (const TokenList& tokens, bool is_binary)
: tokens(tokens)
, last()
, current()
, cursor(tokens.begin())
, is_binary(is_binary)
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

	if(t.IsBinary())
	{
		const char* data = t.begin();
		if (data[0] != 'L') {
			err_out = "failed to parse ID, unexpected data type, expected L(ong) (binary)";
			return 0L;
		}

		ai_assert(t.end() - data == 9);

		BE_NCONST uint64_t id = *reinterpret_cast<const uint64_t*>(data+1);
		AI_SWAP8(id);
		return id;
	}

	// XXX: should use size_t here
	unsigned int length = static_cast<unsigned int>(t.end() - t.begin());
	ai_assert(length > 0);

	const char* out;
	const uint64_t id = strtoul10_64(t.begin(),&out,&length);
	if (out > t.end()) {
		err_out = "failed to parse ID (text)";
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

	if(t.IsBinary())
	{
		const char* data = t.begin();
		if (data[0] != 'L') {
			err_out = "failed to parse ID, unexpected data type, expected L(ong) (binary)";
			return 0;
		}

		ai_assert(t.end() - data == 9);
		BE_NCONST uint64_t id = *reinterpret_cast<const uint64_t*>(data+1);
		AI_SWAP8(id);
		return id;
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

	if(t.IsBinary())
	{
		const char* data = t.begin();
		if (data[0] != 'F' && data[0] != 'D') {
			err_out = "failed to parse F(loat) or D(ouble), unexpected data type (binary)";
			return 0.0f;
		}

		if (data[0] == 'F') {
			ai_assert(t.end() - data == 5);
			// no byte swapping needed for ieee floats
			return *reinterpret_cast<const float*>(data+1);
		}
		else {
			ai_assert(t.end() - data == 9);
			// no byte swapping needed for ieee floats
			return static_cast<float>(*reinterpret_cast<const double*>(data+1));
		}
	}

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

	if(t.IsBinary())
	{
		const char* data = t.begin();
		if (data[0] != 'I') {
			err_out = "failed to parse I(nt), unexpected data type (binary)";
			return 0;
		}

		ai_assert(t.end() - data == 5);
		BE_NCONST int32_t ival = *reinterpret_cast<const int32_t*>(data+1);
		AI_SWAP4(ival);
		return static_cast<int>(ival);
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

	if(t.IsBinary())
	{
		const char* data = t.begin();
		if (data[0] != 'S') {
			err_out = "failed to parse S(tring), unexpected data type (binary)";
			return "";
		}

		ai_assert(t.end() - data >= 5);

		// read string length
		BE_NCONST int32_t len = *reinterpret_cast<const int32_t*>(data+1);
		AI_SWAP4(len);

		ai_assert(t.end() - data == 5 + len);
		return std::string(data + 5, len);
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


// ------------------------------------------------------------------------------------------------
// read an array of float3 tuples
void ParseVectorDataArray(std::vector<aiVector3D>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// may throw bad_alloc if the input is rubbish, but this need
	// not to be prevented - importing would fail but we wouldn't
	// crash since assimp handles this case properly.
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	if (a.Tokens().size() % 3 != 0) {
		ParseError("number of floats is not a multiple of three (3)",&el);
	}
	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		aiVector3D v;
		v.x = ParseTokenAsFloat(**it++);
		v.y = ParseTokenAsFloat(**it++);
		v.z = ParseTokenAsFloat(**it++);

		out.push_back(v);
	}
}


// ------------------------------------------------------------------------------------------------
// read an array of color4 tuples
void ParseVectorDataArray(std::vector<aiColor4D>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	//  see notes in ParseVectorDataArray() above
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	if (a.Tokens().size() % 4 != 0) {
		ParseError("number of floats is not a multiple of four (4)",&el);
	}
	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		aiColor4D v;
		v.r = ParseTokenAsFloat(**it++);
		v.g = ParseTokenAsFloat(**it++);
		v.b = ParseTokenAsFloat(**it++);
		v.a = ParseTokenAsFloat(**it++);

		out.push_back(v);
	}
}


// ------------------------------------------------------------------------------------------------
// read an array of float2 tuples
void ParseVectorDataArray(std::vector<aiVector2D>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// see notes in ParseVectorDataArray() above
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	if (a.Tokens().size() % 2 != 0) {
		ParseError("number of floats is not a multiple of two (2)",&el);
	}
	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		aiVector2D v;
		v.x = ParseTokenAsFloat(**it++);
		v.y = ParseTokenAsFloat(**it++);

		out.push_back(v);
	}
}


// ------------------------------------------------------------------------------------------------
// read an array of ints
void ParseVectorDataArray(std::vector<int>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// see notes in ParseVectorDataArray()
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		const int ival = ParseTokenAsInt(**it++);
		out.push_back(ival);
	}
}


// ------------------------------------------------------------------------------------------------
// read an array of floats
void ParseVectorDataArray(std::vector<float>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// see notes in ParseVectorDataArray()
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		const float ival = ParseTokenAsFloat(**it++);
		out.push_back(ival);
	}
}


// ------------------------------------------------------------------------------------------------
// read an array of uints
void ParseVectorDataArray(std::vector<unsigned int>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// see notes in ParseVectorDataArray()
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		const int ival = ParseTokenAsInt(**it++);
		if(ival < 0) {
			ParseError("encountered negative integer index");
		}
		out.push_back(static_cast<unsigned int>(ival));
	}
}


// ------------------------------------------------------------------------------------------------
// read an array of uint64_ts
void ParseVectorDataArray(std::vector<uint64_t>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// see notes in ParseVectorDataArray()
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		const uint64_t ival = ParseTokenAsID(**it++);
		
		out.push_back(ival);
	}
}


// ------------------------------------------------------------------------------------------------
aiMatrix4x4 ReadMatrix(const Element& element)
{
	std::vector<float> values;
	ParseVectorDataArray(values,element);

	if(values.size() != 16) {
		ParseError("expected 16 matrix elements");
	}

	aiMatrix4x4 result;


	result.a1 = values[0];
	result.a2 = values[1];
	result.a3 = values[2];
	result.a4 = values[3];

	result.b1 = values[4];
	result.b2 = values[5];
	result.b3 = values[6];
	result.b4 = values[7];

	result.c1 = values[8];
	result.c2 = values[9];
	result.c3 = values[10];
	result.c4 = values[11];

	result.d1 = values[12];
	result.d2 = values[13];
	result.d3 = values[14];
	result.d4 = values[15];

	result.Transpose();
	return result;
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsString() with ParseError handling
std::string ParseTokenAsString(const Token& t)
{
	const char* err;
	const std::string& i = ParseTokenAsString(t,err);
	if(err) {
		ParseError(err,t);
	}
	return i;
}


// ------------------------------------------------------------------------------------------------
// extract a required element from a scope, abort if the element cannot be found
const Element& GetRequiredElement(const Scope& sc, const std::string& index, const Element* element /*= NULL*/) 
{
	const Element* el = sc[index];
	if(!el) {
		ParseError("did not find required element \"" + index + "\"",element);
	}
	return *el;
}


// ------------------------------------------------------------------------------------------------
// extract required compound scope
const Scope& GetRequiredScope(const Element& el)
{
	const Scope* const s = el.Compound();
	if(!s) {
		ParseError("expected compound scope",&el);
	}

	return *s;
}


// ------------------------------------------------------------------------------------------------
// get token at a particular index
const Token& GetRequiredToken(const Element& el, unsigned int index)
{
	const TokenList& t = el.Tokens();
	if(index >= t.size()) {
		ParseError(Formatter::format( "missing token at index " ) << index,&el);
	}

	return *t[index];
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsID() with ParseError handling
uint64_t ParseTokenAsID(const Token& t) 
{
	const char* err;
	const uint64_t i = ParseTokenAsID(t,err);
	if(err) {
		ParseError(err,t);
	}
	return i;
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsDim() with ParseError handling
size_t ParseTokenAsDim(const Token& t)
{
	const char* err;
	const size_t i = ParseTokenAsDim(t,err);
	if(err) {
		ParseError(err,t);
	}
	return i;
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsFloat() with ParseError handling
float ParseTokenAsFloat(const Token& t)
{
	const char* err;
	const float i = ParseTokenAsFloat(t,err);
	if(err) {
		ParseError(err,t);
	}
	return i;
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsInt() with ParseError handling
int ParseTokenAsInt(const Token& t)
{
	const char* err;
	const int i = ParseTokenAsInt(t,err);
	if(err) {
		ParseError(err,t);
	}
	return i;
}



} // !FBX
} // !Assimp

#endif

