

/** @file Defines the material system of the library
 *
 */

#ifndef AI_MATERIAL_H_INC
#define AI_MATERIAL_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	aiPTI_Buffer = 0x5,
};


// ---------------------------------------------------------------------------
/** Defines algorithms for generating UVW-coords (for texture sampling)
*  procedurally.
*/
// ---------------------------------------------------------------------------
enum aiTexUVWGen
{
	/** The view vector will be reflected to a pixel's normal. 
	*
	*  The result is used as UVW-coordinate for 
	*  accessing a cubemap
	*/
	aiTexUVWGen_VIEWREFLEFT = 0x800001,


	/** The view vector will be used as UVW-src
	*
	*  The view vector is used as UVW-coordinate for 
	*  accessing a cubemap
	*/
	aiTexUVWGen_VIEW = 0x800002,


	/** The view vector will be refracted to the pixel's normal. 
	*
	*  If this is used, the refraction index to be applied should
	*  also be contained in the material description.
	*  The result is used as UVW-coordinate for 
	*  accessing a cubemap.
	*/
	aiTexUVWGen_VIEWREFRACT = 0x800003
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
	*	Keys are case insensitive.
	*/
	aiString* mKey;


	/**	Size of the buffer mData is pointing to, in bytes
	*/
	unsigned int mDataLength;


	/** Type information for the property.
	*
	*  Defines the data layout inside the
	*  data buffer. This is used by the library
	*  internally to perform debug checks.
	*/
	aiPropertyTypeInfo mType;


	/**	Binary buffer to hold the property's value
	*
	*  The buffer has no terminal character. However,
	*  if a string is stored inside it may use 0 as terminal,
	*  but it would be contained in mDataLength.
	*/
	char* mData;
};


// ---------------------------------------------------------------------------
/** Data structure for a material
*
*  Material data is stored using a key-value structure, called property
*  (to guarant that the system is maximally flexible).
*  The library defines a set of standard keys, which should be enough
*  for nearly all purposes. 
*/
// ---------------------------------------------------------------------------
#ifdef __cplusplus
class aiMaterial
{
protected:
	aiMaterial() {}
public:
#else
struct aiMaterial
{
#endif // __cplusplus
	/** List of all material properties loaded.
	*/
	aiMaterialProperty** mProperties;

