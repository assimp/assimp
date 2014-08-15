/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team

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
---------------------------------------------------------------------------
*/
/** @file Default implementation of IOSystem using the standard C file functions */

#include "AssimpPCH.h"

#include <stdlib.h>
#include "DefaultIOSystem.h"
#include "DefaultIOStream.h"

#ifdef __unix__
#include <sys/param.h>
#include <stdlib.h>
#endif

#ifdef WIN32
#define LITTLE_ENDIAN_SYSTEM
namespace {
    int DecodeUTF8(     const char*  zUTF8String,
        size_t               nUTF8StringBytes,
        wchar_t*         pUCS2Buffer,
        size_t               nUCS2BufferBytes )
    {
        unsigned char  c1 = 0,
            c2 = 0,
            c3 = 0;
        char* pIn = (char*)zUTF8String;
        char* pOut = (char*)pUCS2Buffer;
        size_t nUsed = 0;
        size_t nAllowed = nUCS2BufferBytes - sizeof(wchar_t);

        for (register size_t i = 0; i < nUTF8StringBytes; i++, pIn++)
        {
            if (*pIn == 0)
                break;

            if (((nUsed+=2) > nAllowed) && pOut)
            {
                *pOut = 0;
                return -1;
                //_DWFCORE_THROW( DWFOverflowException, L"Buffer too small." );
            }

            c1 = *pIn;

            //
            // one byte
            //
            if (c1 < 0x80)
            {

#ifdef  LITTLE_ENDIAN_SYSTEM

                *(pOut++) = c1;
                *(pOut++) = 0x00;
#else

                *(pOut++) = 0x00;
                *(pOut++) = c1;
#endif

            }
            //
            // three bytes
            //
            else if ((c1 & 0xf0) == 0xe0)
            {
                c2 = *(pIn+1);
                c3 = *(pIn+2);

#ifdef  LITTLE_ENDIAN_SYSTEM

                *(pOut++) = (((c2 & 0x03) << 6) |  (c3 & 0x3f));
                *(pOut++) = (((c1 & 0x0f) << 4) | ((c2 & 0x3c) >> 2));
#else

                *(pOut++) = (((c1 & 0x0f) << 4) | ((c2 & 0x3c) >> 2));
                *(pOut++) = (((c2 & 0x03) << 6) |  (c3 & 0x3f));
#endif

                i += 2;
                pIn += 2;
            }
            //
            // two bytes
            //
            else
            {
                c2 = *(pIn+1);

#ifdef  LITTLE_ENDIAN_SYSTEM

                *(pOut++) = (((c1 & 0x03) << 6) | (c2 & 0x3f));
                *(pOut++) = ((c1 & 0x1c) >> 2);
#else

                *(pOut++) = ((c1 & 0x1c) >> 2);
                *(pOut++) = (((c1 & 0x03) << 6) | (c2 & 0x3f));
#endif

                i++;
                pIn++;
            }
        }

        *(pOut++) = 0;
        *(pOut++) = 0;

        return (int)nUsed;
    }
}
#endif

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor. 
DefaultIOSystem::DefaultIOSystem()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor. 
DefaultIOSystem::~DefaultIOSystem()
{
	// nothing to do here
}

// maximum path length
// XXX http://insanecoding.blogspot.com/2007/11/pathmax-simply-isnt.html 
#ifdef WIN32 
    // On Windows systems, ignore PATH_MAX. This makes it possible to support long paths
    // with AssImp just by converting them to extended path form such as "\\?\D:\very long path"
    // before passing to AssImp.
    // See: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx
#   define PATHLIMIT 8192
#elif defined(PATH_MAX)
#	define PATHLIMIT PATH_MAX
#else
#	define PATHLIMIT 4096
#endif


// ------------------------------------------------------------------------------------------------
// Tests for the existence of a file at the given path.
bool DefaultIOSystem::Exists( const char* pFile) const
{
#ifdef WIN32
    wchar_t pFileW[PATHLIMIT];
    DecodeUTF8(pFile, strlen(pFile), pFileW, PATHLIMIT);
    
    FILE* file = ::_wfopen(pFileW, L"rb");
#else
    FILE* file = ::fopen( pFile, "rb");
#endif
    if( !file)
		return false;

	::fclose( file);
	return true;
}

// ------------------------------------------------------------------------------------------------
// Open a new file with a given path.
IOStream* DefaultIOSystem::Open( const char* strFile, const char* strMode)
{
	ai_assert(NULL != strFile);
	ai_assert(NULL != strMode);

#ifdef WIN32
    wchar_t pFileW[PATHLIMIT];
    DecodeUTF8(strFile, strlen(strFile), pFileW, PATHLIMIT);

    wchar_t strModeW[4];
    DecodeUTF8(strMode, strlen(strMode), strModeW, 4);
    
    FILE* file = ::_wfopen(pFileW, strModeW);
#else
	FILE* file = ::fopen( strFile, strMode);
#endif 
	if( NULL == file) 
		return NULL;

	return new DefaultIOStream(file, (std::string) strFile);
}

// ------------------------------------------------------------------------------------------------
// Closes the given file and releases all resources associated with it.
void DefaultIOSystem::Close( IOStream* pFile)
{
	delete pFile;
}

// ------------------------------------------------------------------------------------------------
// Returns the operation specific directory separator
char DefaultIOSystem::getOsSeparator() const
{
#ifndef _WIN32
	return '/';
#else
	return '\\';
#endif
}

// ------------------------------------------------------------------------------------------------
// IOSystem default implementation (ComparePaths isn't a pure virtual function)
bool IOSystem::ComparePaths (const char* one, const char* second) const
{
	return !ASSIMP_stricmp(one,second);
}

// ------------------------------------------------------------------------------------------------
// Convert a relative path into an absolute path
inline void MakeAbsolutePath (const char* in, char* _out)
{
	ai_assert(in && _out);
	char* ret;
#ifdef _WIN32
	ret = ::_fullpath(_out, in,PATHLIMIT);
#else
    	// use realpath
    	ret = realpath(in, _out);
#endif  
	if(!ret) {
		// preserve the input path, maybe someone else is able to fix
		// the path before it is accessed (e.g. our file system filter)
		DefaultLogger::get()->warn("Invalid path: "+std::string(in));
		strcpy(_out,in);
	}  
}

// ------------------------------------------------------------------------------------------------
// DefaultIOSystem's more specialized implementation
bool DefaultIOSystem::ComparePaths (const char* one, const char* second) const
{
	// chances are quite good both paths are formatted identically,
	// so we can hopefully return here already
	if( !ASSIMP_stricmp(one,second) )
		return true;

	char temp1[PATHLIMIT];
	char temp2[PATHLIMIT];
	
	MakeAbsolutePath (one, temp1);
	MakeAbsolutePath (second, temp2);

	return !ASSIMP_stricmp(temp1,temp2);
}

#undef PATHLIMIT
