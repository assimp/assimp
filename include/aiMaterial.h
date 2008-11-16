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
    aiPTI_Buffer = 0x5,


	/** This value is not used. It is just there to force the
	 *  compiler to map this enum to a 32 Bit integer.
	 */
	_aiPTI_Force32Bit = 0x9fffffff
};

// ---------------------------------------------------------------------------
/** Defines how the Nth texture is combined with all previous textures.
*
*/
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
    aiTextureOp_SignedAdd = 0x5,


	/** This value is not used. It is just there to force the
	 *  compiler to map this enum to a 32 Bit integer.
	 */
	_aiTextureOp_Force32Bit = 0x9fffffff
};

// ---------------------------------------------------------------------------
/** Defines how UV coordinates beyond the valid range are handled.
*/
enum aiTextureMapMode
{
    /** A texture coordinate u|v is translated to u%1|v%1 
     */
    aiTextureMapMode_Wrap = 0x0,

    /** Texture coordinates outside [0...1]
     *  are clamped to the nearest valid value.
     */
    aiTextureMapMode_Clamp = 0x1,

	 /** If the texture coordinates for a pixel are outside [0...1]
	  *  the texture is not applied to that pixel
     */
    aiTextureMapMode_Decal = 0x3,

    /** A texture coordinate u|v becomes u%1|v%1 if (u-(u%1))%2 is zero and
     *  1-(u%1)|1-(v%1) otherwise
     */
    aiTextureMapMode_Mirror = 0x2,


	/** This value is not used. It is just there to force the
	 *  compiler to map this enum to a 32 Bit integer.
	 */
	_aiTextureMapMode_Force32Bit = 0x9fffffff
};

// ---------------------------------------------------------------------------
/** Defines how the mapping coords for a texture are generated.
*
*  See the AI_MATKEY_MAPPING property for more details
*/
enum aiTextureMapping
{
    /** The mapping coordinates are taken from an UV channel.
	 *
	 *  The AI_MATKEY_UVSRC key specifies from which (remember,
	 *  meshes can have more than one UV channel). 
     */
    aiTextureMapping_UV = 0x0 ,

	 /** Spherical mapping
     */
    aiTextureMapping_SPHERE = 0x1,

	 /** Cylindrical mapping
     */
    aiTextureMapping_CYLINDER = 0x2,

	/** Cubic mapping
     */
    aiTextureMapping_BOX = 0x3,

	/** Planar mapping
     */
    aiTextureMapping_PLANE = 0x4,

	/** Undefined mapping. Have fun.
     */
    aiTextureMapping_OTHER = 0x5,


	/** This value is not used. It is just there to force the
	 *  compiler to map this enum to a 32 Bit integer.
	 */
	_aiTextureMapping_Force32Bit = 0x9fffffff
};

// ---------------------------------------------------------------------------
/** Defines which mesh axes are used to construct the projection shape
 *  for non-UV mappings around the model.
 *
 *  This corresponds to the AI_MATKEY_TEXMAP_AXIS property.
*/
enum aiAxis
{
	aiAxis_X = 0x0,
	aiAxis_Y = 0x1,
	aiAxis_Z = 0x2,


	/** This value is not used. It is just there to force the
	 *  compiler to map this enum to a 32 Bit integer.
	 */
	_aiAxis_Force32Bit = 0x9fffffff
};

// ---------------------------------------------------------------------------
/** Defines the purpose of a texture 
*/
enum aiTextureType
{
    /** The texture is combined with the result of the diffuse
	 *  lighting equation.
     */
    aiTextureType_DIFFUSE = 0x0,

	/** The texture is combined with the result of the specular
	 *  lighting equation.
     */
    aiTextureType_SPECULAR = 0x1,

	/** The texture is combined with the result of the ambient
	 *  lighting equation.
     */
    aiTextureType_AMBIENT = 0x2,

	/** The texture is added to the result of the lighting
	 *  calculation. It isn't influenced by any lighting.
     */
    aiTextureType_EMISSIVE = 0x3,

	/** The texture is a height map and serves as input for
	 *  a normal map generator.
     */
    aiTextureType_HEIGHT = 0x4,

	/** The texture is a (tangent space) normal-map.
	 *
	 *  If the normal map does also contain a height channel
	 *  for use with techniques such as Parallax Occlusion Mapping
	 *  it is registered once as a normalmap.
     */
    aiTextureType_NORMALS = 0x5,

	/** The texture defines the glossiness of the material.
	 *
	 *  The glossiness is in fact the exponent of the specular
	 *  lighting equation. Normally there is a conversion
	 *  function define to map the linear color values in the
	 *  texture to a suitable exponent. Have fun.
     */
    aiTextureType_SHININESS = 0x6,

