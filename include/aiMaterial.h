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

/** @file Defines the material system of the library
 *
 */

#ifndef AI_MATERIAL_H_INC
#define AI_MATERIAL_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default material name
#define AI_DEFAULT_MATERIAL_NAME "aiDefaultMat"

// ---------------------------------------------------------------------------
/** Defines type identifiers for use within the material system.
*
*/
// ---------------------------------------------------------------------------
enum aiPropertyTypeInfo
{
    /** Array of single-precision floats
    */
    aiPTI_Float = 0x1,

    /** aiString data structure
    */
    aiPTI_String = 0x3,

    /** Array of Integers
    */
    aiPTI_Integer = 0x4,

    /** Simple binary buffer
    */
    aiPTI_Buffer = 0x5
};

// ---------------------------------------------------------------------------
/** Defines texture operations like add, mul ...
*
*/
// ---------------------------------------------------------------------------
enum aiTextureOp
{
    /** T = T1 * T2
     */
    aiTextureOp_Multiply = 0x0,

    /** T = T1 + T2
     */
    aiTextureOp_Add = 0x1,

    /** T = T1 - T2
     */
    aiTextureOp_Subtract = 0x2,

    /** T = T1 / T2
     */
    aiTextureOp_Divide = 0x3,

    /** T = (T1 + T2) - (T1 * T2)
     */
    aiTextureOp_SmoothAdd = 0x4,

    /** T = T1 + (T2-0.5)
     */
    aiTextureOp_SignedAdd = 0x5
};

// ---------------------------------------------------------------------------
/** Defines texture mapping for use within the material system.
*
*/
// ---------------------------------------------------------------------------
enum aiTextureMapMode
{
    /** A texture coordinate u|v is translated to u%1|v%1 
     */
    aiTextureMapMode_Wrap = 0x0,

    /** Texture coordinates outside the area formed by 1|1 and 0|0
     *  are clamped to the nearest valid value on an axis
     */
    aiTextureMapMode_Clamp = 0x1,

    /** A texture coordinate u|v becomes u%1|v%1 if (u-(u%1))%2 is zero and
     *  1-(u%1)|1-(v%1) otherwise
     */
    aiTextureMapMode_Mirror = 0x2
};

// ---------------------------------------------------------------------------
/** Defines all shading models supported by the library
*
*  @note The list of shading modes has been taken from Blender3D.
*  See Blender3D documentation for more information. The API does
*  not distinguish between "specular" and "diffuse" shaders (thus the
*  specular term for diffuse shading models like Oren-Nayar remains
*  undefined)
*/
// ---------------------------------------------------------------------------
enum aiShadingMode
{
    /** Flat shading. Shading is done on per-face base, 
    *  diffuse only.
    */
    aiShadingMode_Flat = 0x1,

    /** Diffuse gouraud shading. Shading on per-vertex base
    */
    aiShadingMode_Gouraud =	0x2,

    /** Diffuse/Specular Phong-Shading
    *
    *  Shading is applied on per-pixel base. This is the
    *  slowest algorithm, but generates the best results.
    */
    aiShadingMode_Phong = 0x3,

    /** Diffuse/Specular Phong-Blinn-Shading
    *
    *  Shading is applied on per-pixel base. This is a little
    *  bit faster than phong and in some cases even
    *  more realistic
    */
    aiShadingMode_Blinn	= 0x4,

    /** Toon-Shading per pixel
    *
    *  Shading is applied on per-pixel base. The output looks
    *  like a comic. Often combined with edge detection.
    */
    aiShadingMode_Toon = 0x5,

    /** OrenNayar-Shading per pixel
    *
    *  Extension to standard lambertian shading, taking the
    *  roughness of the material into account
    *	
    */
    aiShadingMode_OrenNayar = 0x6,

    /** Minnaert-Shading per pixel
    *
    *  Extension to standard lambertian shading, taking the
    *  "darkness" of the material into account
    */
    aiShadingMode_Minnaert = 0x7,

    /** CookTorrance-Shading per pixel
    */
    aiShadingMode_CookTorrance = 0x8,

    /** No shading at all
    */
    aiShadingMode_NoShading = 0x8
};


// ---------------------------------------------------------------------------
/** Data structure for a single property inside a material
*
*  @see aiMaterial
*/
// ---------------------------------------------------------------------------
struct aiMaterialProperty
{
    /** Specifies the name of the property (key)
    *
    * Keys are case insensitive.
    */
    C_STRUCT aiString mKey;

    /**	Size of the buffer mData is pointing to, in bytes
	* This value may not be 0.
    */
    unsigned int mDataLength;

    /** Type information for the property.
    *
    * Defines the data layout inside the
    * data buffer. This is used by the library
    * internally to perform debug checks.
    */
    aiPropertyTypeInfo mType;

    /**	Binary buffer to hold the property's value
    *
    * The buffer has no terminal character. However,
    * if a string is stored inside it may use 0 as terminal,
    * but it would be contained in mDataLength. This member
	* is never 0
    */
    char* mData;
};

#ifdef __cplusplus
} // need to end extern C block to allow template member functions
#endif

