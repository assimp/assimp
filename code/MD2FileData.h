/** @file Defines the helper data structures for importing MD2 files  */
#ifndef AI_MD2FILEHELPER_H_INC
#define AI_MD2FILEHELPER_H_INC

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
//http://linux.ucla.edu/~phaethon/q3/formats/md2-schoenblum.html
namespace MD2
{

#define AI_MD2_MAGIC_NUMBER_BE	'IDP2'
#define AI_MD2_MAGIC_NUMBER_LE	'2PDI'

// common limitations
#define AI_MD2_VERSION			15
#define AI_MD2_MAXQPATH			64
#define AI_MD2_MAX_FRAMES		512
#define AI_MD2_MAX_SKINS		32	
#define AI_MD2_MAX_VERTS		2048	
#define AI_MD2_MAX_TRIANGLES	4096	

// ---------------------------------------------------------------------------
/**	\brief Data structure for the MD2 main header
 */
// ---------------------------------------------------------------------------
struct Header
{
	int32_t magic; 
	int32_t version; 
	int32_t skinWidth; 
	int32_t skinHeight; 
	int32_t frameSize; 
	int32_t numSkins; 
	int32_t numVertices; 
	int32_t numTexCoords; 
	int32_t numTriangles; 
	int32_t numGlCommands; 
	int32_t numFrames; 
	int32_t offsetSkins; 
	int32_t offsetTexCoords; 
	int32_t offsetTriangles; 
	int32_t offsetFrames; 
	int32_t offsetGlCommands; 
	int32_t offsetEnd; 

} PACK_STRUCT;


// ---------------------------------------------------------------------------
/**	\brief Data structure for a MD2 OpenGl draw command
 */
// ---------------------------------------------------------------------------
struct GLCommand
{
   float s, t;
   uint32_t vertexIndex;
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/**	\brief Data structure for a MD2 triangle
 */
// ---------------------------------------------------------------------------
struct Triangle
{
	uint16_t vertexIndices[3];
	uint16_t textureIndices[3];
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/**	\brief Data structure for a MD2 vertex
 */
// ---------------------------------------------------------------------------
struct Vertex
{
	uint8_t vertex[3];
	uint8_t lightNormalIndex;
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/**	\brief Data structure for a MD2 frame
 */
// ---------------------------------------------------------------------------
struct Frame
{
	float scale[3];
	float translate[3];
	char name[16];
	Vertex vertices[1];
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/**	\brief Data structure for a MD2 texture coordinate
 */
// ---------------------------------------------------------------------------
struct TexCoord
{
	int16_t s;
	int16_t t;
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/**	\brief Data structure for a MD2 skin
 */
// ---------------------------------------------------------------------------
struct Skin
{
	char name[AI_MD2_MAXQPATH];              /* texture file name */
} PACK_STRUCT;

// reset packing to the original value
#if defined(_MSC_VER) ||  defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma pack( pop )
#endif
#undef PACK_STRUCT

};
};

#endif // !! include guard