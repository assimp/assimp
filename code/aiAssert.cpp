
#include "AssimpPCH.h"

#include "../include/aiAssert.h"
#ifdef _WIN32
#ifndef __GNUC__
#  include "crtdbg.h"
#endif //ndef gcc
#endif

// Set a breakpoint using win32, else line, file and message will be returned and progam ends with 
// errrocode = 1
void Assimp::aiAssert (bool expression, const std::string &message, unsigned int uiLine, const std::string &file)
{
	if (!expression)
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
