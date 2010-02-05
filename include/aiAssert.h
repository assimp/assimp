/** @file aiAssert.h
 */
#ifndef AI_DEBUG_H_INC
#define AI_DEBUG_H_INC

#include <string>

#ifndef __cplusplus
#error This header requires C++ to be used.
#endif

namespace Assimp {

//!	\brief	ASSIMP specific assertion test, only works in debug mode
//!	\param	uiLine	Line in file
//!	\param	file	Source file
AI_WONT_RETURN void aiAssert(const std::string &message, unsigned int uiLine, const std::string &file);


//!	\def	ai_assert
//!	\brief	ASSIMP specific assertion test
#ifdef DEBUG  
#  define	ai_assert(expression) if( !(expression)) Assimp::aiAssert( #expression, __LINE__, __FILE__);
#else
#  define	ai_assert(expression)
#endif

}	// Namespace Assimp

#endif
