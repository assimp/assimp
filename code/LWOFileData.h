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

/** @file Defines chunk constants used by the LWO file format

The chunks are taken from LWO2.h, found in the sourcecode of
a project called Nxabega (http://www.sourceforge.net/projects/nxabega).
I assume they are from the official LightWave SDK headers.
Original copyright notice: "Ernie Wright  17 Sep 00" 
 
*/
#ifndef AI_LWO_FILEDATA_INCLUDED
#define AI_LWO_FILEDATA_INCLUDED

// STL headers
#include <vector>
#include <list>

// public ASSIMP headers
#include "../include/aiMesh.h"

// internal headers
#include "IFF.h"
#include "SmoothingGroups.h"

namespace Assimp {
namespace LWO {

#define AI_LWO_FOURCC_LWOB AI_IFF_FOURCC('L','W','O','B')
#define AI_LWO_FOURCC_LWO2 AI_IFF_FOURCC('L','W','O','2')

// chunks specific to the LWOB format
#define AI_LWO_SRFS  AI_IFF_FOURCC('S','R','F','S')
#define AI_LWO_FLAG  AI_IFF_FOURCC('F','L','A','G')
#define AI_LWO_VLUM  AI_IFF_FOURCC('V','L','U','M')
#define AI_LWO_VDIF  AI_IFF_FOURCC('V','D','I','F')
#define AI_LWO_VSPC  AI_IFF_FOURCC('V','S','P','C')
#define AI_LWO_RFLT  AI_IFF_FOURCC('R','F','L','T')
#define AI_LWO_BTEX  AI_IFF_FOURCC('B','T','E','X')
#define AI_LWO_CTEX  AI_IFF_FOURCC('C','T','E','X')
#define AI_LWO_DTEX  AI_IFF_FOURCC('D','T','E','X')
#define AI_LWO_LTEX  AI_IFF_FOURCC('L','T','E','X')
#define AI_LWO_RTEX  AI_IFF_FOURCC('R','T','E','X')
#define AI_LWO_STEX  AI_IFF_FOURCC('S','T','E','X')
#define AI_LWO_TTEX  AI_IFF_FOURCC('T','T','E','X')
#define AI_LWO_TFLG  AI_IFF_FOURCC('T','F','L','G')
#define AI_LWO_TSIZ  AI_IFF_FOURCC('T','S','I','Z')
#define AI_LWO_TCTR  AI_IFF_FOURCC('T','C','T','R')
#define AI_LWO_TFAL  AI_IFF_FOURCC('T','F','A','L')
#define AI_LWO_TVEL  AI_IFF_FOURCC('T','V','E','L')
#define AI_LWO_TCLR  AI_IFF_FOURCC('T','C','L','R')
#define AI_LWO_TVAL  AI_IFF_FOURCC('T','V','A','L')
#define AI_LWO_TAMP  AI_IFF_FOURCC('T','A','M','P')
#define AI_LWO_TIMG  AI_IFF_FOURCC('T','I','M','G')
#define AI_LWO_TAAS  AI_IFF_FOURCC('T','A','A','S')
#define AI_LWO_TREF  AI_IFF_FOURCC('T','R','E','F')
#define AI_LWO_TOPC  AI_IFF_FOURCC('T','O','P','C')
#define AI_LWO_SDAT  AI_IFF_FOURCC('S','D','A','T')
#define AI_LWO_TFP0  AI_IFF_FOURCC('T','F','P','0')
#define AI_LWO_TFP1  AI_IFF_FOURCC('T','F','P','1')


/* top-level chunks */
#define AI_LWO_LAYR  AI_IFF_FOURCC('L','A','Y','R')
#define AI_LWO_TAGS  AI_IFF_FOURCC('T','A','G','S')
#define AI_LWO_PNTS  AI_IFF_FOURCC('P','N','T','S')
#define AI_LWO_BBOX  AI_IFF_FOURCC('B','B','O','X')
#define AI_LWO_VMAP  AI_IFF_FOURCC('V','M','A','P')
#define AI_LWO_VMAD  AI_IFF_FOURCC('V','M','A','D')
#define AI_LWO_POLS  AI_IFF_FOURCC('P','O','L','S')
#define AI_LWO_PTAG  AI_IFF_FOURCC('P','T','A','G')
#define AI_LWO_ENVL  AI_IFF_FOURCC('E','N','V','L')
#define AI_LWO_CLIP  AI_IFF_FOURCC('C','L','I','P')
#define AI_LWO_SURF  AI_IFF_FOURCC('S','U','R','F')
#define AI_LWO_DESC  AI_IFF_FOURCC('D','E','S','C')
#define AI_LWO_TEXT  AI_IFF_FOURCC('T','E','X','T')
#define AI_LWO_ICON  AI_IFF_FOURCC('I','C','O','N')

/* polygon types */
#define AI_LWO_FACE  AI_IFF_FOURCC('F','A','C','E')
#define AI_LWO_CURV  AI_IFF_FOURCC('C','U','R','V')
#define AI_LWO_PTCH  AI_IFF_FOURCC('P','T','C','H')
#define AI_LWO_MBAL  AI_IFF_FOURCC('M','B','A','L')
#define AI_LWO_BONE  AI_IFF_FOURCC('B','O','N','E')

/* polygon tags */
#define AI_LWO_SURF  AI_IFF_FOURCC('S','U','R','F')
#define AI_LWO_PART  AI_IFF_FOURCC('P','A','R','T')
#define AI_LWO_SMGP  AI_IFF_FOURCC('S','M','G','P')

/* envelopes */
#define AI_LWO_PRE   AI_IFF_FOURCC('P','R','E',' ')
#define AI_LWO_POST  AI_IFF_FOURCC('P','O','S','T')
#define AI_LWO_KEY   AI_IFF_FOURCC('K','E','Y',' ')
#define AI_LWO_SPAN  AI_IFF_FOURCC('S','P','A','N')
#define AI_LWO_TCB   AI_IFF_FOURCC('T','C','B',' ')
#define AI_LWO_HERM  AI_IFF_FOURCC('H','E','R','M')
#define AI_LWO_BEZI  AI_IFF_FOURCC('B','E','Z','I')
#define AI_LWO_BEZ2  AI_IFF_FOURCC('B','E','Z','2')
#define AI_LWO_LINE  AI_IFF_FOURCC('L','I','N','E')
#define AI_LWO_STEP  AI_IFF_FOURCC('S','T','E','P')

/* clips */
#define AI_LWO_STIL  AI_IFF_FOURCC('S','T','I','L')
#define AI_LWO_ISEQ  AI_IFF_FOURCC('I','S','E','Q')
#define AI_LWO_ANIM  AI_IFF_FOURCC('A','N','I','M')
#define AI_LWO_XREF  AI_IFF_FOURCC('X','R','E','F')
#define AI_LWO_STCC  AI_IFF_FOURCC('S','T','C','C')
#define AI_LWO_TIME  AI_IFF_FOURCC('T','I','M','E')
#define AI_LWO_CONT  AI_IFF_FOURCC('C','O','N','T')
#define AI_LWO_BRIT  AI_IFF_FOURCC('B','R','I','T')
#define AI_LWO_SATR  AI_IFF_FOURCC('S','A','T','R')
#define AI_LWO_HUE   AI_IFF_FOURCC('H','U','E',' ')
#define AI_LWO_GAMM  AI_IFF_FOURCC('G','A','M','M')
#define AI_LWO_NEGA  AI_IFF_FOURCC('N','E','G','A')
#define AI_LWO_IFLT  AI_IFF_FOURCC('I','F','L','T')
#define AI_LWO_PFLT  AI_IFF_FOURCC('P','F','L','T')

/* surfaces */
#define AI_LWO_COLR  AI_IFF_FOURCC('C','O','L','R')
#define AI_LWO_LUMI  AI_IFF_FOURCC('L','U','M','I')
#define AI_LWO_DIFF  AI_IFF_FOURCC('D','I','F','F')
#define AI_LWO_SPEC  AI_IFF_FOURCC('S','P','E','C')
#define AI_LWO_GLOS  AI_IFF_FOURCC('G','L','O','S')
#define AI_LWO_REFL  AI_IFF_FOURCC('R','E','F','L')
#define AI_LWO_RFOP  AI_IFF_FOURCC('R','F','O','P')
#define AI_LWO_RIMG  AI_IFF_FOURCC('R','I','M','G')
#define AI_LWO_RSAN  AI_IFF_FOURCC('R','S','A','N')
#define AI_LWO_TRAN  AI_IFF_FOURCC('T','R','A','N')
#define AI_LWO_TROP  AI_IFF_FOURCC('T','R','O','P')
#define AI_LWO_TIMG  AI_IFF_FOURCC('T','I','M','G')
#define AI_LWO_RIND  AI_IFF_FOURCC('R','I','N','D')
#define AI_LWO_TRNL  AI_IFF_FOURCC('T','R','N','L')
#define AI_LWO_BUMP  AI_IFF_FOURCC('B','U','M','P')
#define AI_LWO_SMAN  AI_IFF_FOURCC('S','M','A','N')
#define AI_LWO_SIDE  AI_IFF_FOURCC('S','I','D','E')
#define AI_LWO_CLRH  AI_IFF_FOURCC('C','L','R','H')
#define AI_LWO_CLRF  AI_IFF_FOURCC('C','L','R','F')
#define AI_LWO_ADTR  AI_IFF_FOURCC('A','D','T','R')
#define AI_LWO_SHRP  AI_IFF_FOURCC('S','H','R','P')
#define AI_LWO_LINE  AI_IFF_FOURCC('L','I','N','E')
#define AI_LWO_LSIZ  AI_IFF_FOURCC('L','S','I','Z')
#define AI_LWO_ALPH  AI_IFF_FOURCC('A','L','P','H')
#define AI_LWO_AVAL  AI_IFF_FOURCC('A','V','A','L')
#define AI_LWO_GVAL  AI_IFF_FOURCC('G','V','A','L')
#define AI_LWO_BLOK  AI_IFF_FOURCC('B','L','O','K')

/* texture layer */
#define AI_LWO_TYPE  AI_IFF_FOURCC('T','Y','P','E')
#define AI_LWO_CHAN  AI_IFF_FOURCC('C','H','A','N')
#define AI_LWO_NAME  AI_IFF_FOURCC('N','A','M','E')
#define AI_LWO_ENAB  AI_IFF_FOURCC('E','N','A','B')
#define AI_LWO_OPAC  AI_IFF_FOURCC('O','P','A','C')
#define AI_LWO_FLAG  AI_IFF_FOURCC('F','L','A','G')
#define AI_LWO_PROJ  AI_IFF_FOURCC('P','R','O','J')
#define AI_LWO_STCK  AI_IFF_FOURCC('S','T','C','K')
#define AI_LWO_TAMP  AI_IFF_FOURCC('T','A','M','P')

/* texture coordinates */
#define AI_LWO_TMAP  AI_IFF_FOURCC('T','M','A','P')
#define AI_LWO_AXIS  AI_IFF_FOURCC('A','X','I','S')
#define AI_LWO_CNTR  AI_IFF_FOURCC('C','N','T','R')
#define AI_LWO_SIZE  AI_IFF_FOURCC('S','I','Z','E')
#define AI_LWO_ROTA  AI_IFF_FOURCC('R','O','T','A')
#define AI_LWO_OREF  AI_IFF_FOURCC('O','R','E','F')
#define AI_LWO_FALL  AI_IFF_FOURCC('F','A','L','L')
#define AI_LWO_CSYS  AI_IFF_FOURCC('C','S','Y','S')

/* image map */
#define AI_LWO_IMAP  AI_IFF_FOURCC('I','M','A','P')
#define AI_LWO_IMAG  AI_IFF_FOURCC('I','M','A','G')
#define AI_LWO_WRAP  AI_IFF_FOURCC('W','R','A','P')
#define AI_LWO_WRPW  AI_IFF_FOURCC('W','R','P','W')
#define AI_LWO_WRPH  AI_IFF_FOURCC('W','R','P','H')
#define AI_LWO_VMAP  AI_IFF_FOURCC('V','M','A','P')
#define AI_LWO_AAST  AI_IFF_FOURCC('A','A','S','T')
#define AI_LWO_PIXB  AI_IFF_FOURCC('P','I','X','B')

/* procedural */
#define AI_LWO_PROC  AI_IFF_FOURCC('P','R','O','C')
#define AI_LWO_COLR  AI_IFF_FOURCC('C','O','L','R')
#define AI_LWO_VALU  AI_IFF_FOURCC('V','A','L','U')
#define AI_LWO_FUNC  AI_IFF_FOURCC('F','U','N','C')
#define AI_LWO_FTPS  AI_IFF_FOURCC('F','T','P','S')
#define AI_LWO_ITPS  AI_IFF_FOURCC('I','T','P','S')
#define AI_LWO_ETPS  AI_IFF_FOURCC('E','T','P','S')

/* gradient */
#define AI_LWO_GRAD  AI_IFF_FOURCC('G','R','A','D')
#define AI_LWO_GRST  AI_IFF_FOURCC('G','R','S','T')
#define AI_LWO_GREN  AI_IFF_FOURCC('G','R','E','N')
#define AI_LWO_PNAM  AI_IFF_FOURCC('P','N','A','M')
#define AI_LWO_INAM  AI_IFF_FOURCC('I','N','A','M')
#define AI_LWO_GRPT  AI_IFF_FOURCC('G','R','P','T')
#define AI_LWO_FKEY  AI_IFF_FOURCC('F','K','E','Y')
#define AI_LWO_IKEY  AI_IFF_FOURCC('I','K','E','Y')

/* shader */
#define AI_LWO_SHDR  AI_IFF_FOURCC('S','H','D','R')
#define AI_LWO_DATA  AI_IFF_FOURCC('D','A','T','A')


/* VMAP types */
#define AI_LWO_TXUV  AI_IFF_FOURCC('T','X','U','V')
#define AI_LWO_RGB   AI_IFF_FOURCC(' ','R','G','B')
#define AI_LWO_RGBA  AI_IFF_FOURCC('R','G','B','A')
#define AI_LWO_WGHT  AI_IFF_FOURCC('W','G','H','T')


// ---------------------------------------------------------------------------
/** \brief Data structure for a face in a LWO file
 *
 * \note We can't use the code in SmoothingGroups.inl here - the mesh
 *   structures of 3DS/ASE and LWO are too different. 
 */
struct Face : public aiFace
{
	Face() 
		: surfaceIndex(0)
		, smoothGroup(0)
	{}

