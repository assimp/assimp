/** @file Defines the helper data structures for importing MD3 files  */
#ifndef AI_MD3FILEHELPER_H_INC
#define AI_MD3FILEHELPER_H_INC

#include <string>
#include <vector>
#include <sstream>

#include "../include/aiTypes.h"
#include "../include/aiMesh.h"
#include "../include/aiAnim.h"

#if defined(_MSC_VER) ||  defined(__BORLANDC__) ||	defined (__BCPLUSPLUS__)
#	pragma pack(push,1)
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error Compiler not supported
#endif


namespace Assimp
{
// http://linux.ucla.edu/~phaethon/q3/formats/md3format.html
namespace MD3
{

#define AI_MD3_MAGIC_NUMBER_BE	'IDP3'
#define AI_MD3_MAGIC_NUMBER_LE	'3PDI'

// common limitations
#define AI_MD3_VERSION			15
#define AI_MD3_MAXQPATH			64
#define AI_MD3_MAX_FRAMES		1024
#define AI_MD3_MAX_TAGS			16
#define AI_MD3_MAX_SURFACES		32
#define AI_MD3_MAX_SHADERS		256	
#define AI_MD3_MAX_VERTS		4096	
#define AI_MD3_MAX_TRIANGLES	8192	

// master scale factor for all vertices in a MD3 model
#define AI_MD3_XYZ_SCALE		(1.0f/64.0f)

// ---------------------------------------------------------------------------
/**	\brief Data structure for the MD3 main header
 */
// ---------------------------------------------------------------------------
struct Header
{
	// magic number
	int32_t IDENT;

	// file format version
	int32_t VERSION;

	// original name in .pak archive
	unsigned char NAME[ AI_MD3_MAXQPATH ];

	// unknown
	int32_t FLAGS;

	// number of frames in the file
	int32_t NUM_FRAMES;

	// number of tags in the file
	int32_t NUM_TAGS;

	// number of surfaces in the file
	int32_t NUM_SURFACES;

	// number of skins in the file
	int32_t NUM_SKINS;

	// offset of the first frame
	int32_t OFS_FRAMES;

	// offset of the first tag
	int32_t OFS_TAGS;

	// offset of the first surface
	int32_t OFS_SURFACES;

	// end of file
	int32_t OFS_EOF;
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/**	\brief Data structure for the frame header
 */
// ---------------------------------------------------------------------------
struct Frame
{
	// no need to define this, we won't need
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/**	\brief Data structure for the tag header
 */
// ---------------------------------------------------------------------------
struct Tag
{
	// no need to define this, we won't need
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/**	\brief Data structure for the surface header
 */
// ---------------------------------------------------------------------------
struct Surface
{
	// magic number
	int32_t IDENT;

	// original name of the surface
	unsigned char NAME[ AI_MD3_MAXQPATH ];

	// unknown
	int32_t FLAGS;

	// number of frames in the surface
	int32_t NUM_FRAMES;

	// number of shaders in the surface
	int32_t NUM_SHADER;

	// number of vertices in the surface
	int32_t NUM_VERTICES;

	// number of triangles in the surface
	int32_t NUM_TRIANGLES;


	// offset to the triangle data 
	int32_t OFS_TRIANGLES;

	// offset to the shader data
	int32_t OFS_SHADERS;

	// offset to the texture coordinate data
	int32_t OFS_ST;

	// offset to the vertex/normal data
	int32_t OFS_XYZNORMAL;

	// offset to the end of the Surface object
	int32_t OFS_END;
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/**	\brief Data structure for a shader
 */
// ---------------------------------------------------------------------------
struct Shader
{
	// filename of the shader
	unsigned char NAME[ AI_MD3_MAXQPATH ];

	// index of the shader
	int32_t SHADER_INDEX;
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/**	\brief Data structure for a triangle
 */
// ---------------------------------------------------------------------------
struct Triangle
{
	// triangle indices
	int32_t INDEXES[3];
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/**	\brief Data structure for an UV coord
 */
// ---------------------------------------------------------------------------
struct TexCoord
{
	// UV coordinates
	float U,V;
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/**	\brief Data structure for a vertex
 */
// ---------------------------------------------------------------------------
struct Vertex
{
	// X/Y/Z coordinates
	int16_t X,Y,Z;

	// encoded normal vector
	int16_t  NORMAL;
} PACK_STRUCT;

// reset packing to the original value
#if defined(_MSC_VER) ||  defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma pack( pop )
#endif
#undef PACK_STRUCT

// ---------------------------------------------------------------------------
/**	\brief Unpack a Q3 16 bit vector to its full float3 representation
 *
 *	\param p_iNormal Input normal vector in latitude/longitude form
 *	\param p_afOut Pointer to an array of three floats to receive the result
 *
 *	\note This has been taken from q3 source (misc_model.c)
 */
// ---------------------------------------------------------------------------
inline void LatLngNormalToVec3(uint16_t p_iNormal, float* p_afOut)
{
	float lat = (float)(( p_iNormal >> 8 ) & 0xff);
	float lng = (float)(( p_iNormal & 0xff ));
	lat *= 3.141926f/128.0f;
	lng *= 3.141926f/128.0f;

	p_afOut[0] = cosf(lat) * sinf(lng);
	p_afOut[1] = sinf(lat) * sinf(lng);
	p_afOut[2] = cosf(lng);
	return;
}


// ---------------------------------------------------------------------------
/**	\brief Pack a Q3 normal into 16bit latitute/longitude representation
 *	\param p_vIn Input vector
 *	\param p_iOut Output normal
 *
 *	\note This has been taken from q3 source (mathlib.c)
 */
// ---------------------------------------------------------------------------
inline void Vec3NormalToLatLng( const aiVector3D& p_vIn, uint16_t& p_iOut ) 
{
	// check for singularities
	if ( 0.0f == p_vIn[0] && 0.0f == p_vIn[1] ) 
	{
		if ( p_vIn[2] > 0.0f ) 
		{
			((unsigned char*)&p_iOut)[0] = 0;
			((unsigned char*)&p_iOut)[1] = 0;		// lat = 0, long = 0
		} 
		else 
		{
			((unsigned char*)&p_iOut)[0] = 128;
			((unsigned char*)&p_iOut)[1] = 0;		// lat = 0, long = 128
		}
	} 
	else 
	{
		int	a, b;

		a = int(57.2957795f * ( atan2f( p_vIn[1], p_vIn[0] ) ) * (255.0f / 360.0f ));
		a &= 0xff;

		b = int(57.2957795f * ( acosf( p_vIn[2] ) ) * ( 255.0f / 360.0f ));
		b &= 0xff;

		((unsigned char*)&p_iOut)[0] = b;	// longitude
		((unsigned char*)&p_iOut)[1] = a;	// lattitude
	}
}

};
};

#endif // !! AI_MD3FILEHELPER_H_INC