	/** The texture defines a per-pixel opacity.
	 *
	 *  Normally 'white' means opaque and 'black' means 
	 *  'transparency'. Or quite the opposite. Have fun.
     */
    aiTextureType_OPACITY = 0x7,


	/** This value is not used. It is just there to force the
	 *  compiler to map this enum to a 32 Bit integer.
	 */
	_aiTextureType_Force32Bit = 0x9fffffff
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
    aiShadingMode_NoShading = 0x9,

	/** Fresnel shading
    */
    aiShadingMode_Fresnel = 0xa,


	/** This value is not used. It is just there to force the
	 *  compiler to map this enum to a 32 Bit integer.
	 */
	_aiShadingMode_Force32Bit = 0x9fffffff
};

#include "./Compiler/pushpack1.h"

// ---------------------------------------------------------------------------
/** Defines how an UV channel is transformed.
*
*  This is just a helper structure for the AI_MATKEY_UVTRANSFORM key.
*  See its documentation for more details. 
*/
struct aiUVTransform
{
	/** Translation on the u and v axes.
	 */
	aiVector2D mTranslation;

	/** Scaling on the u and v axes.
	 */
	aiVector2D mScaling;

	/** Rotation - in counter-clockwise direction.
	 *
	 *  The rotation angle is specified in radians. The
	 *  rotation center is 0.5f|0.5f.
	 */
	float mRotation;


#ifdef __cplusplus

	aiUVTransform()
		:	mScaling	(1.f,1.f)
		,	mRotation	(0.f)
	{
		// nothing to be done here ...
	}

#endif

} PACK_STRUCT;

#include "./Compiler/poppack1.h"

// ---------------------------------------------------------------------------
/** Data structure for a single property inside a material
*
*  @see aiMaterial
*/
struct aiMaterialProperty
{
    /** Specifies the name of the property (key)
    *
    * Keys are case insensitive. 
    */
    C_STRUCT aiString mKey;

	/** Textures: Specifies the exact usage semantic
	 */
	unsigned int mSemantic;

	/** Textures: Specifies the index of the texture
	 *
	 *  Textures are counted per-type.
	 */
	unsigned int mIndex;

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

#ifdef __cplusplus

	aiMaterialProperty()
	{
		mData = NULL;
		mIndex = mSemantic = 0;
	}

	~aiMaterialProperty()
	{
		delete[] mData;
	}

#endif
};

#ifdef __cplusplus
} // need to end extern C block to allow template member functions
#endif


// ---------------------------------------------------------------------------
/** Data structure for a material
*
*  Material data is stored using a key-value structure, called property
*  (to guarant that the system is maximally flexible).
*  The library defines a set of standard keys (AI_MATKEY) which should be 
*  enough for nearly all purposes. 
*/
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
    inline aiReturn Get(const char* pKey,unsigned int type,
		unsigned int idx, Type* pOut, unsigned int* pMax);

    // -------------------------------------------------------------------
    /** Retrieve a Type value with a specific key 
     *  from the material
	 *
	 * @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
	 * @param pOut Reference to receive the output value
	 */
	template <typename Type>
	inline aiReturn Get(const char* pKey,unsigned int type,
		unsigned int idx,Type& pOut);

	// -------------------------------------------------------------------
	/** Helper function to get a texture from a material structure.
	*
	*  This function is provided just for convinience. 
	*  @param mat Pointer to the input material. May not be NULL
	*  @param index Index of the texture to retrieve. If the index is too 
	*		large the function fails.
	*  @param type Specifies the type of the texture to retrieve (e.g. diffuse,
	*     specular, height map ...)
	*  @param path Receives the output path
	*		NULL is no allowed as value
	*  @param uvindex Receives the UV index of the texture. 
	*		NULL is allowed as value. The return value is 
	*  @param blend Receives the blend factor for the texture
	*		NULL is allowed as value.
	*  @param op Receives the texture operation to perform between
	*		this texture and the previous texture. NULL is allowed as value.
	*  @param mapmode Receives the mapping modes to be used for the texture.
	*      The parameter may be NULL but if it is a valid pointer it MUST
	*      point to an array of 3 aiTextureMapMode variables (one for each
	*      axis: UVW order (=XYZ)). 
	*/
	// -------------------------------------------------------------------
	inline aiReturn GetTexture(aiTextureType type,
		unsigned int  index,
		C_STRUCT aiString* path,
		aiTextureMapping* mapping	= NULL,
		unsigned int* uvindex		= NULL,
		float* blend				= NULL,
		aiTextureOp* op				= NULL,
		aiTextureMapMode* mapmode	= NULL); 
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
/** @def AI_MATKEY_NAME
 *  Defines the name of the material 
 * <br>
 * <b>Type:</b> string (aiString)<br>
 * <b>Default value:</b> none <br>
*/
#define AI_MATKEY_NAME "$mat.name",0,0