	unsigned int surfaceIndex;
	unsigned int smoothGroup;
};


// ---------------------------------------------------------------------------
/** \brief Base structure for all vertex map representations
 */
struct VMapEntry
{
	VMapEntry(unsigned int _dims)
		:  dims(_dims)
	{}

	~VMapEntry() {delete[] rawData;}

	std::string name;
	float* rawData;
	unsigned int dims;
};

// ---------------------------------------------------------------------------
/** \brief Represents an extra vertex color channel
 */
struct VColorChannel : public VMapEntry
{
	VColorChannel(unsigned int num)
		: VMapEntry(4)
	{
		data = new aiColor4D[num];
		for (unsigned int i = 0; i < num;++i)
			data[i].a = 1.0f;
		rawData = (float*)data;
	}

	aiColor4D* data;
};

// ---------------------------------------------------------------------------
/** \brief Represents an extra vertex UV channel
 */
struct UVChannel : public VMapEntry
{
	UVChannel(unsigned int num)
		: VMapEntry(3)
	{
		data = new aiVector3D[num]; // to make the final copying easier
		rawData = (float*)data;
	}

	aiVector3D* data;
};

// ---------------------------------------------------------------------------
/** \brief Represents a weight map 
 */
struct WeightChannel : public VMapEntry
{
	WeightChannel(unsigned int num)
		: VMapEntry(1)
	{
		rawData = new float[num];
		for (unsigned int m = 0; m < num;++m)
			rawData[m] = 0.f;
	}
};


// ---------------------------------------------------------------------------
/** \brief Data structure for a LWO file texture
 */
struct Texture
{
	Texture()
		: mClipIdx(0xffffffff)
		, mStrength			(1.0f)
		, mUVChannelIndex	("unknown")
	{}

