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

#ifndef AI_TYPES_H_INC
#define AI_TYPES_H_INC

#include <sys/types.h>
#include <memory.h>

#if (defined _MSC_VER)
#	include "Compiler/VisualStudio/stdint.h"
#endif // (defined _MSC_VER)

#include "aiVector3D.h"
#include "aiMatrix3x3.h"
#include "aiMatrix4x4.h"
#include "aiVector3D.inl"
#include "aiMatrix3x3.inl"
#include "aiMatrix4x4.inl"

#ifdef __cplusplus
#	include <string>
extern "C" {
#	define C_STRUCT
#else
#	if (defined ASSIMP_DOXYGEN_BUILD)
#		define C_STRUCT
#	else
#		define C_STRUCT struct
#	endif
#endif

/** Maximum dimension for strings, ASSIMP strings are zero terminated */
#ifdef __cplusplus
const size_t MAXLEN = 1024;
#else
#	define MAXLEN 1024
#endif

// ---------------------------------------------------------------------------
/** Represents a two-dimensional vector. 
*/
// ---------------------------------------------------------------------------
struct aiVector2D
{
#ifdef __cplusplus
	aiVector2D () : x(0.0f), y(0.0f) {}
	aiVector2D (float _x, float _y) : x(_x), y(_y) {}
	aiVector2D (const aiVector2D& o) : x(o.x), y(o.y) {}
	
#endif // !__cplusplus

	//! X and y coordinates
	float x, y;
} ;

// aiVector3D type moved to separate header due to size of operators
// aiQuaternion type moved to separate header due to size of operators
// aiMatrix4x4 type moved to separate header due to size of operators

// ---------------------------------------------------------------------------
/** Represents a color in Red-Green-Blue space. 
*/
// ---------------------------------------------------------------------------
struct aiColor3D
{
#ifdef __cplusplus
	aiColor3D () : r(0.0f), g(0.0f), b(0.0f) {}
	aiColor3D (float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
	aiColor3D (const aiColor3D& o) : r(o.r), g(o.g), b(o.b) {}
	
#endif // !__cplusplus

	//! Red, green and blue color values
	float r, g, b;
};


// ---------------------------------------------------------------------------
/** Represents a color in Red-Green-Blue space including an 
*   alpha component. 
*/
// ---------------------------------------------------------------------------
struct aiColor4D
{
#ifdef __cplusplus
	aiColor4D () : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
	aiColor4D (float _r, float _g, float _b, float _a) 
		: r(_r), g(_g), b(_b), a(_a) {}
	aiColor4D (const aiColor4D& o) 
		: r(o.r), g(o.g), b(o.b), a(o.a) {}
	
#endif // !__cplusplus

	//! Red, green, blue and alpha color values
	float r, g, b, a;
};


// ---------------------------------------------------------------------------
/** Represents a string, zero byte terminated 
*/
// ---------------------------------------------------------------------------
struct aiString
{
#ifdef __cplusplus
	inline aiString() :
		length(0) 
	{
		// empty
	}

	//! construction from a given std::string
	inline aiString(const aiString& rOther) : 
		length(rOther.length) 
	{
		memcpy( data, rOther.data, rOther.length);
		this->data[this->length] = '\0';
	}

	//! copy a std::string to the aiString
	void Set( const std::string& pString)
	{
		if( pString.length() > MAXLEN - 1)
			return;
		length = pString.length();
		memcpy( data, pString.c_str(), length);
		data[length] = 0;
	}

	//! comparison operator
	bool operator==(const aiString& other) const
	{
		return  (this->length == other.length &&
				 0 == strcmp(this->data,other.data));
	}

	//! inverse comparison operator
	bool operator!=(const aiString& other) const
	{
		return  (this->length != other.length ||
				 0 != strcmp(this->data,other.data));
	}


#endif // !__cplusplus

	//! Length of the string excluding the terminal 0
	size_t length;

	//! String buffer. Size limit is MAXLEN
	char data[MAXLEN];
} ;


// ---------------------------------------------------------------------------
/**	Standard return type for all library functions.
*
* To check whether a function failed or not check against
* AI_SUCCESS.
*/
// ---------------------------------------------------------------------------

enum aiReturn
{
	//! Indicates that a function was successful
	AI_SUCCESS = 0x0,
	//! Indicates that a function failed
	AI_FAILURE = -0x1,
	//! Indicates that a file was invalid
	AI_INVALIDFILE = -0x2,
	//! Indicates that not enough memory was available
	//! to perform the requested operation
	AI_OUTOFMEMORY = -0x3,
	//! Indicates that an illegal argument has been
	//! passed to a function. This is rarely used,
	//! most functions assert in this case.
	AI_INVALIDARG = -0x4
};


#ifdef __cplusplus
}
#endif //!  __cplusplus
#endif //!! include guard