/** @def AI_MATKEY_TWOSIDED
 *  Indicates that the material must be rendered two-sided
 * <br>
 * <b>Type:</b> int <br>
 * <b>Default value:</b> 0 <br>
*/
#define AI_MATKEY_TWOSIDED "$mat.twosided",0,0


/** @def AI_MATKEY_SHADING_MODE
 *  Defines the shading model to use (aiShadingMode)
 * <br>
 * <b>Type:</b> int (aiShadingMode)<br>
 * <b>Default value:</b> aiShadingMode_Gouraud <br>
*/
#define AI_MATKEY_SHADING_MODEL "$mat.shadingm",0,0


/** @def AI_MATKEY_ENABLE_WIREFRAM
 *  Integer property. 1 to enable wireframe for rendering
 * <br>
 * <b>Type:</b> int <br>
 * <b>Default value:</b> 0 <br>
*/
#define AI_MATKEY_ENABLE_WIREFRAME "$mat.wireframe",0,0


/** @def AI_MATKEY_OPACITY
 *  Defines the base opacity of the material
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 1.0f <br>
*/
#define AI_MATKEY_OPACITY "$mat.opacity",0,0


/** @def AI_MATKEY_BUMPSCALING
 *  Defines the height scaling of a bump map (for stuff like Parallax
 *  Occlusion Mapping)
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 1.0f <br>
*/
#define AI_MATKEY_BUMPSCALING "$mat.bumpscaling",0,0


/** @def AI_MATKEY_SHININESS
 *  Defines the base shininess of the material
 *  This is the exponent of the phong shading equation.
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 0.0f <br>
*/
#define AI_MATKEY_SHININESS "$mat.shininess",0,0


/** @def AI_MATKEY_SHININESS_STRENGTH
 * Defines the strength of the specular highlight.
 * This is simply a multiplier to the specular color of a material
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 1.0f <br>
*/
#define AI_MATKEY_SHININESS_STRENGTH "$mat.shinpercent",0,0

/** @def AI_MATKEY_REFRACTI
 * Index of refraction of the material. This is used by some shading models,
 * e.g. Cook-Torrance. The value is the ratio of the speed of light in a 
 * vacuum to the speed of light in the material (always >= 1.0 in the real world).
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value:</b> 1.0f <br>
*/
#define AI_MATKEY_REFRACTI "$mat.refracti",0,0

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_COLOR_DIFFUSE
 *  Defines the diffuse base color of the material
 * <br>
 * <b>Type:</b> color (aiColor4D or aiColor3D)<br>
 * <b>Default value:</b> 0.0f|0.0f|0.0f|1.0f <br>
*/
#define AI_MATKEY_COLOR_DIFFUSE "$clr.diffuse",0,0

/** @def AI_MATKEY_COLOR_AMBIENT
 *  Defines the ambient base color of the material
 * <br>
 * <b>Type:</b> color (aiColor4D or aiColor3D)<br>
 * <b>Default value:</b> 0.0f|0.0f|0.0f|1.0f <br>
*/
#define AI_MATKEY_COLOR_AMBIENT "$clr.ambient",0,0

/** @def AI_MATKEY_COLOR_SPECULAR
 *  Defines the specular base color of the material
 * <br>
 * <b>Type:</b> color (aiColor4D or aiColor3D)<br>
 * <b>Default value:</b> 0.0f|0.0f|0.0f|1.0f <br>
*/
#define AI_MATKEY_COLOR_SPECULAR "$clr.specular",0,0

/** @def AI_MATKEY_COLOR_EMISSIVE
 *  Defines the emissive base color of the material
 * <br>
 * <b>Type:</b> color (aiColor4D or aiColor3D)<br>
 * <b>Default value:</b> 0.0f|0.0f|0.0f|1.0f <br>
*/
#define AI_MATKEY_COLOR_EMISSIVE "$clr.emissive",0,0

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_TEXTURE 
 * Parameters: type, N<br>
 * Specifies the path to the <N>th texture of type <type>.
 * This can either be a path to the texture or a string of the form '*<i>'
 * where i is an index into the array of embedded textures that has been
 * imported along with the scene. See aiTexture for more details.
 * <b>Type:</b> String<br>
 * <b>Default value to be assumed if this key isn't there:</b>n/a<br>
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_TEXTURE(type, N) "$tex.file",type,N