	//! File name of the texture
	std::string mFileName;

	//! Clip index
	unsigned int mClipIdx;

	//! Strength of the texture
	float mStrength;


	/*************** SPECIFIC TO LWO2 *********************/
	uint32_t type; // type of the texture

	//! Name of the corresponding UV channel
	std::string mUVChannelIndex;
};

// ---------------------------------------------------------------------------
/** \brief Data structure for a LWO file clip
 */
struct Clip
{
	//! path to the base texture
	std::string path;
};

// ---------------------------------------------------------------------------
/** \brief Data structure for a LWO file surface (= material)
 */
struct Surface
{
	Surface()
		: bDoubleSided			(false)
		, mDiffuseValue			(1.0f)
		, mSpecularValue		(1.0f)
		, mTransparency			(0.0f)
		, mGlossiness			(0.0f)
		, mLuminosity			(0.0f)
		, mMaximumSmoothAngle	(0.0f) // 0 == not specified
	{}

	//! Name of the surface
	std::string mName;

	//! Color of the surface
	aiColor3D mColor;

	//! true for two-sided materials
	bool bDoubleSided;

	//! Various material parameters
	float mDiffuseValue,mSpecularValue,mTransparency,mGlossiness,mLuminosity;

	//! Maximum angle between two adjacent triangles
	//! that they can be smoothed - in degrees
	float mMaximumSmoothAngle;