	/** Number of properties loaded
	*/
	unsigned int mNumProperties;
	unsigned int mNumAllocated;
};


// ---------------------------------------------------------------------------
/** @def AI_MATKEY_NAME
*  Defines the name of the material (aiString)
*/
#define AI_MATKEY_NAME "$mat.name"

/** @def AI_MATKEY_SHADING_MODE
*  Defines the shading model to use (aiShadingMode)
*/
#define AI_MATKEY_SHADING_MODEL "$mat.shadingm"

/** @def AI_MATKEY_OPACITY
*  Defines the base opacity of the material
*/
#define AI_MATKEY_OPACITY "$mat.opacity"

/** @def AI_MATKEY_BUMPSCALING
*  Defines the height scaling of a bump map (for stuff like Parallax
*  Occlusion Mapping)
*/
#define AI_MATKEY_BUMPSCALING "$mat.bumpscaling"

/** @def AI_MATKEY_SHININESS
*  Defines the base shininess of the material
*/
#define AI_MATKEY_SHININESS "$mat.shininess"

/** @def AI_MATKEY_COLOR_DIFFUSE
*  Defines the diffuse base color of the material
*/
#define AI_MATKEY_COLOR_DIFFUSE "$clr.diffuse"

/** @def AI_MATKEY_COLOR_AMBIENT
*  Defines the ambient base color of the material
*/
#define AI_MATKEY_COLOR_AMBIENT "$clr.ambient"

/** @def AI_MATKEY_COLOR_SPECULAR
*  Defines the specular base color of the material
*/
#define AI_MATKEY_COLOR_SPECULAR "$clr.specular"

/** @def AI_MATKEY_COLOR_EMISSIVE
*  Defines the emissive base color of the material
*/
#define AI_MATKEY_COLOR_EMISSIVE "$clr.emissive"

/** @def AI_MATKEY_TEXTURE_DIFFUSE
*  Defines a specified diffuse texture channel of the material
*/
#define AI_MATKEY_TEXTURE_DIFFUSE(N) "$tex.file.diffuse["#N"]"
#define AI_MATKEY_TEXTURE_DIFFUSE_  "$tex.file.diffuse"

/** @def AI_MATKEY_TEXTURE_AMBIENT
*  Defines a specified ambient texture channel of the material
*/
#define AI_MATKEY_TEXTURE_AMBIENT(N) "$tex.file.ambient["#N"]"
#define AI_MATKEY_TEXTURE_AMBIENT_   "$tex.file.ambient"

/** @def AI_MATKEY_TEXTURE_SPECULAR
*  Defines a specified specular texture channel of the material
*/
#define AI_MATKEY_TEXTURE_SPECULAR(N) "$tex.file.specular["#N"]"
#define AI_MATKEY_TEXTURE_SPECULAR_   "$tex.file.specular"

/** @def AI_MATKEY_TEXTURE_EMISSIVE
*  Defines a specified emissive texture channel of the material
*/
#define AI_MATKEY_TEXTURE_EMISSIVE(N) "$tex.file.emissive["#N"]"
#define AI_MATKEY_TEXTURE_EMISSIVE_   "$tex.file.emissive"

/** @def AI_MATKEY_TEXTURE_NORMALS
*  Defines a specified normal texture channel of the material
*/
#define AI_MATKEY_TEXTURE_NORMALS(N) "$tex.file.normals["#N"]"
#define AI_MATKEY_TEXTURE_NORMALS_   "$tex.file.normals"

/** @def AI_MATKEY_TEXTURE_BUMP
* Defines a specified bumpmap texture (=heightmap) channel of the material
* This is very similar to #AI_MATKEY_TEXTURE_NORMALS. It is provided
* to allow applications to determine whether the input data for
* normal mapping is already a normal map or needs to be converted to
* a heightmap.
*/
#define AI_MATKEY_TEXTURE_BUMP(N) "$tex.file.bump["#N"]"
#define AI_MATKEY_TEXTURE_BUMP_   "$tex.file.bump"

/** @def AI_MATKEY_TEXTURE_SHININESS
*  Defines a specified shininess texture channel of the material
*/
#define AI_MATKEY_TEXTURE_SHININESS(N) "$tex.file.shininess["#N"]"
#define AI_MATKEY_TEXTURE_SHININESS_   "$tex.file.shininess"

/** @def AI_MATKEY_TEXTURE_OPACITY
*  Defines a specified opacity texture channel of the material
*/
#define AI_MATKEY_TEXTURE_OPACITY(N) "$tex.file.opacity["#N"]"
#define AI_MATKEY_TEXTURE_OPACITY_   "$tex.file.opacity"


#define AI_MATKEY_TEXOP_DIFFUSE(N)		"$tex.op.diffuse["#N"]"
#define AI_MATKEY_TEXOP_AMBIENT(N)		"$tex.op.ambient["#N"]"
#define AI_MATKEY_TEXOP_SPECULAR(N)		"$tex.op.specular["#N"]"
#define AI_MATKEY_TEXOP_EMISSIVE(N)		"$tex.op.emissive["#N"]"
#define AI_MATKEY_TEXOP_NORMALS(N)		"$tex.op.normals["#N"]"
#define AI_MATKEY_TEXOP_BUMP(N)			"$tex.op.bump["#N"]"
#define AI_MATKEY_TEXOP_SHININESS(N)	"$tex.op.shininess["#N"]"
#define AI_MATKEY_TEXOP_OPACITY(N)		"$tex.op.opacity["#N"]"

#define AI_MATKEY_UVWSRC_DIFFUSE(N)		"$tex.uvw.diffuse["#N"]"
#define AI_MATKEY_UVWSRC_AMBIENT(N)		"$tex.uvw.ambient["#N"]"
#define AI_MATKEY_UVWSRC_SPECULAR(N)	"$tex.uvw.specular["#N"]"
#define AI_MATKEY_UVWSRC_EMISSIVE(N)	"$tex.uvw.emissive["#N"]"
#define AI_MATKEY_UVWSRC_NORMALS(N)		"$tex.uvw.normals["#N"]"
#define AI_MATKEY_UVWSRC_BUMP(N)		"$tex.uvw.bump["#N"]"
#define AI_MATKEY_UVWSRC_SHININESS(N)	"$tex.uvw.shininess["#N"]"
#define AI_MATKEY_UVWSRC_OPACITY(N)		"$tex.uvw.opacity["#N"]"

#define AI_MATKEY_REFRACTI_DIFFUSE(N)	"$tex.refracti.diffuse["#N"]"
#define AI_MATKEY_REFRACTI_AMBIENT(N)	"$tex.refracti.ambient["#N"]"
#define AI_MATKEY_REFRACTI_SPECULAR(N)	"$tex.refracti.specular["#N"]"
#define AI_MATKEY_REFRACTI_EMISSIVE(N)	"$tex.refracti.emissive["#N"]"
#define AI_MATKEY_REFRACTI_NORMALS(N)	"$tex.refracti.normals["#N"]"
#define AI_MATKEY_REFRACTI_BUMP(N)		"$tex.refracti.bump["#N"]"
#define AI_MATKEY_REFRACTI_SHININESS(N)	"$tex.refracti.shininess["#N"]"
#define AI_MATKEY_REFRACTI_OPACITY(N)	"$tex.refracti.opacity["#N"]"

#define AI_MATKEY_TEXBLEND_DIFFUSE(N)	"$tex.blend.diffuse["#N"]"
#define AI_MATKEY_TEXBLEND_AMBIENT(N)	"$tex.blend.ambient["#N"]"
#define AI_MATKEY_TEXBLEND_SPECULAR(N)	"$tex.blend.specular["#N"]"
#define AI_MATKEY_TEXBLEND_EMISSIVE(N)	"$tex.blend.emissive["#N"]"
#define AI_MATKEY_TEXBLEND_NORMALS(N)	"$tex.blend.normals["#N"]"
#define AI_MATKEY_TEXBLEND_BUMP(N)		"$tex.blend.bump["#N"]"
#define AI_MATKEY_TEXBLEND_SHININESS(N)	"$tex.blend.shininess["#N"]"
#define AI_MATKEY_TEXBLEND_OPACITY(N)	"$tex.blend.opacity["#N"]"


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
aiReturn aiGetMaterialProperty(const aiMaterial* pMat, 
							   const char* pKey,
							   const aiMaterialProperty** pPropOut);


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
aiReturn aiGetMaterialFloatArray(const aiMaterial* pMat, 
								 const char* pKey,
								 float* pOut,
								 unsigned int* pMax);

#ifdef __cplusplus
// inline it
inline aiReturn aiGetMaterialFloat(const aiMaterial* pMat, 
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
aiReturn aiGetMaterialIntegerArray(const aiMaterial* pMat, 
								   const char* pKey,
								   int* pOut,
								   unsigned int* pMax);

#ifdef __cplusplus
// inline it
inline aiReturn aiGetMaterialInteger(const aiMaterial* pMat, 
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
aiReturn aiGetMaterialColor(const aiMaterial* pMat, 
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
aiReturn aiGetMaterialString(const aiMaterial* pMat, 
							 const char* pKey,
							 aiString* pOut);


#ifdef __cplusplus
}
#endif //!__cplusplus

#endif //!!AI_MATERIAL_H_INC
