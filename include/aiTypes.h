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
#include <string>
extern "C" {
#endif

/** Maximum dimension for strings, ASSIMP strings are zero terminated */
const size_t MAXLEN = 1024;

// ---------------------------------------------------------------------------
/** Represents a two-dimensional vector. 
*/
// ---------------------------------------------------------------------------
typedef struct aiVector2D
{
#ifdef __cplusplus
	aiVector2D () : x(0.0f), y(0.0f) {}
	aiVector2D (float _x, float _y) : x(_x), y(_y) {}
	aiVector2D (const aiVector2D& o) : x(o.x), y(o.y) {}
	
#endif // __cplusplus

	float x, y;
} aiVector2D_t;

// aiVector3D type moved to separate header due to size of operators

// aiQuaternion type moved to separate header due to size of operators

// aiMatrix4x4 type moved to separate header due to size of operators

// ---------------------------------------------------------------------------
/** Represents a color in Red-Green-Blue space. 
*/
// ---------------------------------------------------------------------------
typedef struct aiColor3D
{
#ifdef __cplusplus
	aiColor3D () : r(0.0f), g(0.0f), b(0.0f) {}
	aiColor3D (float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
	aiColor3D (const aiColor3D& o) : r(o.r), g(o.g), b(o.b) {}
	
#endif // __cplusplus

	float r, g, b;
} aiColor3D_t;


// ---------------------------------------------------------------------------
/** Represents a color in Red-Green-Blue space including an 
*   alpha component. 
*/
// ---------------------------------------------------------------------------
typedef struct aiColor4D
{
#ifdef __cplusplus
	aiColor4D () : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
	aiColor4D (float _r, float _g, float _b, float _a) 
		: r(_r), g(_g), b(_b), a(_a) {}
	aiColor4D (const aiColor4D& o) 
		: r(o.r), g(o.g), b(o.b), a(o.a) {}
	
#endif // __cplusplus

	float r, g, b, a;
} aiColor4D_t;


// ---------------------------------------------------------------------------
/** Represents a string, zero byte terminated 
*/
// ---------------------------------------------------------------------------
typedef struct aiString
{
#ifdef __cplusplus
	inline aiString() :
		length(0) 
	{
		// empty
	}

	inline aiString(const aiString& rOther) : 
		length(rOther.length) 
	{
		memcpy( data, rOther.data, rOther.length);
		this->data[this->length] = '\0';
	}

	void Set( const std::string& pString)
	{
		if( pString.length() > MAXLEN - 1)
			return;
		length = pString.length();
		memcpy( data, pString.c_str(), length);
		data[length] = 0;
	}
#endif // __cplusplus

	size_t length;
	char data[MAXLEN];
} aiString_t;


// ---------------------------------------------------------------------------
/**	Standard return type for all library functions.
*
* To check whether a function failed or not check against
* AI_SUCCESS.
*/
// ---------------------------------------------------------------------------
enum aiReturn
{
	AI_SUCCESS = 0x0,
	AI_FAILURE = -0x1,
	AI_INVALIDFILE = -0x2,
	AI_OUTOFMEMORY = -0x3,
	AI_INVALIDARG = -0x4
};

#ifdef __cplusplus
}
#endif
#endif

