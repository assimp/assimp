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

} // Namespace Assimp

#endif
