
/** 
 ** This file is part of the Free Asset Import Library ASSIMP.
 ** -------------------------------------------------------------------------
 **
 ** ASSIMP is free software: you can redistribute it and/or modify it 
 ** under the terms of the GNU Lesser General Public License as published 
 ** by the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** ASSIMP is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public License
 ** along with ASSIMP. If not, see <http://www.gnu.org/licenses/>.
 **
 ** -------------------------------------------------------------------------
 **/

/** @file Defines texture helper structures for the library
 *
 * Used for file formats that embedd their textures into the file
 */

#ifndef AI_TEXTURE_H_INC
#define AI_TEXTURE_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif


// ---------------------------------------------------------------------------
/** \def AI_MAKE_EMBEDDED_TEXNAME
 *  Used to build the reserved path name used by the material system to 
 *  reference textures that are embedded into their corresponding
 *  model files. The parameter specifies the index of the texture
 *  (zero-based, in the aiScene::mTextures array)
 */
// ---------------------------------------------------------------------------
#if (!defined AI_MAKE_EMBEDDED_TEXNAME)
#	define AI_MAKE_EMBEDDED_TEXNAME(_n_) "*" # _n_
#endif

// ugly compiler dependent packing stuff
#if defined(_MSC_VER) ||  defined(__BORLANDC__) ||	defined (__BCPLUSPLUS__)
#	pragma pack(push,1)
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error Compiler not supported. Never do this again.
#endif

// ---------------------------------------------------------------------------
/** Helper structure to represent a texel in ARGB8888 format
* 
* Used by aiTexture
*/
// ---------------------------------------------------------------------------
struct aiTexel
{
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
} PACK_STRUCT;

// reset packing to the original value
#if defined(_MSC_VER) ||  defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma pack( pop )
#endif
#undef PACK_STRUCT

// ---------------------------------------------------------------------------
/** Helper structure to describe an embedded texture
* 
* Normally textures are contained in external files but some file formats
* do embedd them. Embedded
*/
// ---------------------------------------------------------------------------
struct aiTexture
{
	/** Width of the texture, in pixels
	 *
	 * If mHeight is zero the texture is compressedin a format
	 * like JPEG. In this case mWidth specifies the size of the
	 * memory area pcData is pointing to, in bytes.
	 */
	unsigned int mWidth;

	/** Height of the texture, in pixels
	 *
	 * If this value is zero, pcData points to an compressed texture
	 * in an unknown format (e.g. JPEG).
	 */
	unsigned int mHeight;

	/** A hint from the loader to make it easier for applications
	 *  to determine the type of embedded compressed textures.
	 *
	 * If mHeight != 0 this member is undefined. Otherwise it
	 * will be set to '\0\0\0\0' if the loader has no additional
	 * information about the texture file format used OR the
	 * file extension of the format without a leading dot.
	 * E.g. 'dds\0', 'pcx\0'
	 */
	char achFormatHint[4];

	/** Data of the texture.
	 *
	 * Points to an array of mWidth * mHeight aiTexel's.
	 * The format of the texture data is always ARGB8888 to
	 * make the implementation for user of the library as easy
	 * as possible. If mHeight = 0 this is a pointer to a memory
	 * buffer of size mWidth containing the compressed texture
	 * data. Good luck, have fun!
	 */
	aiTexel* pcData;

#ifdef __cplusplus

	// Construction
	aiTexture ()
		: mHeight(0), mWidth(0), pcData(NULL)
	{
		achFormatHint[0] = '\0';
		achFormatHint[1] = '\0';
		achFormatHint[2] = '\0';
		achFormatHint[3] = '\0';
	}

	// Destruction
	~aiTexture ()
	{
		delete[] pcData;
	}
#endif
};

#ifdef __cplusplus
}
#endif

#endif // AI_TEXTURE_H_INC