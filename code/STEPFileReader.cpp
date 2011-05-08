/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
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

/** @file  STEPFileReader.cpp
 *  @brief Implementation of the STEP file parser, which fills a 
 *     STEP::DB with data read from a file.
 */
#include "AssimpPCH.h"
#include "STEPFileReader.h"
#include "TinyFormatter.h"
#include "fast_atof.h"

using namespace Assimp;
namespace EXPRESS = STEP::EXPRESS;

#include <functional>

// ------------------------------------------------------------------------------------------------
// From http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

// trim from start
static inline std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1( std::ptr_fun(Assimp::IsSpace<char>))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1( std::ptr_fun(Assimp::IsSpace<char>))).base(),s.end());
	return s;
}
// trim from both ends
static inline std::string &trim(std::string &s) {
	return ltrim(rtrim(s));
}




// ------------------------------------------------------------------------------------------------
std::string AddLineNumber(const std::string& s,uint64_t line /*= LINE_NOT_SPECIFIED*/, const std::string& prefix = "") 
{
	return line == STEP::SyntaxError::LINE_NOT_SPECIFIED ? prefix+s : static_cast<std::string>( (Formatter::format(),prefix,"(line ",line,") ",s) );
}


// ------------------------------------------------------------------------------------------------
std::string AddEntityID(const std::string& s,uint64_t entity /*= ENTITY_NOT_SPECIFIED*/, const std::string& prefix = "") 
{
	return entity == STEP::TypeError::ENTITY_NOT_SPECIFIED ? prefix+s : static_cast<std::string>( (Formatter::format(),prefix,"(entity #",entity,") ",s));
}


// ------------------------------------------------------------------------------------------------
STEP::SyntaxError::SyntaxError (const std::string& s,uint64_t line /* = LINE_NOT_SPECIFIED */)
: DeadlyImportError(AddLineNumber(s,line))
{

}

// ------------------------------------------------------------------------------------------------
STEP::TypeError::TypeError (const std::string& s,uint64_t entity /* = ENTITY_NOT_SPECIFIED */,uint64_t line /*= LINE_NOT_SPECIFIED*/)
: DeadlyImportError(AddLineNumber(AddEntityID(s,entity),line))
{

}


// ------------------------------------------------------------------------------------------------
STEP::DB* STEP::ReadFileHeader(boost::shared_ptr<IOStream> stream)
{
	boost::shared_ptr<StreamReaderLE> reader = boost::shared_ptr<StreamReaderLE>(new StreamReaderLE(stream));
	std::auto_ptr<STEP::DB> db = std::auto_ptr<STEP::DB>(new STEP::DB(reader));

	LineSplitter& splitter = db->GetSplitter();
	if (!splitter || *splitter != "ISO-10303-21;") {
		throw STEP::SyntaxError("expected magic token: ISO-10303-21",1);
	}

	HeaderInfo& head = db->GetHeader();
	for(++splitter; splitter; ++splitter) {
		const std::string& s = *splitter;
		if (s == "DATA;") {
			// here we go, header done, start of data section
			++splitter;
			break;
		}

		// want one-based line numbers for human readers, so +1
		const uint64_t line = splitter.get_index()+1;

		if (s.substr(0,11) == "FILE_SCHEMA") {
			const char* sz = s.c_str()+11;
			SkipSpaces(sz,&sz);
			std::auto_ptr< const EXPRESS::DataType > schema = std::auto_ptr< const EXPRESS::DataType >( EXPRESS::DataType::Parse(sz) );

			// the file schema should be a regular list entity, although it usually contains exactly one entry
			// since the list itself is contained in a regular parameter list, we actually have
			// two nested lists.
			const EXPRESS::LIST* list = dynamic_cast<const EXPRESS::LIST*>(schema.get());
			if (list && list->GetSize()) {
				list = dynamic_cast<const EXPRESS::LIST*>( (*list)[0] );
				if (!list) {
					throw STEP::SyntaxError("expected FILE_SCHEMA to be a list",line);
				}

				// XXX need support for multiple schemas?
				if (list->GetSize() > 1)	{
					DefaultLogger::get()->warn(AddLineNumber("multiple schemas currently not supported",line));
				}
				const EXPRESS::STRING* string;
				if (!list->GetSize() || !(string=dynamic_cast<const EXPRESS::STRING*>( (*list)[0] ))) {
					throw STEP::SyntaxError("expected FILE_SCHEMA to contain a single string literal",line);
				}
				head.fileSchema =  *string;
			}
		}

		// XXX handle more header fields
	}

	return db.release();
}


