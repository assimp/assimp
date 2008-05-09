/*
Free Asset Import Library (ASSIMP)
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


//
//! @file Definition of in-memory structures for the MDL file format. 
//
// The specification has been taken from various sources on the internet.
// http://tfc.duke.free.fr/coding/mdl-specs-en.html


#ifndef AI_MDLFILEHELPER_H_INC
#define AI_MDLFILEHELPER_H_INC

#include <string>
#include <vector>

#include "../include/aiTypes.h"
#include "../include/aiMesh.h"
#include "../include/aiAnim.h"
#include "../include/aiMaterial.h"

// ugly compiler dependent packing stuff
#if defined(_MSC_VER) ||  defined(__BORLANDC__) ||	defined (__BCPLUSPLUS__)
#	pragma pack(push,1)
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error Compiler not supported. Never do this again.
#endif


namespace Assimp
{
namespace MDL
{

// magic bytes used in Quake 1 MDL meshes
#define AI_MDL_MAGIC_NUMBER_BE	'IDPO'
#define AI_MDL_MAGIC_NUMBER_LE	'OPDI'

// magic bytes used in GameStudio A4 MDL meshes
#define AI_MDL_MAGIC_NUMBER_BE_GS4	'MDL3'
#define AI_MDL_MAGIC_NUMBER_LE_GS4	'3LDM'

// magic bytes used in GameStudio A5+ MDL meshes
#define AI_MDL_MAGIC_NUMBER_BE_GS5a	'MDL4'
#define AI_MDL_MAGIC_NUMBER_LE_GS5a	'4LDM'
#define AI_MDL_MAGIC_NUMBER_BE_GS5b	'MDL5'
#define AI_MDL_MAGIC_NUMBER_LE_GS5b	'5LDM'

// magic bytes used in GameStudio A6+ MDL meshes
#define AI_MDL_MAGIC_NUMBER_BE_GS6	'MDL6'
#define AI_MDL_MAGIC_NUMBER_LE_GS6	'6LDM'

// magic bytes used in GameStudio A7+ MDL meshes
#define AI_MDL_MAGIC_NUMBER_BE_GS7	'MDL7'
#define AI_MDL_MAGIC_NUMBER_LE_GS7	'7LDM'

// common limitations for Quake1 meshes. The loader does not check them,
// but models should not exceed these limits.
#if (!defined AI_MDL_VERSION)
#	define AI_MDL_VERSION				6
#endif
#if (!defined AI_MDL_MAX_FRAMES)
#	define AI_MDL_MAX_FRAMES			256
#endif
#if (!defined AI_MDL_MAX_UVS)
#	define AI_MDL_MAX_UVS				1024	
#endif
#if (!defined AI_MDL_MAX_VERTS)
#	define AI_MDL_MAX_VERTS				1024
#endif
#if (!defined AI_MDL_MAX_TRIANGLES)
#	define AI_MDL_MAX_TRIANGLES			2048	
#endif

// ---------------------------------------------------------------------------
/** \struct Header
 *  \brief Data structure for the MDL main header
 */
// ---------------------------------------------------------------------------
struct Header
{
	//! magic number: "IDPO"
	int32_t ident;          

	//! version number: 6
	int32_t version;          

	//! scale factors for each axis
	aiVector3D scale;				

	//! translation factors for each axis
	aiVector3D translate;	

	//! bounding radius of the mesh
	float boundingradius;
	 
	//! Position of the viewer's exe. Ignored
	aiVector3D vEyePos;

	//! Number of textures
	int32_t num_skins;       

	//! Texture width in pixels
	int32_t skinwidth;        

	//! Texture height in pixels
	int32_t skinheight;       

	//! Number of vertices contained in the file
	int32_t num_verts;       

	//! Number of triangles contained in the file
	int32_t num_tris;         

	//! Number of frames contained in the file
	int32_t num_frames;      

	//! 0 = synchron, 1 = random . Ignored
	int32_t synctype;         

	//! State flag
	int32_t flags;     

	//! ???
	float size;
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \struct Header_MDL7
 *  \brief Data structure for the MDL 7 main header
 */
// ---------------------------------------------------------------------------
struct Header_MDL7
{
	//! magic number: "MDL7"
	char	ident[4];		

	//! Version number. Ignored
	int32_t	version;		

	//! Number of bones in file
	int32_t	bones_num;

	//! Number of groups in file
	int32_t	groups_num;