#define AI_TEXTYPE_OPACITY		0x0
#define AI_TEXTYPE_SPECULAR		0x1
#define AI_TEXTYPE_AMBIENT		0x2
#define AI_TEXTYPE_EMISSIVE		0x3
#define AI_TEXTYPE_HEIGHT		0x4
#define AI_TEXTYPE_NORMALS		0x5
#define AI_TEXTYPE_SHININESS	0x6
#define AI_TEXTYPE_DIFFUSE		0x7

// ---------------------------------------------------------------------------
/** Data structure for a material
*
*  Material data is stored using a key-value structure, called property
*  (to guarant that the system is maximally flexible).
*  The library defines a set of standard keys (AI_MATKEY) which should be 
*  enough for nearly all purposes. 
*/
// ---------------------------------------------------------------------------
struct ASSIMP_API aiMaterial
{
#ifdef __cplusplus
protected:
    aiMaterial() {}
public:

	// -------------------------------------------------------------------
    /** Retrieve an array of Type values with a specific key 
     *  from the material
     *
     * @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
     * @param pOut Pointer to a buffer to receive the result. 
     * @param pMax Specifies the size of the given buffer, in Type's.
     * Receives the number of values (not bytes!) read. 
     * NULL is a valid value for this parameter.
     */
    template <typename Type>
    inline aiReturn Get(const char* pKey,Type* pOut,
        unsigned int* pMax);

    // -------------------------------------------------------------------
    /** Retrieve a Type value with a specific key 
     *  from the material
	 *
	 * @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
	 * @param pOut Reference to receive the output value
	 */
	template <typename Type>
	inline aiReturn Get(const char* pKey,Type& pOut);

	// -------------------------------------------------------------------
	/** Helper function to get a texture from a material
	*
	*  This function is provided just for convinience. 
	*  @param iIndex Index of the texture to retrieve. If the index is too 
	*		large the function fails.
	*  @param iTexType One of the AI_TEXTYPE constants. Specifies the type of
	*		the texture to retrieve (e.g. diffuse, specular, height map ...)
	*  @param szPath Receives the output path
	*		NULL is no allowed as value
	*  @param piUVIndex Receives the UV index of the texture. 
	*		NULL is allowed as value.
	*  @param pfBlendFactor Receives the blend factor for the texture
	*		NULL is allowed as value.
	*  @param peTextureOp Receives the texture operation to perform between
	*		this texture and the previous texture. NULL is allowed as value.
	*  @param peMapMode Receives the mapping modes to be used for the texture.
	*      The parameter may be NULL but if it is a valid pointer it MUST
	*      point to an array of 3 aiTextureMapMode variables (one for each
	*      axis: UVW order (=XYZ)). 
	*/
	// -------------------------------------------------------------------
	inline aiReturn GetTexture(unsigned int iIndex,
		unsigned int iTexType,
		C_STRUCT aiString* szPath,
		unsigned int* piUVIndex		= NULL,
		float* pfBlendFactor		= NULL,
		aiTextureOp* peTextureOp	= NULL,
		aiTextureMapMode* peMapMode = NULL); 
#endif

    /** List of all material properties loaded.
    */
    C_STRUCT aiMaterialProperty** mProperties;

    /** Number of properties loaded
    */
    unsigned int mNumProperties;
    unsigned int mNumAllocated;
};

