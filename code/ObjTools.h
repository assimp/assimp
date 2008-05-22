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

/**	@file	ObjTools.h
 *	@brief	Some helpful templates for text parsing
 */
#ifndef OBJ_TOOLS_H_INC
#define OBJ_TOOLS_H_INC

namespace Assimp
{

/** @brief	Returns true, if token is a space on any supported platform
*	@param	token	Token to search in
*	@return	true, if token is a space			
*/
inline bool isSpace(char token)
{
	return (token == ' ' || token == '\n' || token == '\f' || token == '\r' ||
		token == '\t');
}

/**	@brief	Returns next word separated by a space
 *	@param	pBuffer	Pointer to data buffer
 *	@param	pEnd	Pointer to end of buffer
 *	@return	Pointer to next space
 */
template<class Char_T>
inline Char_T getNextWord(Char_T pBuffer, Char_T pEnd)
{
	while (pBuffer != pEnd)
	{
		if (!isSpace(*pBuffer))
			break;
		pBuffer++;
	}
	return pBuffer;
}

/**	@brief	Returns ponter a next token
 *	@param	pBuffer	Pointer to data buffer
 *	@param	pEnd	Pointer to end of buffer
 *	@return	Pointer to next token
 */
template<class Char_T>
inline Char_T getNextToken(Char_T pBuffer, Char_T pEnd)
{
	while (pBuffer != pEnd)
	{
		if (isSpace(*pBuffer))
			break;
		pBuffer++;
	}
	return getNextWord(pBuffer, pEnd);
}

/**	@brief	Skips a line
 *	@param	Iterator set to current position
 *	@param	Iterator set to end of scratch buffer for readout
 *	@param	Current linenumber in format
 *	@return	Current-iterator with new position
 */
template<class char_t>
inline char_t skipLine(char_t it, char_t end, unsigned int &uiLine)
{
	while ( it != end && *it != '\n' )
		++it;
	if ( it != end )
	{
		++it;
		++uiLine;
	}
	return it;
}

template<class char_t>
inline char_t getName( char_t it, char_t end, std::string &name )
{
	name = "";
	it = getNextToken<char_t>( it, end );
	if ( it == end )
		return end;
	
	char *pStart = &(*it);
	while ( !isSpace(*it) && it != end )
		++it;

	// Get name
	std::string strName(pStart, &(*it));
	if ( strName.empty() )
		return it;
	else
		name = strName;
	
	return it;
}

template<class char_t>
inline char_t CopyNextWord( char_t it, char_t end, char *pBuffer, size_t length )
{
	size_t index = 0;
	it = getNextWord<char_t>( it, end );
	while (!isSpace( *it ) && it != end )
	{
		pBuffer[index] = *it ;
		index++;
		if (index == length-1)
			break;
		++it;
	}
	pBuffer[index] = '\0';
	return it;
}


} // Namespace Assimp

#endif