// for backward compatibility
#define AI_MATKEY_TEXTURE_DIFFUSE(N)	\
	AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_TEXTURE_SPECULAR(N)	\
	AI_MATKEY_TEXTURE(aiTextureType_SPECULAR,N)

#define AI_MATKEY_TEXTURE_AMBIENT(N)	\
	AI_MATKEY_TEXTURE(aiTextureType_AMBIENT,N)

#define AI_MATKEY_TEXTURE_EMISSIVE(N)	\
	AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_TEXTURE_NORMALS(N)	\
	AI_MATKEY_TEXTURE(aiTextureType_NORMALS,N)

#define AI_MATKEY_TEXTURE_HEIGHT(N)	\
	AI_MATKEY_TEXTURE(aiTextureType_HEIGHT,N)

#define AI_MATKEY_TEXTURE_SHININESS(N)	\
	AI_MATKEY_TEXTURE(aiTextureType_SHININESS,N)

#define AI_MATKEY_TEXTURE_OPACITY(N)	\
	AI_MATKEY_TEXTURE(aiTextureType_OPACITY,N)


// ---------------------------------------------------------------------------
/** @def AI_MATKEY_UVWSRC
 * Parameters: type, N<br>
 * Specifies which UV channel is used as source for the mapping coordinates 
 * of the <N>th texture of type <type>.
 * <b>Type:</b> int<br>
 * <b>Default value to be assumed if this key isn't there:</b>0<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE(type,N)  and 
 * AI_MATKEY_TEXTURE_MAPPING(type,N) == UV<br>
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_UVWSRC(type, N) "$tex.uvwsrc",type,N

// for backward compatibility
#define AI_MATKEY_UVWSRC_DIFFUSE(N)	\
	AI_MATKEY_UVWSRC(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_UVWSRC_SPECULAR(N)	\
	AI_MATKEY_UVWSRC(aiTextureType_SPECULAR,N)

#define AI_MATKEY_UVWSRC_AMBIENT(N)	\
	AI_MATKEY_UVWSRC(aiTextureType_AMBIENT,N)

#define AI_MATKEY_UVWSRC_EMISSIVE(N)	\
	AI_MATKEY_UVWSRC(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_UVWSRC_NORMALS(N)	\
	AI_MATKEY_UVWSRC(aiTextureType_NORMALS,N)

#define AI_MATKEY_UVWSRC_HEIGHT(N)	\
	AI_MATKEY_UVWSRC(aiTextureType_HEIGHT,N)

#define AI_MATKEY_UVWSRC_SHININESS(N)	\
	AI_MATKEY_UVWSRC(aiTextureType_SHININESS,N)

#define AI_MATKEY_UVWSRC_OPACITY(N)	\
	AI_MATKEY_UVWSRC(aiTextureType_OPACITY,N)


// ---------------------------------------------------------------------------
/** @def AI_MATKEY_TEXOP 
 * Parameters: type, N<br>
 * Specifies how the of the <N>th texture of type <type> is combined with
 * the result of all color values from all previous textures combined.
 * <b>Type:</b> int (aiTextureOp)<br>
 * <b>Default value to be assumed if this key isn't there:</b>multiply<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE(type,N)<br>
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_TEXOP(type, N) "$tex.op",type,N

// for backward compatibility
#define AI_MATKEY_TEXOP_DIFFUSE(N)	\
	AI_MATKEY_TEXOP(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_TEXOP_SPECULAR(N)	\
	AI_MATKEY_TEXOP(aiTextureType_SPECULAR,N)

#define AI_MATKEY_TEXOP_AMBIENT(N)	\
	AI_MATKEY_TEXOP(aiTextureType_AMBIENT,N)

#define AI_MATKEY_TEXOP_EMISSIVE(N)	\
	AI_MATKEY_TEXOP(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_TEXOP_NORMALS(N)	\
	AI_MATKEY_TEXOP(aiTextureType_NORMALS,N)

#define AI_MATKEY_TEXOP_HEIGHT(N)	\
	AI_MATKEY_TEXOP(aiTextureType_HEIGHT,N)

#define AI_MATKEY_TEXOP_SHININESS(N)	\
	AI_MATKEY_TEXOP(aiTextureType_SHININESS,N)

#define AI_MATKEY_TEXOP_OPACITY(N)	\
	AI_MATKEY_TEXOP(aiTextureType_OPACITY,N)


// ---------------------------------------------------------------------------
/** @def AI_MATKEY_MAPPING 
 * Parameters: type, N<br>
 * Specifies how the of the <N>th texture of type <type>is mapped.
 * <br>
 * <b>Type:</b> int (aiTextureMapping)<br>
 * <b>Default value to be assumed if this key isn't there:</b>UV<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE(type,N)<br>
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_MAPPING(type, N) "$tex.mapping",type,N

// for backward compatibility
#define AI_MATKEY_MAPPING_DIFFUSE(N)	\
	AI_MATKEY_MAPPING(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_MAPPING_SPECULAR(N)	\
	AI_MATKEY_MAPPING(aiTextureType_SPECULAR,N)

#define AI_MATKEY_MAPPING_AMBIENT(N)	\
	AI_MATKEY_MAPPING(aiTextureType_AMBIENT,N)

#define AI_MATKEY_MAPPING_EMISSIVE(N)	\
	AI_MATKEY_MAPPING(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_MAPPING_NORMALS(N)	\
	AI_MATKEY_MAPPING(aiTextureType_NORMALS,N)

#define AI_MATKEY_MAPPING_HEIGHT(N)	\
	AI_MATKEY_MAPPING(aiTextureType_HEIGHT,N)

#define AI_MATKEY_MAPPING_SHININESS(N)	\
	AI_MATKEY_MAPPING(aiTextureType_SHININESS,N)

#define AI_MATKEY_MAPPING_OPACITY(N)	\
	AI_MATKEY_MAPPING(aiTextureType_OPACITY,N)

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_TEXBLEND (
 * Parameters: type, N<br>
 * Specifies the strength of the <N>th texture of type <type>. This is just
 * a multiplier for the texture's color values.
 * <br>
 * <b>Type:</b> float<br>
 * <b>Default value to be assumed if this key isn't there:</b> 1.f<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE(type,N)<br>
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_TEXBLEND(type, N) "$tex.blend",type,N

// for backward compatibility
#define AI_MATKEY_TEXBLEND_DIFFUSE(N)	\
	AI_MATKEY_TEXBLEND(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_TEXBLEND_SPECULAR(N)	\
	AI_MATKEY_TEXBLEND(aiTextureType_SPECULAR,N)

#define AI_MATKEY_TEXBLEND_AMBIENT(N)	\
	AI_MATKEY_TEXBLEND(aiTextureType_AMBIENT,N)

#define AI_MATKEY_TEXBLEND_EMISSIVE(N)	\
	AI_MATKEY_TEXBLEND(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_TEXBLEND_NORMALS(N)	\
	AI_MATKEY_TEXBLEND(aiTextureType_NORMALS,N)

#define AI_MATKEY_TEXBLEND_HEIGHT(N)	\
	AI_MATKEY_TEXBLEND(aiTextureType_HEIGHT,N)

#define AI_MATKEY_TEXBLEND_SHININESS(N)	\
	AI_MATKEY_TEXBLEND(aiTextureType_SHININESS,N)

#define AI_MATKEY_TEXBLEND_OPACITY(N)	\
	AI_MATKEY_TEXBLEND(aiTextureType_OPACITY,N)

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_MAPPINGMODE_U 
 * Parameters: type, N<br>
 * Specifies the texture mapping mode for the <N>th texture of type <type> in
 * the u (x) direction
 * <br>
 * <b>Type:</b> int (aiTextureMapMode)<br>
 * <b>Default value:</b> aiTextureMapMode_Wrap<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE(type,N)<br>
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_MAPPINGMODE_U(type, N) "$tex.mapmodeu",type,N

// for backward compatibility
#define AI_MATKEY_MAPPINGMODE_U_DIFFUSE(N)	\
	AI_MATKEY_MAPPINGMODE_U(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_MAPPINGMODE_U_SPECULAR(N)	\
	AI_MATKEY_MAPPINGMODE_U(aiTextureType_SPECULAR,N)

#define AI_MATKEY_MAPPINGMODE_U_AMBIENT(N)	\
	AI_MATKEY_MAPPINGMODE_U(aiTextureType_AMBIENT,N)

#define AI_MATKEY_MAPPINGMODE_U_EMISSIVE(N)	\
	AI_MATKEY_MAPPINGMODE_U(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_MAPPINGMODE_U_NORMALS(N)	\
	AI_MATKEY_MAPPINGMODE_U(aiTextureType_NORMALS,N)

#define AI_MATKEY_MAPPINGMODE_U_HEIGHT(N)	\
	AI_MATKEY_MAPPINGMODE_U(aiTextureType_HEIGHT,N)

#define AI_MATKEY_MAPPINGMODE_U_SHININESS(N)	\
	AI_MATKEY_MAPPINGMODE_U(aiTextureType_SHININESS,N)

#define AI_MATKEY_MAPPINGMODE_U_OPACITY(N)	\
	AI_MATKEY_MAPPINGMODE_U(aiTextureType_OPACITY,N)

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_MAPPINGMODE_V 
 * Parameters: type, N<br>
 * Specifies the texture mapping mode for the <N>th texture of type <type> in
 * the w (z) direction
 * <br>
 * <b>Type:</b> int (aiTextureMapMode)<br>
 * <b>Default value:</b> aiTextureMapMode_Wrap<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE(type,N)<br>
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_MAPPINGMODE_V(type, N) "$tex.mapmodev",type,N

// for backward compatibility
#define AI_MATKEY_MAPPINGMODE_V_DIFFUSE(N)	\
	AI_MATKEY_MAPPINGMODE_V(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_MAPPINGMODE_V_SPECULAR(N)	\
	AI_MATKEY_MAPPINGMODE_V(aiTextureType_SPECULAR,N)

#define AI_MATKEY_MAPPINGMODE_V_AMBIENT(N)	\
	AI_MATKEY_MAPPINGMODE_V(aiTextureType_AMBIENT,N)

#define AI_MATKEY_MAPPINGMODE_V_EMISSIVE(N)	\
	AI_MATKEY_MAPPINGMODE_V(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_MAPPINGMODE_V_NORMALS(N)	\
	AI_MATKEY_MAPPINGMODE_V(aiTextureType_NORMALS,N)

#define AI_MATKEY_MAPPINGMODE_V_HEIGHT(N)	\
	AI_MATKEY_MAPPINGMODE_V(aiTextureType_HEIGHT,N)

#define AI_MATKEY_MAPPINGMODE_V_SHININESS(N)	\
	AI_MATKEY_MAPPINGMODE_V(aiTextureType_SHININESS,N)

#define AI_MATKEY_MAPPINGMODE_V_OPACITY(N)	\
	AI_MATKEY_MAPPINGMODE_V(aiTextureType_OPACITY,N)

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_MAPPINGMODE_W 
 * Parameters: type, N<br>
 * Specifies the texture mapping mode for the <N>th texture of type <type> in
 * the w (z) direction
 * <br>
 * <b>Type:</b> int (aiTextureMapMode)<br>
 * <b>Default value:</b> aiTextureMapMode_Wrap<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE(type,N)<br>
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_MAPPINGMODE_W(type, N) "$tex.mapmodew",type,N

// for backward compatibility
#define AI_MATKEY_MAPPINGMODE_W_DIFFUSE(N)	\
	AI_MATKEY_MAPPINGMODE_W(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_MAPPINGMODE_W_SPECULAR(N)	\
	AI_MATKEY_MAPPINGMODE_W(aiTextureType_SPECULAR,N)

#define AI_MATKEY_MAPPINGMODE_W_AMBIENT(N)	\
	AI_MATKEY_MAPPINGMODE_W(aiTextureType_AMBIENT,N)

#define AI_MATKEY_MAPPINGMODE_W_EMISSIVE(N)	\
	AI_MATKEY_MAPPINGMODE_W(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_MAPPINGMODE_W_NORMALS(N)	\
	AI_MATKEY_MAPPINGMODE_W(aiTextureType_NORMALS,N)

#define AI_MATKEY_MAPPINGMODE_W_HEIGHT(N)	\
	AI_MATKEY_MAPPINGMODE_W(aiTextureType_HEIGHT,N)

#define AI_MATKEY_MAPPINGMODE_W_SHININESS(N)	\
	AI_MATKEY_MAPPINGMODE_W(aiTextureType_SHININESS,N)

#define AI_MATKEY_MAPPINGMODE_W_OPACITY(N)	\
	AI_MATKEY_MAPPINGMODE_W(aiTextureType_OPACITY,N)


// ---------------------------------------------------------------------------
/** @def AI_MATKEY_TEXMAP_AXIS
 * Parameters: type, N<br>
 * Specifies the main mapping axis <N>th texture of type <type>.
 * This applies to non-UV mapped textures. For spherical, cylindrical and
 * planar this is the main axis of the corresponding geometric shape.
 * <br>
 * <b>Type:</b> int (aiAxis)<br>
 * <b>Default value:</b> aiAxis_Z<br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE(type,N) and 
 * AI_MATKEY_TEXTURE_MAPPING(type,N) != UV<br>
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_TEXMAP_AXIS(type, N) "$tex.mapaxis",type,N

// ---------------------------------------------------------------------------
/** @def AI_MATKEY_UVTRANSFORM 
 * Parameters: type, N<br>
 * Specifies how the UV mapping coordinates for the<N>th texture of type
 * <type> are transformed before they're used for mapping. This is an array
 * of five floats - use the aiUVTransform structure for simplicity. 
 * <br>
 * <b>Type:</b> Array of 5 floats<br>
 * <b>Default value:</b> 0.f,0.f,1.f,1.f,0.f <br>
 * <b>Requires:</b> AI_MATKEY_TEXTURE(type,N) and 
 * AI_MATKEY_TEXTURE_MAPPING(type,N) == UV<br>
 * <b>Note:</b>Transformed 3D texture coordinates are not supported
 */