	//! Size of data in the file
	int32_t	data_size;	

	//! Ignored. Used to store entity specific information
	int32_t	entlump_size;	

	//! Ignored. Used to store MED related data
	int32_t	medlump_size;	

	// -------------------------------------------------------
	// Sizes of some file parts

	uint16_t bone_stc_size;
	uint16_t skin_stc_size;
	uint16_t colorvalue_stc_size;
	uint16_t material_stc_size;
	uint16_t skinpoint_stc_size;
	uint16_t triangle_stc_size;
	uint16_t mainvertex_stc_size;
	uint16_t framevertex_stc_size;
	uint16_t bonetrans_stc_size;
	uint16_t frame_stc_size;
} PACK_STRUCT;

#define AI_MDL7_MAX_BONENAMESIZE	20 

// ---------------------------------------------------------------------------
/** \struct Bone_MDL7
 *  \brief Bone in a MDL7 file
 */
// ---------------------------------------------------------------------------
struct Bone_MDL7
{
	uint16_t parent_index;
	uint8_t _unused_[2]; // 
	float x,y,z;

	char name[AI_MDL7_MAX_BONENAMESIZE];
};

#define AI_MDL7_MAX_GROUPNAMESIZE	16

// ---------------------------------------------------------------------------
/** \struct Group_MDL7
 *  \brief Group in a MDL7 file
 */
// ---------------------------------------------------------------------------
struct Group_MDL7
{
	//! = '1' -> triangle based Mesh
	unsigned char	typ;		

	int8_t	deformers;
	int8_t	max_weights;
	int8_t	_unused_;

	//! size of data for this group in bytes ( MD7_GROUP stc. included).
	int32_t	groupdata_size; 
	char	name[AI_MDL7_MAX_GROUPNAMESIZE];

	//! Number of skins
	int32_t	numskins;

	//! Number of texture coordinates
	int32_t	num_stpts;	

	//! Number of triangles
	int32_t	numtris;

	//! Number of vertices
	int32_t	numverts;

	//! Number of frames
	int32_t	numframes;
} PACK_STRUCT;

#define	AI_MDL7_SKINTYPE_MIPFLAG				0x08
#define	AI_MDL7_SKINTYPE_MATERIAL				0x10
#define	AI_MDL7_SKINTYPE_MATERIAL_ASCDEF		0x20
#define	AI_MDL7_SKINTYPE_RGBFLAG				0x80


#define AI_MDL7_MAX_BONENAMESIZE 20

// ---------------------------------------------------------------------------
/** \struct Deformer_MDL7
 *  \brief Deformer in a MDL7 file
 */
// ---------------------------------------------------------------------------
struct Deformer_MDL7
{
	int8_t	deformer_version;		// 0
	int8_t	deformer_typ;			// 0 - bones
	int8_t	_unused_[2];
	int32_t	group_index;
	int32_t	elements;
	int32_t	deformerdata_size;
} PACK_STRUCT;

struct DeformerElement_MDL7
{
	//! bei deformer_typ==0 (==bones) element_index == bone index
	int32_t	element_index;		
	char	element_name[AI_MDL7_MAX_BONENAMESIZE];
	int32_t	weights;
} PACK_STRUCT;

struct DeformerWeight_MDL7
{
	//! for deformer_typ==0 (==bones) index == vertex index
	int32_t	index;				
	float	weight;
} PACK_STRUCT;

#define AI_MDL7_MAX_TEXNAMESIZE		0x10

typedef int32_t MD7_MATERIAL_ASCDEFSIZE;

// ---------------------------------------------------------------------------
/** \struct Skin_MDL7
 *  \brief Skin in a MDL7 file
 */
// ---------------------------------------------------------------------------
struct Skin_MDL7
{
	uint8_t			typ;
	int8_t			_unused_[3];
	int32_t			width;
	int32_t			height;
	char			texture_name[AI_MDL7_MAX_TEXNAMESIZE];	
} PACK_STRUCT;

struct ColorValue_MDL7
{
	float r,g,b,a;
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/** \struct Material_MDL7
 *  \brief Material in a MDL7 file
 */
// ---------------------------------------------------------------------------
struct Material_MDL7
{
	//! Diffuse base color of the material
	ColorValue_MDL7	Diffuse;        

	//! Ambient base color of the material
    ColorValue_MDL7	Ambient;  