// ------------------------------------------------------------------------------------------------
void STEP::ReadFile(DB& db,const EXPRESS::ConversionSchema& scheme)
{
	db.SetSchema(scheme);

	const DB::ObjectMap& map = db.GetObjects();
	LineSplitter& splitter = db.GetSplitter();
	for(; splitter; ++splitter) {
		const std::string& s = *splitter;
		if (s == "ENDSEC;") {
			break;
		}

		// want one-based line numbers for human readers, so +1
		const uint64_t line = splitter.get_index()+1;

		// LineSplitter already ignores empty lines
		ai_assert(s.length());
		if (s[0] != '#') {
			DefaultLogger::get()->warn(AddLineNumber("expected token \'#\'",line));
			continue;
		}

		// ---
		// extract id, entity class name and argument string,
		// but don't create the actual object yet. 
		// ---

		const std::string::size_type n0 = s.find_first_of('=');
		if (n0 == std::string::npos) {
			DefaultLogger::get()->warn(AddLineNumber("expected token \'=\'",line));
			continue;
		}

		const uint64_t id = strtoul10_64(s.substr(1,n0-1).c_str());
		if (!id) {
			DefaultLogger::get()->warn(AddLineNumber("expected positive, numeric entity id",line));
			continue;
		}

		const std::string::size_type n1 = s.find_first_of('(',n0);
		if (n1 == std::string::npos) {
			DefaultLogger::get()->warn(AddLineNumber("expected token \'(\'",line));
			continue;
		}

		const std::string::size_type n2 = s.find_last_of(')');
		if (n2 == std::string::npos || n2 < n1) {
			DefaultLogger::get()->warn(AddLineNumber("expected token \')\'",line));
			continue;
		}

		if (map.find(id) != map.end()) {
			DefaultLogger::get()->warn(AddLineNumber((Formatter::format(),"an object with the id #",id," already exists"),line));
		}

		std::string type = s.substr(n0+1,n1-n0-1);
		trim(type);
		std::transform( type.begin(), type.end(), type.begin(), &Assimp::ToLower<char>  );
		db.InternInsert(boost::shared_ptr<LazyObject>(new LazyObject(db,id,line,type,s.substr(n1,n2-n1+1))));
	}

	if (!splitter) {
		DefaultLogger::get()->warn("STEP: ignoring unexpected EOF");
	}

	if ( !DefaultLogger::isNullLogger() ){
		DefaultLogger::get()->debug((Formatter::format(),"STEP: got ",map.size()," object records"));
	}
}


// ------------------------------------------------------------------------------------------------
const EXPRESS::DataType* EXPRESS::DataType::Parse(const char*& inout,uint64_t line, const EXPRESS::ConversionSchema* schema /*= NULL*/)
{
	const char* cur = inout;
	SkipSpaces(&cur);

	if (*cur == ',' || IsSpaceOrNewLine(*cur)) {
		throw STEP::SyntaxError("unexpected token, expected parameter",line);
	}

	// just skip over constructions such as IFCPLANEANGLEMEASURE(0.01) and read only the value
	if (schema) {
		bool ok = false;
		for(const char* t = cur; *t && *t != ')' && *t != ','; ++t) {
			if (*t=='(') {
				if (!ok) {
					break;
				}
				for(--t;IsSpace(*t);--t);
				std::string s(cur,static_cast<size_t>(t-cur+1));
				std::transform(s.begin(),s.end(),s.begin(),&ToLower<char> );
				if (schema->IsKnownToken(s)) {
					for(cur = t+1;*cur++ != '(';);
					const EXPRESS::DataType* const dt = Parse(cur);
					inout = *cur ? cur+1 : cur;
					return dt;
				}
				break;
			}
			else if (!IsSpace(*t)) {
				ok = true;
			}
		}
	}

	if (*cur == '*' ) {
		inout = cur+1;
		return new EXPRESS::ISDERIVED();
	}
	else if (*cur == '$' ) {
		inout = cur+1;
		return new EXPRESS::UNSET();
	}
	else if (*cur == '(' ) {
		// start of an aggregate, further parsing is done by the LIST factory constructor
		inout = cur;
		return EXPRESS::LIST::Parse(inout,line,schema);
	}
	else if (*cur == '.' ) {
		// enum (includes boolean)
		const char* start = ++cur;
		for(;*cur != '.';++cur) {
			if (*cur == '\0') {
				throw STEP::SyntaxError("enum not closed",line);
			}
		}
		inout = cur+1;
		return new EXPRESS::ENUMERATION( std::string(start, static_cast<size_t>(cur-start) ));
	}
	else if (*cur == '#' ) {
		// object reference
		return new EXPRESS::ENTITY(strtoul10_64(++cur,&inout));
	}
	else if (*cur == '\'' ) {
		// string literal
		const char* start = ++cur;
		for(;*cur != '\'';++cur) {
			if (*cur == '\0') {
				throw STEP::SyntaxError("string literal not closed",line);
			}
		}
		if (cur[1]=='\'') {
			for(cur+=2;*cur != '\'';++cur) {
				if (*cur == '\0') {
					throw STEP::SyntaxError("string literal not closed",line);
				}
			}
		}
		inout = cur+1;
		return new EXPRESS::STRING( std::string(start, static_cast<size_t>(cur-start) ));
	}
	else if (*cur == '\"' ) {
		throw STEP::SyntaxError("binary data not supported yet",line);
	}

	// else -- must be a number. if there is a decimal dot in it,
	// parse it as real value, otherwise as integer.
	const char* start = cur;
	for(;*cur  && *cur != ',' && *cur != ')' && !IsSpace(*cur);++cur) {
		if (*cur == '.') {
			// XXX many STEP files contain extremely accurate data, float's precision may not suffice in many cases
			float f;
			inout = fast_atof_move(start,f);
			return new EXPRESS::REAL(f);
		}
	}

	bool neg = false;
	if (*start == '-') {
		neg = true;
		++start;
	}
	else if (*start == '+') {
		++start;
	}
	int64_t num = static_cast<int64_t>( strtoul10_64(start,&inout) );
	return new EXPRESS::INTEGER(neg?-num:num);
}