	//! Textures
	Texture mColorTexture,mDiffuseTexture,mSpecularTexture,
		mBumpTexture,mTransparencyTexture;
};

// ---------------------------------------------------------------------------
#define AI_LWO_VALIDATE_CHUNK_LENGTH(length,name,size) \
	if (length < size) \
	{ \
		DefaultLogger::get()->warn("LWO: "#name" chunk is too small"); \
		break; \
	} \


// some typedefs ... to make life with loader monsters like this easier
typedef std::vector	<	aiVector3D		>	PointList;
typedef std::vector	<	LWO::Face		>	FaceList;
typedef std::vector	<	LWO::Surface	>	SurfaceList;
typedef std::vector	<	std::string		>	TagList;
typedef std::vector	<	unsigned int	>	TagMappingTable;
typedef std::vector	<	WeightChannel	>	WeightChannelList;
typedef std::vector	<	VColorChannel	>	VColorChannelList;
typedef std::vector	<	UVChannel		>	UVChannelList;
typedef std::vector	<	Clip			>	ClipList;


// ---------------------------------------------------------------------------
/** \brief Represents a layer in the file
 */
struct Layer
{
	Layer()
		: mFaceIDXOfs(0)
		, mPointIDXOfs(0)
		, mParent (0x0)
	{}

	/** Temporary point list from the file */
	PointList mTempPoints;

	/** Weight channel list from the file */
	WeightChannelList mWeightChannels;

	/** Vertex color list from the file */
	VColorChannelList mVColorChannels;

	/** UV channel list from the file */
	UVChannelList mUVChannels;

	/** Temporary face list from the file*/
	FaceList mFaces;

	/** Current face indexing offset from the beginning of the buffers*/
	unsigned int mFaceIDXOfs;

	/** Current point indexing offset from the beginning of the buffers*/
	unsigned int mPointIDXOfs;

	/** Parent index */
	uint16_t mParent;

	/** Name of the layer */
	std::string mName;
};

typedef std::list<LWO::Layer>		LayerList;


}}


#endif // !! AI_LWO_FILEDATA_INCLUDED