// ---------------------------------------------------------------------------
#define AI_MATKEY_UVTRANSFORM(type, N) "$tex.uvtrafo",type,N

// for backward compatibility
#define AI_MATKEY_UVTRANSFORM_DIFFUSE(N)	\
	AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_UVTRANSFORM_SPECULAR(N)	\
	AI_MATKEY_UVTRANSFORM(aiTextureType_SPECULAR,N)

#define AI_MATKEY_UVTRANSFORM_AMBIENT(N)	\
	AI_MATKEY_UVTRANSFORM(aiTextureType_AMBIENT,N)

#define AI_MATKEY_UVTRANSFORM_EMISSIVE(N)	\
	AI_MATKEY_UVTRANSFORM(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_UVTRANSFORM_NORMALS(N)	\
	AI_MATKEY_UVTRANSFORM(aiTextureType_NORMALS,N)

#define AI_MATKEY_UVTRANSFORM_HEIGHT(N)	\
	AI_MATKEY_UVTRANSFORM(aiTextureType_HEIGHT,N)

#define AI_MATKEY_UVTRANSFORM_SHININESS(N)	\
	AI_MATKEY_UVTRANSFORM(aiTextureType_SHININESS,N)

#define AI_MATKEY_UVTRANSFORM_OPACITY(N)	\
	AI_MATKEY_UVTRANSFORM(aiTextureType_OPACITY,N)




#define AI_MATKEY_ORENNAYAR_ROUGHNESS	 "$shading.orennayar.roughness",0,0
#define AI_MATKEY_MINNAERT_DARKNESS		 "$shading.minnaert.darkness",0,0
#define AI_MATKEY_COOK_TORRANCE_PARAM	 "$shading.cookt.param",0,0

/** @def AI_MATKEY_GLOBAL_BACKGROUND_IMAGE
*  Global property defined by some loaders. Contains the path to 
*  the image file to be used as background image.
*/
#define AI_MATKEY_GLOBAL_BACKGROUND_IMAGE "$global.bg.image2d",0,0


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
	aiTextureType type,
    unsigned int  index,
    const C_STRUCT aiMaterialProperty** pPropOut);