// ------------------------------------------------------------------------------------------------
const EXPRESS::LIST* EXPRESS::LIST::Parse(const char*& inout,uint64_t line, const EXPRESS::ConversionSchema* schema /*= NULL*/)
{
	std::auto_ptr<EXPRESS::LIST> list(new EXPRESS::LIST());
	EXPRESS::LIST::MemberList& members = list->members;

	const char* cur = inout;
	if (*cur++ != '(') {
		throw STEP::SyntaxError("unexpected token, expected \'(\' token at beginning of list",line);
	}

	// estimate the number of items upfront - lists can grow large
	size_t count = 1;
	for(const char* c=cur; *c && *c != ')'; ++c) {
		count += (*c == ',' ? 1 : 0);
	}

	members.reserve(count);

	for(;;++cur) {
		if (!*cur) {
			throw STEP::SyntaxError("unexpected end of line while reading list");
		}
		SkipSpaces(cur,&cur);
		if (*cur == ')') {
			break;
		}
		
		members.push_back( boost::shared_ptr<const EXPRESS::DataType>(EXPRESS::DataType::Parse(cur,line,schema)));
		SkipSpaces(cur,&cur);

		if (*cur != ',') {
			if (*cur == ')') {
				break;
			}
			throw STEP::SyntaxError("unexpected token, expected \',\' or \')\' token after list element",line);
		}
	}

	inout = cur+1;
	return list.release();
}

// ------------------------------------------------------------------------------------------------
STEP::LazyObject::LazyObject(DB& db, uint64_t id,uint64_t line,const std::string& type,const std::string& args) 
	: db(db)
	, id(id)
	, line(line)
	, type(type)
	, obj()
	// need to initialize this upfront, otherwise the destructor
	// will crash if an exception is thrown in the c'tor
	, conv_args() 
{
	const char* arg = args.c_str();
	conv_args = EXPRESS::LIST::Parse(arg,line,&db.GetSchema());

	// find any external references and store them in the database.
	// this helps us emulate STEPs INVERSE fields.
	for (size_t i = 0; i < conv_args->GetSize(); ++i) {
		const EXPRESS::DataType* t = conv_args->operator [](i);
		if (const EXPRESS::ENTITY* e = t->ToPtr<EXPRESS::ENTITY>()) {
			db.MarkRef(*e,id);
		}
	}
}

// ------------------------------------------------------------------------------------------------
STEP::LazyObject::~LazyObject() 
{
	// 'obj' always remains in our possession, so there is 
	// no need for a smart pointer type.
	delete obj;
	delete conv_args;
}


// ------------------------------------------------------------------------------------------------
void STEP::LazyObject::LazyInit() const
{
	const EXPRESS::ConversionSchema& schema = db.GetSchema();
	STEP::ConvertObjectProc proc = schema.GetConverterProc(type);

	if (!proc) {
		throw STEP::TypeError("unknown object type: " + type,id,line);
	}

	// if the converter fails, it should throw an exception, but it should never return NULL
	try {
		obj = proc(db,*conv_args);
	}
	catch(const TypeError& t) {
		// augment line and entity information
		throw TypeError(t.what(),id,line);
	}
	++db.evaluated_count;
	ai_assert(obj);

	// store the original id in the object instance
	obj->SetID(id);
}