#ifdef __cplusplus
extern "C" {
#endif


// ---------------------------------------------------------------------------
/** @def AI_BUILD_KEY
 * Builds a material texture key with a dynamic index.
 * Applications <b>could</b> do this (C-example):
 * @code
 * int i;
 * struct aiMaterial* pMat = .... 
 * for (i = 0;true;++i) {
 *    if (AI_SUCCESS != aiGetMaterialFloat(pMat,AI_MATKEY_TEXTURE_DIFFUSE(i),...)) {
 *       ...
 *    }
 * }
 * @endcode 
 * However, this is wrong because AI_MATKEY_TEXTURE_DIFFUSE() builds the key 
 * string at <b>compile time</b>. <br>
 * Therefore, the dynamic indexing results in a
 * material key like this : "$tex.file.diffuse[i]" - and it is not very
 * propable that there is a key with this name ... (except the programmer
 * of an ASSIMP loader has made the same mistake :-) ).<br>
 * This is the right way:
 * @code
 * int i;
 * char szBuffer[512];
 * struct aiMaterial* pMat = .... 
 * for (i = 0;true;++i) {
 *    AI_BUILD_KEY(AI_MATKEY_TEXTURE_DIFFUSE_,i,szBuffer);
 *    if (AI_SUCCESS != aiGetMaterialFloat(pMat,szBuffer,...)) {
 *       ...
 *    }
 * }
 * @endcode 
 * @param base Base material key. This is the same key you'd have used
 *   normally with an underscore as suffix (e.g. AI_MATKEY_TEXTURE_DIFFUSE_)
 * @param index Index to be used. Here you may pass a variable!
 * @param out Array of chars to receive the output value. It must be 
 *  sufficiently large. This will be checked via a static assertion for
 *  C++0x. For MSVC8 and later versions the security enhanced version of
 *  sprintf() will be used. However, if your buffer is at least 256 bytes
 *  long you'll never overrun.
*/
// ---------------------------------------------------------------------------
#if _MSC_VER >= 1400

	// MSVC 8+. Use the sprintf_s function with security enhancements
#	define AI_BUILD_KEY(base,index,out) \
	::sprintf_s(out,"%s[%i]",base,index);

#elif (defined ASSIMP_BUILD_CPP_09)

	// C++09 compiler. Use a static assertion to validate the size
	// of the output buffer
#	define AI_BUILD_KEY(base,index,out) \
	static_assert(sizeof(out) >= 180,"Output buffer is too small");  \
	::sprintf(out,"%s[%i]",base,index); 

#else

	// otherwise ... simply hope the buffer is large enough :-)
#	define AI_BUILD_KEY(base,index,out) \
	::sprintf(out,"%s[%i]",base,index);
#endif


// ---------------------------------------------------------------------------
/** @def AI_MATKEY_NAME
 *  Defines the name of the material 
 * <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
*/
#define AI_MATKEY_NAME "$mat.name"

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_TWOSIDED
 *  Indicates that the material must be rendered two-sided
 * <br>
 * <b>Type:</b> int <br>
 * <b>Default value:</b> 0 <br>
*/
#define AI_MATKEY_TWOSIDED "$mat.twosided"

/** @def AI_MATKEY_SHADING_MODE
 *  Defines the shading model to use (aiShadingMode)
 * <br>
 * <b>Type:</b> int (aiShadingMode)<br>
 * <b>Default value:</b> aiShadingMode_Gouraud <br>
*/
#define AI_MATKEY_SHADING_MODEL "$mat.shadingm"

/** @def AI_MATKEY_ENABLE_WIREFRAM
 *  Integer property. 1 to enable wireframe for rendering
 * <br>
 * <b>Type:</b> int <br>
 * <b>Default value:</b> 0 <br>
*/
#define AI_MATKEY_ENABLE_WIREFRAME "$mat.wireframe"

/** @def AI_MATKEY_OPACITY
 *  Defines the base opacity of the material
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 1.0f <br>
*/
#define AI_MATKEY_OPACITY "$mat.opacity"

/** @def AI_MATKEY_BUMPSCALING
 *  Defines the height scaling of a bump map (for stuff like Parallax
 *  Occlusion Mapping)
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 1.0f <br>
*/
#define AI_MATKEY_BUMPSCALING "$mat.bumpscaling"

/** @def AI_MATKEY_SHININESS
 *  Defines the base shininess of the material
 *  This is the exponent of the phong shading equation.
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 0.0f <br>
*/
#define AI_MATKEY_SHININESS "$mat.shininess"

/** @def AI_MATKEY_SHININESS_STRENGTH
 * Defines the strength of the specular highlight.
 * This is simply a multiplier to the specular color of a material
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 1.0f <br>
*/
#define AI_MATKEY_SHININESS_STRENGTH "$mat.shinpercent"

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_COLOR_DIFFUSE
 *  Defines the diffuse base color of the material
 * <br>
 * <b>Type:</b> color (aiColor4D or aiColor3D)<br>
 * <b>Default value:</b> 0.0f|0.0f|0.0f|1.0f <br>
*/
#define AI_MATKEY_COLOR_DIFFUSE "$clr.diffuse"

/** @def AI_MATKEY_COLOR_AMBIENT
 *  Defines the ambient base color of the material
 * <br>
 * <b>Type:</b> color (aiColor4D or aiColor3D)<br>
 * <b>Default value:</b> 0.0f|0.0f|0.0f|1.0f <br>
*/
#define AI_MATKEY_COLOR_AMBIENT "$clr.ambient"

/** @def AI_MATKEY_COLOR_SPECULAR
 *  Defines the specular base color of the material
 * <br>
 * <b>Type:</b> color (aiColor4D or aiColor3D)<br>
 * <b>Default value:</b> 0.0f|0.0f|0.0f|1.0f <br>
*/
#define AI_MATKEY_COLOR_SPECULAR "$clr.specular"

/** @def AI_MATKEY_COLOR_EMISSIVE
 *  Defines the emissive base color of the material
 * <br>
 * <b>Type:</b> color (aiColor4D or aiColor3D)<br>
 * <b>Default value:</b> 0.0f|0.0f|0.0f|1.0f <br>
*/
#define AI_MATKEY_COLOR_EMISSIVE "$clr.emissive"

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_TEXTURE_DIFFUSE
*  Defines a specific diffuse texture channel of the material
 *  <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
 * @note The key string is built at compile time, therefore it is not 
 * posible  to use this macro in a loop with varying values for N. 
 * However, you can  use the macro suffixed with '_' to build the key 
 * dynamically. The AI_BUILD_KEY()-macro can be used to do this.
*/
#define AI_MATKEY_TEXTURE_DIFFUSE(N) "$tex.file.diffuse["#N"]"
#define AI_MATKEY_TEXTURE_DIFFUSE_  "$tex.file.diffuse"

/** @def AI_MATKEY_TEXTURE_AMBIENT
 *  Defines a specific ambient texture channel of the material
 *  <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
 * @note The key string is built at compile time, therefore it is not 
 * posible  to use this macro in a loop with varying values for N. 
 * However, you can  use the macro suffixed with '_' to build the key 
 * dynamically. The AI_BUILD_KEY()-macro can be used to do this.
*/
#define AI_MATKEY_TEXTURE_AMBIENT(N) "$tex.file.ambient["#N"]"
#define AI_MATKEY_TEXTURE_AMBIENT_   "$tex.file.ambient"

/** @def AI_MATKEY_TEXTURE_SPECULAR
 *  Defines a specific specular texture channel of the material
 *  <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
 * @note The key string is built at compile time, therefore it is not 
 * posible  to use this macro in a loop with varying values for N. 
 * However, you can  use the macro suffixed with '_' to build the key 
 * dynamically. The AI_BUILD_KEY()-macro can be used to do this.
*/
#define AI_MATKEY_TEXTURE_SPECULAR(N) "$tex.file.specular["#N"]"
#define AI_MATKEY_TEXTURE_SPECULAR_   "$tex.file.specular"

/** @def AI_MATKEY_TEXTURE_EMISSIVE
 *  Defines a specific emissive texture channel of the material
 *  <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
 * @note The key string is built at compile time, therefore it is not 
 * posible  to use this macro in a loop with varying values for N. 
 * However, you can  use the macro suffixed with '_' to build the key 
 * dynamically. The AI_BUILD_KEY()-macro can be used to do this.
*/
#define AI_MATKEY_TEXTURE_EMISSIVE(N) "$tex.file.emissive["#N"]"
#define AI_MATKEY_TEXTURE_EMISSIVE_   "$tex.file.emissive"

/** @def AI_MATKEY_TEXTURE_NORMALS
 *  Defines a specific normal texture channel of the material
 *  <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
 * @note The key string is built at compile time, therefore it is not 
 * posible  to use this macro in a loop with varying values for N. 
 * However, you can  use the macro suffixed with '_' to build the key 
 * dynamically. The AI_BUILD_KEY()-macro can be used to do this.
*/
#define AI_MATKEY_TEXTURE_NORMALS(N) "$tex.file.normals["#N"]"
#define AI_MATKEY_TEXTURE_NORMALS_   "$tex.file.normals"

/** @def AI_MATKEY_TEXTURE_HEIGHT
 * Defines a specified bumpmap texture (=heightmap) channel of the material
 * This is very similar to #AI_MATKEY_TEXTURE_NORMALS. It is provided
 * to allow applications to determine whether the input data for
 * normal mapping is already a normal map or needs to be converted from
 * a heightmap to a normal map.
 * <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
 * @note The key string is built at compile time, therefore it is not 
 * posible  to use this macro in a loop with varying values for N. 
 * However, you can  use the macro suffixed with '_' to build the key 
 * dynamically. The AI_BUILD_KEY()-macro can be used to do this.
*/
#define AI_MATKEY_TEXTURE_HEIGHT(N) "$tex.file.height["#N"]"
#define AI_MATKEY_TEXTURE_HEIGHT_   "$tex.file.height"

/** @def AI_MATKEY_TEXTURE_SHININESS
 *  Defines a specific shininess texture channel of the material
 *  <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
 * @note The key string is built at compile time, therefore it is not 
 * posible  to use this macro in a loop with varying values for N. 
 * However, you can  use the macro suffixed with '_' to build the key 
 * dynamically. The AI_BUILD_KEY()-macro can be used to do this.
*/
#define AI_MATKEY_TEXTURE_SHININESS(N) "$tex.file.shininess["#N"]"
#define AI_MATKEY_TEXTURE_SHININESS_   "$tex.file.shininess"

/** @def AI_MATKEY_TEXTURE_OPACITY
 *  Defines a specific opacity texture channel of the material
 *  <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
 * @note The key string is built at compile time, therefore it is not 
 * posible  to use this macro in a loop with varying values for N. 
 * However, you can  use the macro suffixed with '_' to build the key 
 * dynamically. The AI_BUILD_KEY()-macro can be used to do this.
*/
#define AI_MATKEY_TEXTURE_OPACITY(N) "$tex.file.opacity["#N"]"
#define AI_MATKEY_TEXTURE_OPACITY_   "$tex.file.opacity"


// ---------------------------------------------------------------------------
/** @def AI_MATKEY_TEXOP_DIFFUSE
 * Specifies the blend operation too be used to combine the Nth
 * diffuse texture with the (N-1)th diffuse texture (or the diffuse
 * base color for the first diffuse texture)
 * <br>
 * <b>Type:</b> int (aiTextureOp)<br>
 * <b>Default value:</b> 0<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE_DIFFUSE(0)<br>
 * @note The key string is built at compile time, therefore it is not posible 
 * to use this macro in a loop with varying values for N. However, you can 
 * use the macro suffixed with '_' to build the key dynamically. The 
 * AI_BUILD_KEY()-macro can be used to do this.
 */
#define AI_MATKEY_TEXOP_DIFFUSE(N)	"$tex.op.diffuse["#N"]"
/** @see AI_MATKEY_TEXOP_DIFFUSE */
#define AI_MATKEY_TEXOP_AMBIENT(N)	"$tex.op.ambient["#N"]"
/** @see AI_MATKEY_TEXOP_DIFFUSE */
#define AI_MATKEY_TEXOP_SPECULAR(N)	"$tex.op.specular["#N"]"
/** @see AI_MATKEY_TEXOP_DIFFUSE */
#define AI_MATKEY_TEXOP_EMISSIVE(N)	"$tex.op.emissive["#N"]"
/** @see AI_MATKEY_TEXOP_DIFFUSE */
#define AI_MATKEY_TEXOP_NORMALS(N)	"$tex.op.normals["#N"]"
/** @see AI_MATKEY_TEXOP_DIFFUSE */
#define AI_MATKEY_TEXOP_HEIGHT(N)	"$tex.op.height["#N"]"
/** @see AI_MATKEY_TEXOP_DIFFUSE */
#define AI_MATKEY_TEXOP_SHININESS(N)"$tex.op.shininess["#N"]"
/** @see AI_MATKEY_TEXOP_DIFFUSE */
#define AI_MATKEY_TEXOP_OPACITY(N)	"$tex.op.opacity["#N"]"

#define AI_MATKEY_TEXOP_DIFFUSE_	"$tex.op.diffuse"
#define AI_MATKEY_TEXOP_AMBIENT_	"$tex.op.ambient"
#define AI_MATKEY_TEXOP_SPECULAR_	"$tex.op.specular"
#define AI_MATKEY_TEXOP_EMISSIVE_	"$tex.op.emissive"
#define AI_MATKEY_TEXOP_NORMALS_	"$tex.op.normals"
#define AI_MATKEY_TEXOP_HEIGHT_		"$tex.op.height"
#define AI_MATKEY_TEXOP_SHININESS_	"$tex.op.shininess"
#define AI_MATKEY_TEXOP_OPACITY_	"$tex.op.opacity"

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_UVWSRC_DIFFUSE
 * Specifies the UV channel to be used for the Nth diffuse texture
 * <br>
 * <b>Type:</b> int<br>
 * <b>Default value:</b> 0<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE_DIFFUSE(0)<br>
 * @note The key string is built at compile time, therefore it is not posible 
 * to use this macro in a loop with varying values for N. However, you can 
 * use the macro suffixed with '_' to build the key dynamically. The 
 * AI_BUILD_KEY()-macro can be used to do this.
 */
#define AI_MATKEY_UVWSRC_DIFFUSE(N)		"$tex.uvw.diffuse["#N"]"
/** @see AI_MATKEY_UVWSRC_DIFFUSE */
#define AI_MATKEY_UVWSRC_AMBIENT(N)		"$tex.uvw.ambient["#N"]"
/** @see AI_MATKEY_UVWSRC_DIFFUSE */
#define AI_MATKEY_UVWSRC_SPECULAR(N)	"$tex.uvw.specular["#N"]"
/** @see AI_MATKEY_UVWSRC_DIFFUSE */
#define AI_MATKEY_UVWSRC_EMISSIVE(N)	"$tex.uvw.emissive["#N"]"
/** @see AI_MATKEY_UVWSRC_DIFFUSE */
#define AI_MATKEY_UVWSRC_NORMALS(N)		"$tex.uvw.normals["#N"]"
/** @see AI_MATKEY_UVWSRC_DIFFUSE */
#define AI_MATKEY_UVWSRC_HEIGHT(N)		"$tex.uvw.height["#N"]"
/** @see AI_MATKEY_UVWSRC_DIFFUSE */
#define AI_MATKEY_UVWSRC_SHININESS(N)	"$tex.uvw.shininess["#N"]"
/** @see AI_MATKEY_UVWSRC_DIFFUSE */
#define AI_MATKEY_UVWSRC_OPACITY(N)		"$tex.uvw.opacity["#N"]"

#define AI_MATKEY_UVWSRC_DIFFUSE_		"$tex.uvw.diffuse"
#define AI_MATKEY_UVWSRC_AMBIENT_		"$tex.uvw.ambient"
#define AI_MATKEY_UVWSRC_SPECULAR_		"$tex.uvw.specular"
#define AI_MATKEY_UVWSRC_EMISSIVE_		"$tex.uvw.emissive"
#define AI_MATKEY_UVWSRC_NORMALS_		"$tex.uvw.normals"
#define AI_MATKEY_UVWSRC_HEIGHT_		"$tex.uvw.height"
#define AI_MATKEY_UVWSRC_SHININESS_		"$tex.uvw.shininess"
#define AI_MATKEY_UVWSRC_OPACITY_		"$tex.uvw.opacity"

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_TEXBLEND_DIFFUSE
 * Specifies the blend factor to be used for the Nth diffuse texture.
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 1.0f<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE_DIFFUSE(0)<br>
 * @note The key string is built at compile time, therefore it is not posible 
 * to use this macro in a loop with varying values for N. However, you can 
 * use the macro suffixed with '_' to build the key dynamically. The 
 * AI_BUILD_KEY()-macro can be used to do this.
 */
#define AI_MATKEY_TEXBLEND_DIFFUSE(N)	"$tex.blend.diffuse["#N"]"
/** @see AI_MATKEY_TEXBLEND_DIFFUSE */
#define AI_MATKEY_TEXBLEND_AMBIENT(N)	"$tex.blend.ambient["#N"]"
/** @see AI_MATKEY_TEXBLEND_DIFFUSE */
#define AI_MATKEY_TEXBLEND_SPECULAR(N)	"$tex.blend.specular["#N"]"
/** @see AI_MATKEY_TEXBLEND_DIFFUSE */
#define AI_MATKEY_TEXBLEND_EMISSIVE(N)	"$tex.blend.emissive["#N"]"
/** @see AI_MATKEY_TEXBLEND_DIFFUSE */
#define AI_MATKEY_TEXBLEND_NORMALS(N)	"$tex.blend.normals["#N"]"
/** @see AI_MATKEY_TEXBLEND_DIFFUSE */
#define AI_MATKEY_TEXBLEND_HEIGHT(N)	"$tex.blend.height["#N"]"
/** @see AI_MATKEY_TEXBLEND_DIFFUSE */
#define AI_MATKEY_TEXBLEND_SHININESS(N)	"$tex.blend.shininess["#N"]"
/** @see AI_MATKEY_TEXBLEND_DIFFUSE */
#define AI_MATKEY_TEXBLEND_OPACITY(N)	"$tex.blend.opacity["#N"]"

#define AI_MATKEY_TEXBLEND_DIFFUSE_		"$tex.blend.diffuse"
#define AI_MATKEY_TEXBLEND_AMBIENT_		"$tex.blend.ambient"
#define AI_MATKEY_TEXBLEND_SPECULAR_	"$tex.blend.specular"
#define AI_MATKEY_TEXBLEND_EMISSIVE_	"$tex.blend.emissive"
#define AI_MATKEY_TEXBLEND_NORMALS_		"$tex.blend.normals"
#define AI_MATKEY_TEXBLEND_HEIGHT_		"$tex.blend.height"
#define AI_MATKEY_TEXBLEND_SHININESS_	"$tex.blend.shininess"
#define AI_MATKEY_TEXBLEND_OPACITY_		"$tex.blend.opacity"

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_MAPPINGMODE_U_DIFFUSE
 * Specifies the texture mapping mode for the Nth diffuse texture in
 * the u (x) direction
 * <br>
 * <b>Type:</b> int (aiTextureMapMode)<br>
 * <b>Default value:</b> aiTextureMapMode_Wrap<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE_DIFFUSE(0)<br>
 * @note The key string is built at compile time, therefore it is not posible 
 * to use this macro in a loop with varying values for N. However, you can 
 * use the macro suffixed with '_' to build the key dynamically. The 
 * AI_BUILD_KEY()-macro can be used to do this.
 */
#define AI_MATKEY_MAPPINGMODE_U_DIFFUSE(N)	"$tex.mapmodeu.diffuse["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_U_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_U_AMBIENT(N)	"$tex.mapmodeu.ambient["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_U_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_U_SPECULAR(N)	"$tex.mapmodeu.specular["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_U_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_U_EMISSIVE(N)	"$tex.mapmodeu.emissive["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_U_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_U_NORMALS(N)	"$tex.mapmodeu.normals["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_U_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_U_HEIGHT(N)	"$tex.mapmodeu.height["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_U_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_U_SHININESS(N)"$tex.mapmodeu.shininess["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_U_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_U_OPACITY(N)	"$tex.mapmodeu.opacity["#N"]"

#define AI_MATKEY_MAPPINGMODE_U_DIFFUSE_	"$tex.mapmodeu.diffuse"
#define AI_MATKEY_MAPPINGMODE_U_AMBIENT_	"$tex.mapmodeu.ambient"
#define AI_MATKEY_MAPPINGMODE_U_SPECULAR_	"$tex.mapmodeu.specular"
#define AI_MATKEY_MAPPINGMODE_U_EMISSIVE_	"$tex.mapmodeu.emissive"
#define AI_MATKEY_MAPPINGMODE_U_NORMALS_	"$tex.mapmodeu.normals"
#define AI_MATKEY_MAPPINGMODE_U_HEIGHT_		"$tex.mapmodeu.height"
#define AI_MATKEY_MAPPINGMODE_U_SHININESS_	"$tex.mapmodeu.shininess"
#define AI_MATKEY_MAPPINGMODE_U_OPACITY_	"$tex.mapmodeu.opacity"

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_MAPPINGMODE_V_DIFFUSE
 * Specifies the texture mapping mode for the Nth diffuse texture in
 * the v (y) direction
 * <br>
 * <b>Type:</b> int (aiTextureMapMode)<br>
 * <b>Default value:</b> aiTextureMapMode_Wrap<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE_DIFFUSE(0)<br>
 * @note The key string is built at compile time, therefore it is not posible 
 * to use this macro in a loop with varying values for N. However, you can 
 * use the macro suffixed with '_' to build the key dynamically. The 
 * AI_BUILD_KEY()-macro can be used to do this.
 */
#define AI_MATKEY_MAPPINGMODE_V_DIFFUSE(N)	"$tex.mapmodev.diffuse["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_V_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_V_AMBIENT(N)	"$tex.mapmodev.ambient["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_V_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_V_SPECULAR(N)	"$tex.mapmodev.specular["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_V_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_V_EMISSIVE(N)	"$tex.mapmodev.emissive["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_V_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_V_NORMALS(N)	"$tex.mapmodev.normals["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_V_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_V_HEIGHT(N)	"$tex.mapmodev.height["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_V_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_V_SHININESS(N)"$tex.mapmodev.shininess["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_V_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_V_OPACITY(N)	"$tex.mapmodev.opacity["#N"]"

#define AI_MATKEY_MAPPINGMODE_V_DIFFUSE_	"$tex.mapmodev.diffuse"
#define AI_MATKEY_MAPPINGMODE_V_AMBIENT_	"$tex.mapmodev.ambient"
#define AI_MATKEY_MAPPINGMODE_V_SPECULAR_	"$tex.mapmodev.specular"
#define AI_MATKEY_MAPPINGMODE_V_EMISSIVE_	"$tex.mapmodev.emissive"
#define AI_MATKEY_MAPPINGMODE_V_NORMALS_	"$tex.mapmodev.normals"
#define AI_MATKEY_MAPPINGMODE_V_HEIGHT_		"$tex.mapmodev.height"
#define AI_MATKEY_MAPPINGMODE_V_SHININESS_	"$tex.mapmodev.shininess"
#define AI_MATKEY_MAPPINGMODE_V_OPACITY_	"$tex.mapmodev.opacity"

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_MAPPINGMODE_W_DIFFUSE
 * Specifies the texture mapping mode for the Nth diffuse texture in
 * the w (z) direction
 * <br>
 * <b>Type:</b> int (aiTextureMapMode)<br>
 * <b>Default value:</b> aiTextureMapMode_Wrap<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE_DIFFUSE(0)<br>
 * @note The key string is built at compile time, therefore it is not posible 
 * to use this macro in a loop with varying values for N. However, you can 
 * use the macro suffixed with '_' to build the key dynamically. The 
 * AI_BUILD_KEY()-macro can be used to do this.
 */
#define AI_MATKEY_MAPPINGMODE_W_DIFFUSE(N)	"$tex.mapmodew.diffuse["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_W_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_W_AMBIENT(N)	"$tex.mapmodew.ambient["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_W_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_W_SPECULAR(N)	"$tex.mapmodew.specular["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_W_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_W_EMISSIVE(N)	"$tex.mapmodew.emissive["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_W_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_W_NORMALS(N)	"$tex.mapmodew.normals["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_W_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_W_HEIGHT(N)	"$tex.mapmodew.bump["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_W_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_W_SHININESS(N)"$tex.mapmodew.shininess["#N"]"
/** @see AI_MATKEY_MAPPINGMODE_W_DIFFUSE */
#define AI_MATKEY_MAPPINGMODE_W_OPACITY(N)	"$tex.mapmodew.opacity["#N"]"

#define AI_MATKEY_MAPPINGMODE_W_DIFFUSE_	"$tex.mapmodew.diffuse"
#define AI_MATKEY_MAPPINGMODE_W_AMBIENT_	"$tex.mapmodew.ambient"
#define AI_MATKEY_MAPPINGMODE_W_SPECULAR_	"$tex.mapmodew.specular"
#define AI_MATKEY_MAPPINGMODE_W_EMISSIVE_	"$tex.mapmodew.emissive"
#define AI_MATKEY_MAPPINGMODE_W_NORMALS_	"$tex.mapmodew.normals"
#define AI_MATKEY_MAPPINGMODE_W_HEIGHT_		"$tex.mapmodew.height"
#define AI_MATKEY_MAPPINGMODE_W_SHININESS_	"$tex.mapmodew.shininess"
#define AI_MATKEY_MAPPINGMODE_W_OPACITY_	"$tex.mapmodew.opacity"

#define AI_MATKEY_ORENNAYAR_ROUGHNESS	 "$shading.orennayar.roughness"
#define AI_MATKEY_MINNAERT_DARKNESS		 "$shading.minnaert.darkness"
#define AI_MATKEY_COOK_TORRANCE_REFRACTI "$shading.cookt.refracti"
#define AI_MATKEY_COOK_TORRANCE_PARAM	 "$shading.cookt.param"

/** @def AI_MATKEY_GLOBAL_BACKGROUND_IMAGE
*  Global property defined by some loaders. Contains the path to 
*  the image file to be used as background image.
*/
#define AI_MATKEY_GLOBAL_BACKGROUND_IMAGE "$global.bg.image2d"


// ---------------------------------------------------------------------------
/** Retrieve a material property with a specific key from the material
*
*  @param pMat Pointer to the input material. May not be NULL
*  @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
*  @param pPropOut Pointer to receive a pointer to a valid aiMaterialProperty
*         structure or NULL if the key has not been found. 
*/
// ---------------------------------------------------------------------------
ASSIMP_API aiReturn aiGetMaterialProperty(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
    const C_STRUCT aiMaterialProperty** pPropOut);


// ---------------------------------------------------------------------------
/** Retrieve an array of float values with a specific key 
*  from the material
*
* @param pMat Pointer to the input material. May not be NULL
* @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
* @param pOut Pointer to a buffer to receive the result. 
* @param pMax Specifies the size of the given buffer, in float's.
*        Receives the number of values (not bytes!) read. 
*/
// ---------------------------------------------------------------------------
ASSIMP_API aiReturn aiGetMaterialFloatArray(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
    float* pOut,
    unsigned int* pMax);

#ifdef __cplusplus
// inline it
inline aiReturn aiGetMaterialFloat(const C_STRUCT aiMaterial* pMat, 
     const char* pKey,
     float* pOut)
    {return aiGetMaterialFloatArray(pMat,pKey,pOut,(unsigned int*)0x0);}
#else 
// use our friend, the C preprocessor
#define aiGetMaterialFloat (pMat, pKey, pOut) \
    aiGetMaterialFloatArray(pMat, pKey, pOut, NULL)
#endif //!__cplusplus


// ---------------------------------------------------------------------------
/** Retrieve an array of integer values with a specific key 
*  from the material
*
* @param pMat Pointer to the input material. May not be NULL
* @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
* @param pOut Pointer to a buffer to receive the result. 
* @param pMax Specifies the size of the given buffer, in int's.
*        Receives the number of values (not bytes!) read. 
*/
// ---------------------------------------------------------------------------
ASSIMP_API aiReturn aiGetMaterialIntegerArray(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
    int* pOut,
    unsigned int* pMax);

#ifdef __cplusplus
// inline it
inline aiReturn aiGetMaterialInteger(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
    int* pOut)
    {return aiGetMaterialIntegerArray(pMat,pKey,pOut,(unsigned int*)0x0);}
#else 
// use our friend, the C preprocessor
#define aiGetMaterialInteger (pMat, pKey, pOut) \
    aiGetMaterialIntegerArray(pMat, pKey, pOut, NULL)
#endif //!__cplusplus



// ---------------------------------------------------------------------------
/** Retrieve a color value from the material property table
*
*	@param pMat Pointer to the input material. May not be NULL
*	@param pKey Key to search for. One of the AI_MATKEY_XXX constants.
*	@param pOut Pointer to a buffer to receive the result. 
*/
// ---------------------------------------------------------------------------
ASSIMP_API aiReturn aiGetMaterialColor(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
    aiColor4D* pOut);


// ---------------------------------------------------------------------------
/** Retrieve a string from the material property table
*
*	@param pMat Pointer to the input material. May not be NULL
*	@param pKey Key to search for. One of the AI_MATKEY_XXX constants.
*	@param pOut Pointer to a buffer to receive the result. 
*/
// ---------------------------------------------------------------------------
ASSIMP_API aiReturn aiGetMaterialString(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
    aiString* pOut);


// ---------------------------------------------------------------------------
/** Helper function to get a texture from a material
 *
 *  This function is provided just for convinience. 
 *  @param pMat Pointer to the input material. May not be NULL
 *  @param iIndex Index of the texture to retrieve. If the index is too 
 *		large the function fails.
 *  @param iTexType One of the AI_TEXTYPE constants. Specifies the type of
 *		the texture to retrieve (e.g. diffuse, specular, height map ...)
 *  @param szPath Receives the output path
 *		NULL is no allowed as value
 *  @param piUVIndex Receives the UV index of the texture. 
 *		NULL is allowed as value.
 *  @param pfBlendFactor Receives the blend factor for the texture
 *		NULL is allowed as value.
 *  @param peTextureOp Receives the texture operation to perform between
 *		this texture and the previous texture. NULL is allowed as value.
 *  @param peMapMode Receives the mapping modes to be used for the texture.
 *      The parameter may be NULL but if it is a valid pointer it MUST
 *      point to an array of 3 aiTextureMapMode variables (one for each
 *      axis: UVW order (=XYZ)). 
 */
// ---------------------------------------------------------------------------
#ifdef __cplusplus
ASSIMP_API aiReturn aiGetMaterialTexture(const C_STRUCT aiMaterial* pMat,
    unsigned int iIndex,
    unsigned int iTexType,
    C_STRUCT aiString* szPath,
    unsigned int* piUVIndex		= NULL,
    float* pfBlendFactor		= NULL,
    aiTextureOp* peTextureOp	= NULL,
	aiTextureMapMode* peMapMode = NULL); 
#else
aiReturn aiGetMaterialTexture(const C_STRUCT aiMaterial* pMat,
    unsigned int iIndex,
    unsigned int iTexType,
    C_STRUCT aiString* szPath,
    unsigned int* piUVIndex,
    float* pfBlendFactor,
    aiTextureOp* peTextureOp,
	aiTextureMapMode* peMapMode); 
#endif // !#ifdef __cplusplus

#ifdef __cplusplus
}

#include "aiMaterial.inl"

#endif //!__cplusplus
#endif //!!AI_MATERIAL_H_INC