// ---------------------------------------------------------------------------
/** Retrieve an array of float values with a specific key 
*  from the material
*
* Pass one of the AI_MATKEY_XXX constants for the last three parameters (the
* example reads the AI_MATKEY_UVTRANSFORM property of the first diffuse texture)
* @begincode
*
* aiUVTransform trafo;
* unsigned int max = sizeof(aiUVTransform);
* if (AI_SUCCESS != aiGetMaterialFloatArray(mat, AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE,0),
*    (float*)&trafo, &max) || sizeof(aiUVTransform) != max)
* {
*   // error handling 
* }
* @endcode
*
* @param pMat Pointer to the input material. May not be NULL
* @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
* @param pOut Pointer to a buffer to receive the result. 
* @param pMax Specifies the size of the given buffer, in float's.
*        Receives the number of values (not bytes!) read. 
* @param type (see the code sample above)
* @param index (see the code sample above)
* @return Specifies whether the key has been found. If not, the output
*   arrays remains unmodified and pMax is set to 0.
*/
// ---------------------------------------------------------------------------
ASSIMP_API aiReturn aiGetMaterialFloatArray(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
	unsigned int type,
    unsigned int index,
    float* pOut,
    unsigned int* pMax);



#ifdef __cplusplus

// ---------------------------------------------------------------------------
/** Retrieve a single float property with a specific key from the material.
*
* Pass one of the AI_MATKEY_XXX constants for the last three parameters (the
* example reads the AI_MATKEY_SPECULAR_STRENGTH property of the first diffuse texture)
* @begincode
*
* float specStrength = 1.f; // default value, remains unmodified if we fail.
* aiGetMaterialFloat(mat, AI_MATKEY_SPECULAR_STRENGTH,
*    (float*)&specStrength);
* @endcode
*
* @param pMat Pointer to the input material. May not be NULL
* @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
* @param pOut Receives the output float.
* @param type (see the code sample above)
* @param index (see the code sample above)
* @return Specifies whether the key has been found. If not, the output
*   float remains unmodified.
*/
// ---------------------------------------------------------------------------
inline aiReturn aiGetMaterialFloat(const C_STRUCT aiMaterial* pMat, 
	const char* pKey,
	unsigned int type,
    unsigned int index,
	float* pOut)
{
	return aiGetMaterialFloatArray(pMat,pKey,type,index,pOut,(unsigned int*)0x0);
}

