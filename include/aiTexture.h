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

/** @file Defines texture helper structures for the library
 *
 * Used for file formats which embedd their textures into the model file.
 * Supported are both normal textures, which are stored as uncompressed
 * pixels, and "compressed" textures, which are stored in a file format
 * such as PNG or TGA.
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


#include "./Compiler/pushpack1.h"

// ---------------------------------------------------------------------------
/** Helper structure to represent a texel in ARGB8888 format
* 
* Used by aiTexture
*/
// ---------------------------------------------------------------------------
struct aiTexel
{
	unsigned char b,g,r,a;

#ifdef __cplusplus
	//! Comparison operator
	bool operator== (const aiTexel& other) const
	{
		return b == other.b && r == other.r &&
			g == other.g && a == other.a;
	}

	//! Negative comparison operator
	bool operator!= (const aiTexel& other) const
	{
		return b != other.b || r != other.r ||
			g != other.g || a != other.a;
	}
#endif // __cplusplus

} PACK_STRUCT;

#include "./Compiler/poppack1.h"

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
	 * If mHeight is zero the texture is compressed in a format
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
	 * E.g. 'dds\0', 'pcx\0'.  All characters are lower-case.
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
		: mWidth  (0)
		, mHeight (0)
		, pcData  (NULL)
	{
		achFormatHint[0] = 0;
		achFormatHint[1] = 0;
		achFormatHint[2] = 0;
		achFormatHint[3] = 0;
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