	//! Specular base color of the material
    ColorValue_MDL7	Specular;  

	//! Emissive base color of the material
    ColorValue_MDL7	Emissive; 

	//! Phong power
    float			Power;         
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \struct Skin
 *  \brief Skin data structure #1
 */
// ---------------------------------------------------------------------------
struct Skin
{
	//! 0 = single (Skin), 1 = group (GroupSkin)
	//! For MDL3-5: Defines the type of the skin and there
	//! fore the size of the data to skip:
	//-------------------------------------------------------
	//! 2 for 565 RGB,
	//! 3 for 4444 ARGB, 
	//! 10 for 565 mipmapped, 
	//! 11 for 4444 mipmapped (bpp = 2),
	//! 12 for 888 RGB mipmapped (bpp = 3), 
	//! 13 for 8888 ARGB mipmapped (bpp = 4)
	//-------------------------------------------------------
	int32_t group;      

	//! Texture data
	uint8_t *data;  
} PACK_STRUCT;

struct Skin_MDL5
{
	int32_t size, width, height;      
	uint8_t *data;  
} PACK_STRUCT;

struct RGB565
{
	uint16_t r : 5;
	uint16_t g : 6;
	uint16_t b : 5;
} PACK_STRUCT;

struct ARGB4
{
	uint16_t a : 4;
	uint16_t r : 4;
	uint16_t g : 4;
	uint16_t b : 4;
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/** \struct GroupSkin
 *  \brief Skin data structure #2 (group of pictures)
 */
// ---------------------------------------------------------------------------
struct GroupSkin
{
	//! 0 = single (Skin), 1 = group (GroupSkin)
    int32_t group;     

	//! Number of images
	int32_t nb;       

	//! Time for each image
    float *time;   

	//! Data of each image
	uint8_t **data;  
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/** \struct TexCoord
 *  \brief Texture coordinate data structure
 */
// ---------------------------------------------------------------------------
struct TexCoord
{
	//! Is the vertex on the noundary between front and back piece?
	int32_t onseam;

	//! Texture coordinate in the tx direction
	int32_t s;

	//! Texture coordinate in the ty direction
	int32_t t;
} PACK_STRUCT;


struct TexCoord_MDL3
{
	//! position, horizontally in range 0..skinwidth-1
	int16_t u; 

	//! position, vertically in range 0..skinheight-1
	int16_t v; 
} PACK_STRUCT;

struct TexCoord_MDL7
{
	//! position, horizontally in range 0..1
	float u; 

	//! position, vertically in range 0..1
	float v; 
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \struct Triangle
 *  \brief Triangle data structure
 */
// ---------------------------------------------------------------------------
struct Triangle
{
	//! 0 = backface, 1 = frontface
	int32_t facesfront;  

	//! Vertex indices
	int32_t vertex[3];   
} PACK_STRUCT;


struct Triangle_MDL3
{
	//!  Index of 3 3D vertices in range 0..numverts
	uint16_t index_xyz[3];

	//! Index of 3 skin vertices in range 0..numskinverts
	uint16_t index_uv[3]; 
} PACK_STRUCT;


struct SkinSet_MDL7
{
	//! Index into the UV coordinate list
	uint16_t	st_index[3]; // size 6	

	//! Material index
	int32_t		material;	 // size 4				
} PACK_STRUCT;


struct Triangle_MDL7
{
	//! Vertex indices
	uint16_t   v_index[3]; 	// size 6

	//! Two skinsets. The second will be used for multi-texturing
	SkinSet_MDL7  skinsets[2];
} PACK_STRUCT; 


// Helper constants for Triangle::facesfront
#if (!defined AI_MDL_BACKFACE)
#	define AI_MDL_BACKFACE			0x0
#endif
#if (!defined  AI_MDL_FRONTFACE)
#	define AI_MDL_FRONTFACE			0x1
#endif

// ---------------------------------------------------------------------------
/** \struct Vertex
 *  \brief Vertex data structure
 */
// ---------------------------------------------------------------------------
struct Vertex
{
	uint8_t v[3];
	uint8_t normalIndex;
} PACK_STRUCT;


struct Vertex_MDL4
{
	uint16_t v[3];
	uint8_t normalIndex;
	uint8_t unused;
} PACK_STRUCT;

#define AI_MDL7_FRAMEVERTEX120503_STCSIZE		16
#define AI_MDL7_FRAMEVERTEX030305_STCSIZE		26

// ---------------------------------------------------------------------------
/** \struct Vertex_MDL7
 *  \brief Vertex data structure used in MDL7 files
 */
// ---------------------------------------------------------------------------
struct Vertex_MDL7
{
	float	x,y,z;
	uint16_t vertindex;	
	union {
		uint16_t norm162index;
		float norm[3];
	};
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \struct BoneTransform_MDL7
 *  \brief bone transformation matrix structure used in MDL7 files
 */
// ---------------------------------------------------------------------------
struct BoneTransform_MDL7
{
	//! 4*3
	float	m [4*4];				