#else 

// Use our friend, the C preprocessor
#define aiGetMaterialFloat (pMat, type, index, pKey, pOut) \
    aiGetMaterialFloatArray(pMat, type, index, pKey, pOut, NULL)

#endif //!__cplusplus


// ---------------------------------------------------------------------------
/** Retrieve an array of integer values with a specific key 
*  from a material
*
* See the sample for aiGetMaterialFloatArray for more information.
*/
// ---------------------------------------------------------------------------
ASSIMP_API aiReturn aiGetMaterialIntegerArray(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
	unsigned int  type,
	unsigned int  index,
    int* pOut,
    unsigned int* pMax);


#ifdef __cplusplus

// ---------------------------------------------------------------------------
/** Retrieve an integer property with a specific key from a material
*
* See the sample for aiGetMaterialFloat for more information.
*/
// ---------------------------------------------------------------------------
inline aiReturn aiGetMaterialInteger(const C_STRUCT aiMaterial* pMat, 
	const char* pKey,
	unsigned int type,
    unsigned int index,
	int* pOut)
{
	return aiGetMaterialIntegerArray(pMat,pKey,type,index,pOut,(unsigned int*)0x0);
}

#else 

// use our friend, the C preprocessor
#define aiGetMaterialInteger (pMat, type, index, pKey, pOut) \
    aiGetMaterialIntegerArray(pMat, type, index, pKey, pOut, NULL)

