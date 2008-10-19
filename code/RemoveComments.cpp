/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
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

/** @file Defines a helper class, "CommentRemover", which can be
 *  used to remove comments (single and multi line) from a text file.
 */

#include "AssimpPCH.h"
#include "RemoveComments.h"
#include "ParsingUtils.h"

namespace Assimp
{

// ------------------------------------------------------------------------------------------------
void CommentRemover::RemoveLineComments(const char* szComment,
	char* szBuffer, char chReplacement /* = ' ' */)
{
	// validate parameters
	ai_assert(NULL != szComment && NULL != szBuffer && *szComment);

	const size_t len = ::strlen(szComment);
	while (*szBuffer)
	{
		if (!::strncmp(szBuffer,szComment,len))
		{
			while (!IsLineEnd(*szBuffer))
				*szBuffer++ = chReplacement;
		}
		++szBuffer;
	}
}

// ------------------------------------------------------------------------------------------------
void CommentRemover::RemoveMultiLineComments(const char* szCommentStart,
	const char* szCommentEnd,char* szBuffer,
	char chReplacement)
{
	// validate parameters
	ai_assert(NULL != szCommentStart && NULL != szCommentEnd &&
		NULL != szBuffer && *szCommentStart && *szCommentEnd);

	const size_t len  = ::strlen(szCommentEnd);
	const size_t len2 = ::strlen(szCommentStart);

	while (*szBuffer)
	{
		if (!::strncmp(szBuffer,szCommentStart,len2))
		{
			while (*szBuffer)
			{
				if (!::strncmp(szBuffer,szCommentEnd,len))
				{
					for (unsigned int i = 0; i < len;++i)
						*szBuffer++ = chReplacement;
					
					break;
				}
			*szBuffer++ = chReplacement;
			}
			if (!(*szBuffer))return;
			continue;
		}
		++szBuffer;
	}
}

}; // !! Assimp