	//! the index of this vertex, 0.. header::bones_num - 1
	uint16_t bone_index;		

	//! I HATE 3DGS AND THE SILLY DEVELOPER WHO DESIGNED
	//! THIS STUPID FILE FORMAT!
	int8_t _unused_[2];
} PACK_STRUCT;


#define AI_MDL7_MAX_FRAMENAMESIZE		16


// ---------------------------------------------------------------------------
/** \struct Frame_MDL7
 *  \brief Frame data structure used by MDL7 files
 */
// ---------------------------------------------------------------------------
struct Frame_MDL7
{
	char	frame_name[AI_MDL7_MAX_FRAMENAMESIZE];
	uint32_t	vertices_count;			
	uint32_t	transmatrix_count;		
};


// ---------------------------------------------------------------------------
/** \struct SimpleFrame
 *  \brief Data structure for a simple frame
 */
// ---------------------------------------------------------------------------
struct SimpleFrame
{
	//! Minimum vertex of the bounding box
	Vertex bboxmin; 

	//! Maximum vertex of the bounding box
	Vertex bboxmax; 

	//! Name of the frame
	char name[16];

	//! Vertex list of the frame
	Vertex *verts; 
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/** \struct Frame
 *  \brief Model frame data structure
 */
// ---------------------------------------------------------------------------
struct Frame
{
	//! 0 = simple frame, !0 = group frame
	int32_t type;       

	//! Frame data
	SimpleFrame frame;  	      
} PACK_STRUCT;


struct SimpleFrame_MDLn_SP
{
	//! Minimum vertex of the bounding box
	Vertex_MDL4 bboxmin; 

	//! Maximum vertex of the bounding box
	Vertex_MDL4 bboxmax; 

	//! Name of the frame
	char name[16];

	//! Vertex list of the frame
	Vertex_MDL4 *verts; 
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/** \struct GroupFrame
 *  \brief Data structure for a group of frames
 */
// ---------------------------------------------------------------------------
struct GroupFrame
{
	//! 0 = simple frame, !0 = group frame
	int32_t type;                         

	//! Minimum vertex for all single frames
	Vertex min;         

	//! Maximum vertex for all single frames
	Vertex max;         

	//! Time for all single frames
	float *time;                  

	//! List of single frames
	SimpleFrame *frames; 
} PACK_STRUCT;

// reset packing to the original value
#if defined(_MSC_VER) ||  defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma pack( pop )
#endif
#undef PACK_STRUCT


// ---------------------------------------------------------------------------
/** \struct IntFace_MDL7
 *  \brief Internal data structure to temporarily represent a face
 */
// ---------------------------------------------------------------------------
struct IntFace_MDL7
{
	// provide a constructor for our own convenience
	IntFace_MDL7()
	{
		// set everything to zero
		mIndices[0] = mIndices[1] = mIndices[2] = 0;
		iMatIndex[0] = iMatIndex[1] = 0;
	}

	//! Vertex indices
	uint32_t mIndices[3];

	//! Material index (maximally two channels, which are joined later)
	unsigned int iMatIndex[2];
}; 


// ---------------------------------------------------------------------------
/** \struct IntMaterial_MDL7
 *  \brief Internal data structure to temporarily represent a material
 *  which has been created from two single materials along with the
 *  original material indices.
 */
// ---------------------------------------------------------------------------
struct IntMaterial_MDL7
{
	// provide a constructor for our own convenience
	IntMaterial_MDL7()
	{
		pcMat = NULL;
		iOldMatIndices[0] = iOldMatIndices[1] = 0;
	}

	//! Material instance
	MaterialHelper* pcMat;

	//! Old material indices
	unsigned int iOldMatIndices[2];
};

};}; // end namespaces

#endif // !! AI_MDLFILEHELPER_H_INC