#endif //!__cplusplus



// ---------------------------------------------------------------------------
/** Retrieve a color value from the material property table
*
* See the sample for aiGetMaterialFloat for more information.
*/
// ---------------------------------------------------------------------------
ASSIMP_API aiReturn aiGetMaterialColor(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
	unsigned int type,
    unsigned int index,
    aiColor4D* pOut);


// ---------------------------------------------------------------------------
/** Retrieve a string from the material property table
*
* See the sample for aiGetMaterialFloat for more information.
*/
// ---------------------------------------------------------------------------
ASSIMP_API aiReturn aiGetMaterialString(const C_STRUCT aiMaterial* pMat, 
    const char* pKey,
	unsigned int type,
    unsigned int index,
    aiString* pOut);


// ---------------------------------------------------------------------------
/** Helper function to get a texture from a material structure.
 *
 *  This function is provided just for convinience. 
 *  @param mat Pointer to the input material. May not be NULL
 *  @param index Index of the texture to retrieve. If the index is too 
 *		large the function fails.
 *  @param type Specifies the type of the texture to retrieve (e.g. diffuse,
 *     specular, height map ...)
 *  @param path Receives the output path
 *		NULL is no allowed as value
 *  @param uvindex Receives the UV index of the texture. 
 *		NULL is allowed as value. The return value is 
 *  @param blend Receives the blend factor for the texture
 *		NULL is allowed as value.
 *  @param op Receives the texture operation to perform between
 *		this texture and the previous texture. NULL is allowed as value.
 *  @param mapmode Receives the mapping modes to be used for the texture.
 *      The parameter may be NULL but if it is a valid pointer it MUST
 *      point to an array of 3 aiTextureMapMode variables (one for each
 *      axis: UVW order (=XYZ)). 
 */
// ---------------------------------------------------------------------------
#ifdef __cplusplus
ASSIMP_API aiReturn aiGetMaterialTexture(const C_STRUCT aiMaterial* mat,
	aiTextureType type,
    unsigned int  index,
    C_STRUCT aiString* path,
	aiTextureMapping* mapping	= NULL,
    unsigned int* uvindex		= NULL,
    float* blend				= NULL,
    aiTextureOp* op				= NULL,
	aiTextureMapMode* mapmode	= NULL); 
#else
aiReturn aiGetMaterialTexture(const C_STRUCT aiMaterial* mat,
    aiTextureType type,
    unsigned int  index,
    C_STRUCT aiString* path,
	aiTextureMapping* mapping	/*= NULL*/,
    unsigned int* uvindex		/*= NULL*/,
    float* blend				/*= NULL*/,
    aiTextureOp* op				/*= NULL*/,
	aiTextureMapMode* mapmode	/*= NULL*/); 
#endif // !#ifdef __cplusplus

#ifdef __cplusplus
}

#include "aiMaterial.inl"

#endif //!__cplusplus
#endif //!!AI_MATERIAL_H_INC
