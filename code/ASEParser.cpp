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

/** @file Implementation of the ASE parser class */

// internal headers
#include "TextureTransform.h"
#include "ASELoader.h"
#include "MaterialSystem.h"
#include "fast_atof.h"

// public ASSIMP headers
#include "../include/DefaultLogger.h"
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"

using namespace Assimp;
using namespace Assimp::ASE;

#if (defined BLUBB)
#	undef BLUBB
#endif
#define BLUBB(_message_) \
	{LogError(_message_);return;}

// ------------------------------------------------------------------------------------------------
#define AI_ASE_HANDLE_TOP_LEVEL_SECTION(iDepth) \
	else if ('{' == *m_szFile)iDepth++; \
	else if ('}' == *m_szFile) \
	{ \
		if (0 == --iDepth) \
		{ \
			++m_szFile; \
			SkipToNextToken(); \
			return; \
		} \
	} \
	else if ('\0' == *m_szFile) \
	{ \
		return; \
	} \
	if(IsLineEnd(*m_szFile) && !bLastWasEndLine) \
	{ \
		++iLineNumber; \
		bLastWasEndLine = true; \
	} else bLastWasEndLine = false; \
	++m_szFile; 

// ------------------------------------------------------------------------------------------------
#define AI_ASE_HANDLE_SECTION(iDepth, level, msg) \
	if ('{' == *m_szFile)iDepth++; \
	else if ('}' == *m_szFile) \
	{ \
		if (0 == --iDepth) \
		{ \
			++m_szFile; \
			SkipToNextToken(); \
			return; \
		} \
	} \
	else if ('\0' == *m_szFile) \
	{ \
		LogError("Encountered unexpected EOL while parsing a " msg \
		" chunk (Level " level ")"); \
	} \
	if(IsLineEnd(*m_szFile) && !bLastWasEndLine) \
		{ \
		++iLineNumber; \
		bLastWasEndLine = true; \
	} else bLastWasEndLine = false; \
	++m_szFile; 

#ifdef _MSC_VER
#	define sprintf sprintf_s
#endif

// ------------------------------------------------------------------------------------------------
Parser::Parser (const char* szFile)
{
	ai_assert(NULL != szFile);
	m_szFile = szFile;

	// makre sure that the color values are invalid
	m_clrBackground.r = std::numeric_limits<float>::quiet_NaN();
	m_clrAmbient.r = std::numeric_limits<float>::quiet_NaN();

	iLineNumber = 0;
	iFirstFrame = 0;
	iLastFrame = 0;
	iFrameSpeed = 30;    // use 30 as default value for this property
	iTicksPerFrame = 1;  // use 1 as default value for this property
	bLastWasEndLine = false; // need to handle \r\n seqs due to binary file mapping
}
// ------------------------------------------------------------------------------------------------
void Parser::LogWarning(const char* szWarn)
{
	ai_assert(NULL != szWarn);

	char szTemp[1024];
#if _MSC_VER >= 1400
	sprintf_s(szTemp,"Line %i: %s",iLineNumber,szWarn);
#else
	ai_assert(strlen(szWarn) < 950);
	sprintf(szTemp,"Line %i: %s",iLineNumber,szWarn);
#endif

	// output the warning to the logger ...
	DefaultLogger::get()->warn(szTemp);
}
// ------------------------------------------------------------------------------------------------
void Parser::LogInfo(const char* szWarn)
{
	ai_assert(NULL != szWarn);

	char szTemp[1024];
#if _MSC_VER >= 1400
	sprintf_s(szTemp,"Line %i: %s",iLineNumber,szWarn);
#else
	ai_assert(strlen(szWarn) < 950);
	sprintf(szTemp,"Line %i: %s",iLineNumber,szWarn);
#endif

	// output the information to the logger ...
	DefaultLogger::get()->info(szTemp);
}
// ------------------------------------------------------------------------------------------------
void Parser::LogError(const char* szWarn)
{
	ai_assert(NULL != szWarn);

	char szTemp[1024];
#if _MSC_VER >= 1400
	sprintf_s(szTemp,"Line %i: %s",iLineNumber,szWarn);
#else
	ai_assert(strlen(szWarn) < 950);
	sprintf(szTemp,"Line %i: %s",iLineNumber,szWarn);
#endif

	// throw an exception
	throw new ImportErrorException(szTemp);
}
// ------------------------------------------------------------------------------------------------
bool Parser::SkipToNextToken()
{
	while (true)
	{
		char me = *m_szFile;

		// increase the line number counter if necessary
		if (IsLineEnd(me) && !bLastWasEndLine)
		{
			++iLineNumber;
			bLastWasEndLine = true;
		}
		else bLastWasEndLine = false;
		if ('*' == me || '}' == me || '{' == me)return true;
		if ('\0' == me)return false;

		++m_szFile;
	}
}
// ------------------------------------------------------------------------------------------------
bool Parser::SkipSection()
{
	// must handle subsections ...
	int iCnt = 0;
	while (true)
	{
		if ('}' == *m_szFile)
		{
			--iCnt;
			if (0 == iCnt)
			{
				// go to the next valid token ...
				++m_szFile;
				SkipToNextToken();
				return true;
			}
		}
		else if ('{' == *m_szFile)
		{
			++iCnt;
		}
		else if ('\0' == *m_szFile)
		{
			LogWarning("Unable to parse block: Unexpected EOF, closing bracket \'}\' was expected [#1]");	
			return false;
		}
		else if(IsLineEnd(*m_szFile))++iLineNumber;
		++m_szFile;
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::Parse()
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// version should be 200. Validate this ...
			if (0 == strncmp(m_szFile,"*3DSMAX_ASCIIEXPORT",19) &&
				IsSpaceOrNewLine(*(m_szFile+19)))
			{
				m_szFile+=20;

				unsigned int iVersion;
				ParseLV4MeshLong(iVersion);

				if (200 != iVersion)
				{
					LogWarning("Unknown file format version: *3DSMAX_ASCIIEXPORT should \
						be 200. Continuing happily ...");
				}
				continue;
			}
			// main scene information
			if (0 == strncmp(m_szFile,"*SCENE",6) &&
				IsSpaceOrNewLine(*(m_szFile+6)))
			{
				m_szFile+=7;
				ParseLV1SceneBlock();
				continue;
			}
			// material list
			if (0 == strncmp(m_szFile,"*MATERIAL_LIST",14) &&
				IsSpaceOrNewLine(*(m_szFile+14)))
			{
				m_szFile+=15;
				ParseLV1MaterialListBlock();
				continue;
			}
			// geometric object (mesh)
			if (0 == strncmp(m_szFile,"*GEOMOBJECT",11) &&
				IsSpaceOrNewLine(*(m_szFile+11)))
			{
				m_szFile+=12;
				m_vMeshes.push_back(Mesh());
				ParseLV1GeometryObjectBlock(m_vMeshes.back());
				continue;
			}
			// helper object = dummy in the hierarchy
			if (0 == strncmp(m_szFile,"*HELPEROBJECT",13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				m_vMeshes.push_back(Mesh());
				ParseLV1GeometryObjectBlock(m_vMeshes.back());
				continue;
			}
			// ignore comments, lights and cameras
			// (display comments on the console)
			if (0 == strncmp(m_szFile,"*LIGHTOBJECT",12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;
				LogInfo("Found light source (*LIGHTOBJECT chunk). It will be ignored");
				SkipSection();
				continue;
			}
			if (0 == strncmp(m_szFile,"*CAMERAOBJECT",13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				LogInfo("Found virtual camera (*CAMERAOBJECT chunk). It will be ignored");
				SkipSection();
				continue;
			}
			if (0 == strncmp(m_szFile,"*COMMENT",8) &&
				IsSpaceOrNewLine(*(m_szFile+8)))
			{
				m_szFile+=9;
				std::string out = "<unknown>";
				ParseString(out,"*COMMENT");
				LogInfo(("Comment: " + out).c_str());
				continue;
			}
		}
		AI_ASE_HANDLE_TOP_LEVEL_SECTION(iDepth);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV1SceneBlock()
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			if (0 == strncmp(m_szFile,"*SCENE_BACKGROUND_STATIC",24) &&
				IsSpaceOrNewLine(*(m_szFile+24)))
			{
				m_szFile+=25;

				// parse a color triple and assume it is really the bg color
				ParseLV4MeshFloatTriple( &m_clrBackground.r );
				continue;
			}
			if (0 == strncmp(m_szFile,"*SCENE_AMBIENT_STATIC",21) &&
				IsSpaceOrNewLine(*(m_szFile+21)))
			{
				m_szFile+=22;

				// parse a color triple and assume it is really the bg color
				ParseLV4MeshFloatTriple( &m_clrAmbient.r );
				continue;
			}
			if (0 == strncmp(m_szFile,"*SCENE_FIRSTFRAME",17) &&
				IsSpaceOrNewLine(*(m_szFile+17)))
			{
				m_szFile+=18;
				ParseLV4MeshLong(iFirstFrame);
				continue;
			}
			if (0 == strncmp(m_szFile,"*SCENE_LASTFRAME",16) &&
				IsSpaceOrNewLine(*(m_szFile+16)))
			{
				m_szFile+=17;
				ParseLV4MeshLong(iLastFrame);
				continue;
			}
			if (0 == strncmp(m_szFile,"*SCENE_FRAMESPEED",17) &&
				IsSpaceOrNewLine(*(m_szFile+17)))
			{
				m_szFile+=18;
				ParseLV4MeshLong(iFrameSpeed);
				continue;
			}
			if (0 == strncmp(m_szFile,"*SCENE_TICKSPERFRAME",20) &&
				IsSpaceOrNewLine(*(m_szFile+20)))
			{
				m_szFile+=21;
				ParseLV4MeshLong(iTicksPerFrame);
				continue;
			}
		}
		AI_ASE_HANDLE_TOP_LEVEL_SECTION(iDepth);
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV1MaterialListBlock()
{
	int iDepth = 0;
	unsigned int iMaterialCount = 0;
	unsigned int iOldMaterialCount = (unsigned int)m_vMaterials.size();
	while (true)
	{
		if ('*' == *m_szFile)
		{
			if (0 == strncmp(m_szFile,"*MATERIAL_COUNT",15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV4MeshLong(iMaterialCount);

				// now allocate enough storage to hold all materials
				m_vMaterials.resize(iOldMaterialCount+iMaterialCount);
				continue;
			}
			if (0 == strncmp(m_szFile,"*MATERIAL",9) &&
				IsSpaceOrNewLine(*(m_szFile+9)))
			{
				m_szFile+=10;
				unsigned int iIndex = 0;
				ParseLV4MeshLong(iIndex);

				if (iIndex >= iMaterialCount)
				{
					LogWarning("Out of range: material index is too large");
					iIndex = iMaterialCount-1;
				}

				// get a reference to the material
				Material& sMat = m_vMaterials[iIndex+iOldMaterialCount];
				// parse the material block
				ParseLV2MaterialBlock(sMat);
				continue;
			}
		}
		AI_ASE_HANDLE_TOP_LEVEL_SECTION(iDepth);
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV2MaterialBlock(ASE::Material& mat)
{
	int iDepth = 0;
	unsigned int iNumSubMaterials = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			if (0 == strncmp(m_szFile,"*MATERIAL_NAME",14) &&
				IsSpaceOrNewLine(*(m_szFile+14)))
			{
				m_szFile+=15;
			
				if (!ParseString(mat.mName,"*MATERIAL_NAME"))SkipToNextToken();
				continue;
			}
			// ambient material color
			if (0 == strncmp(m_szFile,"*MATERIAL_AMBIENT",17) &&
				IsSpaceOrNewLine(*(m_szFile+17)))
			{
				m_szFile+=18;
				ParseLV4MeshFloatTriple(&mat.mAmbient.r);continue;
			}
			// diffuse material color
			if (0 == strncmp(m_szFile,"*MATERIAL_DIFFUSE",17) &&
				IsSpaceOrNewLine(*(m_szFile+17)))
			{
				m_szFile+=18;
				ParseLV4MeshFloatTriple(&mat.mDiffuse.r);continue;
			}
			// specular material color
			if (0 == strncmp(m_szFile,"*MATERIAL_SPECULAR",18) &&
				IsSpaceOrNewLine(*(m_szFile+18)))
			{
				m_szFile+=19;
				ParseLV4MeshFloatTriple(&mat.mSpecular.r);continue;
			}
			// material shading type
			if (0 == strncmp(m_szFile,"*MATERIAL_SHADING",17) &&
				IsSpaceOrNewLine(*(m_szFile+17)))
			{
				m_szFile+=18;
				
				if (0 == strncmp(m_szFile,"Blinn",5) && 
					IsSpaceOrNewLine(*(m_szFile+5)))
				{
					mat.mShading = Dot3DSFile::Blinn;
					m_szFile+=6;
				}
				else if (0 == strncmp(m_szFile,"Phong",5) && 
					IsSpaceOrNewLine(*(m_szFile+5)))
				{
					mat.mShading = Dot3DSFile::Phong;
					m_szFile+=6;
				}
				else if (0 == strncmp(m_szFile,"Flat",4) && 
					IsSpaceOrNewLine(*(m_szFile+4)))
				{
					mat.mShading = Dot3DSFile::Flat;
					m_szFile+=5;
				}
				else if (0 == strncmp(m_szFile,"Wire",4) && 
					IsSpaceOrNewLine(*(m_szFile+4)))
				{
					mat.mShading = Dot3DSFile::Wire;
					m_szFile+=5;
				}
				else
				{
					// assume gouraud shading
					mat.mShading = Dot3DSFile::Gouraud;
					SkipToNextToken();
				}
				continue;
			}
			// material transparency
			if (0 == strncmp(m_szFile,"*MATERIAL_TRANSPARENCY",22) &&
				IsSpaceOrNewLine(*(m_szFile+22)))
			{
				m_szFile+=23;
				ParseLV4MeshFloat(mat.mTransparency);
				mat.mTransparency = 1.0f - mat.mTransparency;continue;
			}
			// material self illumination
			if (0 == strncmp(m_szFile,"*MATERIAL_SELFILLUM",19) &&
				IsSpaceOrNewLine(*(m_szFile+19)))
			{
				m_szFile+=20;
				float f = 0.0f;
				ParseLV4MeshFloat(f);

				mat.mEmissive.r = f;
				mat.mEmissive.g = f;
				mat.mEmissive.b = f;
				continue;
			}
			// material shininess
			if (0 == strncmp(m_szFile,"*MATERIAL_SHINE",15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV4MeshFloat(mat.mSpecularExponent);
				mat.mSpecularExponent *= 15;continue;
			}
			// material shininess strength
			if (0 == strncmp(m_szFile,"*MATERIAL_SHINESTRENGTH",23) &&
				IsSpaceOrNewLine(*(m_szFile+23)))
			{
				m_szFile+=24;
				ParseLV4MeshFloat(mat.mShininessStrength);continue;
			}
			// diffuse color map
			if (0 == strncmp(m_szFile,"*MAP_DIFFUSE",12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;
				// parse the texture block
				ParseLV3MapBlock(mat.sTexDiffuse);continue;
			}
			// ambient color map
			if (0 == strncmp(m_szFile,"*MAP_AMBIENT",12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;
				// parse the texture block
				ParseLV3MapBlock(mat.sTexAmbient);continue;
			}
			// specular color map
			if (0 == strncmp(m_szFile,"*MAP_SPECULAR",13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				// parse the texture block
				ParseLV3MapBlock(mat.sTexSpecular);continue;
			}
			// opacity map
			if (0 == strncmp(m_szFile,"*MAP_OPACITY",12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;
				// parse the texture block
				ParseLV3MapBlock(mat.sTexOpacity);continue;
			}
			// emissive map
			if (0 == strncmp(m_szFile,"*MAP_SELFILLUM",14) &&
				IsSpaceOrNewLine(*(m_szFile+14)))
			{
				m_szFile+=15;
				// parse the texture block
				ParseLV3MapBlock(mat.sTexEmissive);continue;
			}
			// bump map
			if (0 == strncmp(m_szFile,"*MAP_BUMP",9) &&
				IsSpaceOrNewLine(*(m_szFile+9)))
			{
				m_szFile+=10;
				// parse the texture block
				ParseLV3MapBlock(mat.sTexBump);
			}
			// specular/shininess map
			if (0 == strncmp(m_szFile,"*MAP_SHINESTRENGTH",18) &&
				IsSpaceOrNewLine(*(m_szFile+18)))
			{
				m_szFile+=11;
				// parse the texture block
				ParseLV3MapBlock(mat.sTexShininess);continue;
			}
			// number of submaterials
			if (0 == strncmp(m_szFile,"*NUMSUBMTLS",11) &&
				IsSpaceOrNewLine(*(m_szFile+11)))
			{
				m_szFile+=12;
				ParseLV4MeshLong(iNumSubMaterials);

				// allocate enough storage
				mat.avSubMaterials.resize(iNumSubMaterials);
			}
			// submaterial chunks
			if (0 == strncmp(m_szFile,"*SUBMATERIAL",12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;
				
				unsigned int iIndex = 0;
				ParseLV4MeshLong(iIndex);

				if (iIndex >= iNumSubMaterials)
				{
					LogWarning("Out of range: submaterial index is too large");
					iIndex = iNumSubMaterials-1;
				}

				// get a reference to the material
				Material& sMat = mat.avSubMaterials[iIndex];

				// parse the material block
				ParseLV2MaterialBlock(sMat);
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"2","*MATERIAL");
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MapBlock(Texture& map)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// path to the texture
			if (0 == strncmp(m_szFile,"*BITMAP" ,7) &&
				IsSpaceOrNewLine(*(m_szFile+7)))
			{
				m_szFile+=8;
				if(!ParseString(map.mMapName,"*BITMAP"))SkipToNextToken();
				continue;
			}
			// offset on the u axis
			if (0 == strncmp(m_szFile,"*UVW_U_OFFSET" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				ParseLV4MeshFloat(map.mOffsetU);continue;
			}
			// offset on the v axis
			if (0 == strncmp(m_szFile,"*UVW_V_OFFSET" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				ParseLV4MeshFloat(map.mOffsetV);continue;
			}
			// tiling on the u axis
			if (0 == strncmp(m_szFile,"*UVW_U_TILING" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				ParseLV4MeshFloat(map.mScaleU);continue;
			}
			// tiling on the v axis
			if (0 == strncmp(m_szFile,"*UVW_V_TILING" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				ParseLV4MeshFloat(map.mScaleV);continue;
			}
			// rotation around the z-axis
			if (0 == strncmp(m_szFile,"*UVW_ANGLE" ,10) &&
				IsSpaceOrNewLine(*(m_szFile+10)))
			{
				m_szFile+=11;
				ParseLV4MeshFloat(map.mRotation);continue;
			}
			// map blending factor
			if (0 == strncmp(m_szFile,"*MAP_AMOUNT" ,11) &&
				IsSpaceOrNewLine(*(m_szFile+11)))
			{
				m_szFile+=12;
				ParseLV4MeshFloat(map.mTextureBlend);continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MAP_XXXXXX");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
bool Parser::ParseString(std::string& out,const char* szName)
{
	char szBuffer[1024];


	// NOTE: The name could also be the texture in some cases
	// be prepared that this might occur ...
	if (!SkipSpaces(&m_szFile))
	{
		sprintf(szBuffer,"Unable to parse %s block: Unexpected EOL",szName);
		LogWarning(szBuffer);
		return false;
	}
	// there must be "
	if ('\"' != *m_szFile)
	{
		sprintf(szBuffer,"Unable to parse %s block: String is expected "
			"to be enclosed in double quotation marks",szName);
		LogWarning(szBuffer);
		return false;
	}
	++m_szFile;
	const char* sz = m_szFile;
	while (true)
	{
		if ('\"' == *sz)break;
		else if ('\0' == sz)
		{			
			sprintf(szBuffer,"Unable to parse %s block: String is expected to be "
				"enclosed in double quotation marks but EOF was reached before a closing "
				"quotation mark was found",szName);
			LogWarning(szBuffer);
			return false;
		}
		sz++;
	}
	out = std::string(m_szFile,(uintptr_t)sz-(uintptr_t)m_szFile);
	m_szFile = sz;
	return true;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV1GeometryObjectBlock(ASE::Mesh& mesh)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// name of the mesh/node
			if (0 == strncmp(m_szFile,"*NODE_NAME" ,10) &&
				IsSpaceOrNewLine(*(m_szFile+10)))
			{
				m_szFile+=11;
				if(!ParseString(mesh.mName,"*NODE_NAME"))SkipToNextToken();
				continue;
			}
			// name of the parent of the node
			if (0 == strncmp(m_szFile,"*NODE_PARENT" ,12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;
				if(!ParseString(mesh.mParent,"*NODE_PARENT"))SkipToNextToken();
				continue;
			}
			// transformation matrix of the node
			if (0 == strncmp(m_szFile,"*NODE_TM" ,8) &&
				IsSpaceOrNewLine(*(m_szFile+8)))
			{
				m_szFile+=9;
				ParseLV2NodeTransformBlock(mesh);continue;
				//mesh.mTransform.Transpose();
			}
			// mesh data
			if (0 == strncmp(m_szFile,"*MESH" ,5) &&
				IsSpaceOrNewLine(*(m_szFile+5)))
			{
				m_szFile+=6;
				ParseLV2MeshBlock(mesh);continue;
			}
			// mesh material index
			if (0 == strncmp(m_szFile,"*MATERIAL_REF" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				ParseLV4MeshLong(mesh.iMaterialIndex);continue;
			}
			// animation data of the node
			if (0 == strncmp(m_szFile,"*TM_ANIMATION" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				ParseLV2AnimationBlock(mesh);continue;
			}
		}
		AI_ASE_HANDLE_TOP_LEVEL_SECTION(iDepth);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV2AnimationBlock(ASE::Mesh& mesh)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// position keyframes
			if (0 == strncmp(m_szFile,"*CONTROL_POS_TRACK" ,18) &&
				IsSpaceOrNewLine(*(m_szFile+18)))
			{
				m_szFile+=19;
				ParseLV3PosAnimationBlock(mesh);continue;
			}
			// rotation keyframes
			if (0 == strncmp(m_szFile,"*CONTROL_ROT_TRACK" ,18) &&
				IsSpaceOrNewLine(*(m_szFile+18)))
			{
				m_szFile+=19;
				ParseLV3RotAnimationBlock(mesh);continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"2","TM_ANIMATION");
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3PosAnimationBlock(ASE::Mesh& mesh)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// position keyframe
			if (0 == strncmp(m_szFile,"*CONTROL_POS_SAMPLE" ,19) &&
				IsSpaceOrNewLine(*(m_szFile+19)))
			{
				m_szFile+=20;
				
				unsigned int iIndex;
				mesh.mAnim.akeyPositions.push_back(aiVectorKey());
				aiVectorKey& key = mesh.mAnim.akeyPositions.back();

				ParseLV4MeshFloatTriple(&key.mValue.x,iIndex);
				key.mTime = (double)iIndex;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*CONTROL_POS_TRACK");
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3RotAnimationBlock(ASE::Mesh& mesh)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// rotation keyframe
			if (0 == strncmp(m_szFile,"*CONTROL_ROT_SAMPLE" ,19) &&
				IsSpaceOrNewLine(*(m_szFile+19)))
			{
				m_szFile+=20;

				unsigned int iIndex;
				mesh.mAnim.akeyRotations.push_back(aiQuatKey());
				aiQuatKey& key = mesh.mAnim.akeyRotations.back();

				// first read the axis, then the angle in radians
				aiVector3D v;float f;
				ParseLV4MeshFloatTriple(&v.x,iIndex);
				ParseLV4MeshFloat(f);
				key.mTime = (double)iIndex;
				key.mValue = aiQuaternion(v,f);
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*CONTROL_ROT_TRACK");
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV2NodeTransformBlock(ASE::Mesh& mesh)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// first row of the transformation matrix
			if (0 == strncmp(m_szFile,"*TM_ROW0" ,8) &&
				IsSpaceOrNewLine(*(m_szFile+8)))
			{
				m_szFile+=9;
				ParseLV4MeshFloatTriple(mesh.mTransform[0]);continue;
			}
			// second row of the transformation matrix
			if (0 == strncmp(m_szFile,"*TM_ROW1" ,8) &&
				IsSpaceOrNewLine(*(m_szFile+8)))
			{
				m_szFile+=9;
				ParseLV4MeshFloatTriple(mesh.mTransform[1]);continue;
			}
			// third row of the transformation matrix
			if (0 == strncmp(m_szFile,"*TM_ROW2" ,8) &&
				IsSpaceOrNewLine(*(m_szFile+8)))
			{
				m_szFile+=9;
				ParseLV4MeshFloatTriple(mesh.mTransform[2]);continue;
			}
			// fourth row of the transformation matrix
			if (0 == strncmp(m_szFile,"*TM_ROW3" ,8) &&
				IsSpaceOrNewLine(*(m_szFile+8)))
			{
				m_szFile+=9;
				ParseLV4MeshFloatTriple(mesh.mTransform[3]);continue;
			}
			// inherited position axes
			if (0 == strncmp(m_szFile,"*INHERIT_POS" ,12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;
				unsigned int aiVal[3];
				ParseLV4MeshLongTriple(aiVal);
				
				for (unsigned int i = 0; i < 3;++i)
					mesh.inherit.abInheritPosition[i] = aiVal[i] != 0;
				continue;
			}
			// inherited rotation axes
			if (0 == strncmp(m_szFile,"*INHERIT_ROT" ,12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;
				unsigned int aiVal[3];
				ParseLV4MeshLongTriple(aiVal);
				
				for (unsigned int i = 0; i < 3;++i)
					mesh.inherit.abInheritRotation[i] = aiVal[i] != 0;
				continue;
			}
			// inherited scaling axes
			if (0 == strncmp(m_szFile,"*INHERIT_SCL" ,12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;
				unsigned int aiVal[3];
				ParseLV4MeshLongTriple(aiVal);
				
				for (unsigned int i = 0; i < 3;++i)
					mesh.inherit.abInheritScaling[i] = aiVal[i] != 0;
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"2","*NODE_TM");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV2MeshBlock(ASE::Mesh& mesh)
{
	unsigned int iNumVertices = 0;
	unsigned int iNumFaces = 0;
	unsigned int iNumTVertices = 0;
	unsigned int iNumTFaces = 0;
	unsigned int iNumCVertices = 0;
	unsigned int iNumCFaces = 0;
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Number of vertices in the mesh
			if (0 == strncmp(m_szFile,"*MESH_NUMVERTEX" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV4MeshLong(iNumVertices);continue;
			}
			// Number of texture coordinates in the mesh
			if (0 == strncmp(m_szFile,"*MESH_NUMTVERTEX" ,16) &&
				IsSpaceOrNewLine(*(m_szFile+16)))
			{
				m_szFile+=17;
				ParseLV4MeshLong(iNumTVertices);continue;
			}
			// Number of vertex colors in the mesh
			if (0 == strncmp(m_szFile,"*MESH_NUMCVERTEX" ,16) &&
				IsSpaceOrNewLine(*(m_szFile+16)))
			{
				m_szFile+=17;
				ParseLV4MeshLong(iNumCVertices);continue;
			}
			// Number of regular faces in the mesh
			if (0 == strncmp(m_szFile,"*MESH_NUMFACES" ,14) &&
				IsSpaceOrNewLine(*(m_szFile+14)))
			{
				m_szFile+=15;
				ParseLV4MeshLong(iNumFaces);continue;
				// fix ...
				//mesh.mFaces.resize(iNumFaces);
			}
			// Number of UVWed faces in the mesh
			if (0 == strncmp(m_szFile,"*MESH_NUMTVFACES" ,16) &&
				IsSpaceOrNewLine(*(m_szFile+16)))
			{
				m_szFile+=17;
				ParseLV4MeshLong(iNumTFaces);continue;
			}
			// Number of colored faces in the mesh
			if (0 == strncmp(m_szFile,"*MESH_NUMCVFACES" ,16) &&
				IsSpaceOrNewLine(*(m_szFile+16)))
			{
				m_szFile+=17;
				ParseLV4MeshLong(iNumCFaces);continue;
			}
			// mesh vertex list block
			if (0 == strncmp(m_szFile,"*MESH_VERTEX_LIST" ,17) &&
				IsSpaceOrNewLine(*(m_szFile+17)))
			{
				m_szFile+=18;
				ParseLV3MeshVertexListBlock(iNumVertices,mesh);
				continue;
			}
			// mesh face list block
			if (0 == strncmp(m_szFile,"*MESH_FACE_LIST" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV3MeshFaceListBlock(iNumFaces,mesh);continue;
			}
			// mesh texture vertex list block
			if (0 == strncmp(m_szFile,"*MESH_TVERTLIST" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV3MeshTListBlock(iNumTVertices,mesh);continue;
			}
			// mesh texture face block
			if (0 == strncmp(m_szFile,"*MESH_TFACELIST" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV3MeshTFaceListBlock(iNumTFaces,mesh);continue;
			}
			// mesh color vertex list block
			if (0 == strncmp(m_szFile,"*MESH_CVERTLIST" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV3MeshCListBlock(iNumCVertices,mesh);continue;
			}
			// mesh color face block
			if (0 == strncmp(m_szFile,"*MESH_CFACELIST" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV3MeshCFaceListBlock(iNumCFaces,mesh);continue;
			}
			// mesh normals
			if (0 == strncmp(m_szFile,"*MESH_NORMALS" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				ParseLV3MeshNormalListBlock(mesh);continue;
			}
			// another mesh UV channel ...
			if (0 == strncmp(m_szFile,"*MESH_MAPPINGCHANNEL" ,20) &&
				IsSpaceOrNewLine(*(m_szFile+20)))
			{
				m_szFile+=21;

				unsigned int iIndex = 0;
				ParseLV4MeshLong(iIndex);

				if (iIndex < 2)
				{
					LogWarning("Mapping channel has an invalid index. Skipping UV channel");
					// skip it ...
					SkipSection();
				}
				if (iIndex > AI_MAX_NUMBER_OF_TEXTURECOORDS)
				{
					LogWarning("Too many UV channels specified. Skipping channel ..");
					// skip it ...
					SkipSection();
				}
				else
				{
					// parse the mapping channel
					ParseLV3MappingChannel(iIndex-1,mesh);
				}
				continue;
			}
			// mesh animation keyframe. Not supported
			if (0 == strncmp(m_szFile,"*MESH_ANIMATION" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				
				LogWarning("Found *MESH_ANIMATION element in ASE/ASK file. "
					"Keyframe animation is not supported by Assimp, this element "
					"will be ignored");
				//SkipSection();
				continue;
			}
			// mesh animation keyframe. Not supported
			if (0 == strncmp(m_szFile,"*MESH_WEIGHTS" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				ParseLV3MeshWeightsBlock(mesh);continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"2","*MESH");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshWeightsBlock(ASE::Mesh& mesh)
{
	unsigned int iNumVertices = 0;
	unsigned int iNumBones = 0;
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Number of bone vertices ...
			if (0 == strncmp(m_szFile,"*MESH_NUMVERTEX" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV4MeshLong(iNumVertices);continue;
			}
			// Number of bones
			if (0 == strncmp(m_szFile,"*MESH_NUMBONE" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;
				ParseLV4MeshLong(iNumBones);continue;
			}
			// parse the list of bones
			if (0 == strncmp(m_szFile,"*MESH_BONE_LIST" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV4MeshBones(iNumBones,mesh);continue;
			}
			// parse the list of bones vertices
			if (0 == strncmp(m_szFile,"*MESH_BONE_VERTEX_LIST" ,22) &&
				IsSpaceOrNewLine(*(m_szFile+22)))
			{
				m_szFile+=23;
				ParseLV4MeshBonesVertices(iNumVertices,mesh);
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_WEIGHTS");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshBones(unsigned int iNumBones,ASE::Mesh& mesh)
{
	mesh.mBones.resize(iNumBones);
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Mesh bone with name ...
			if (0 == strncmp(m_szFile,"*MESH_BONE_NAME" ,17) &&
				IsSpaceOrNewLine(*(m_szFile+17)))
			{
				m_szFile+=18;

				// parse an index ...
				if(SkipSpaces(&m_szFile))
				{
					unsigned int iIndex = strtol10(m_szFile,&m_szFile);
					if (iIndex >= iNumBones)
					{
						iIndex = iNumBones-1;
						LogWarning("Bone index is out of bounds. Using the largest valid "
							"bone index instead");
					}
					if (!ParseString(mesh.mBones[iIndex].mName,"*MESH_BONE_NAME"))						
						SkipToNextToken();
					continue;
				}
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_BONE_LIST");
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshBonesVertices(unsigned int iNumVertices,ASE::Mesh& mesh)
{
	mesh.mBoneVertices.resize(iNumVertices);
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Mesh bone vertex
			if (0 == strncmp(m_szFile,"*MESH_BONE_VERTEX" ,17) &&
				IsSpaceOrNewLine(*(m_szFile+17)))
			{
				m_szFile+=18;

				// read the vertex index
				unsigned int iIndex = strtol10(m_szFile,&m_szFile);
				if (iIndex >= mesh.mPositions.size())
				{
					iIndex = (unsigned int)mesh.mPositions.size()-1;
					LogWarning("Bone vertex index is out of bounds. Using the largest valid "
						"bone vertex index instead");
				}

				// now there there are 3 normal floats, the
				// should be identical to the vertex positions
				// contained in the *VERTEX_LIST block. Well, we check this
				// in debug builds to be sure ;-)
				float afVert[3];
				ParseLV4MeshFloatTriple(afVert);

				std::pair<int,float> pairOut;
				while (true)
				{
					// first parse the bone index ...
					if (!SkipSpaces(&m_szFile))break;
					pairOut.first = strtol10(m_szFile,&m_szFile);

					// then parse the vertex weight
					if (!SkipSpaces(&m_szFile))break;
					m_szFile = fast_atof_move(m_szFile,pairOut.second);

					// -1 designates unused entries
					if (-1 != pairOut.first)
					{
						mesh.mBoneVertices[iIndex].mBoneWeights.push_back(pairOut);
					}
				}
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"4","*MESH_BONE_VERTEX");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshVertexListBlock(
	unsigned int iNumVertices, ASE::Mesh& mesh)
{
	// allocate enough storage in the array
	mesh.mPositions.resize(iNumVertices);
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Vertex entry
			if (0 == strncmp(m_szFile,"*MESH_VERTEX" ,12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;

				aiVector3D vTemp;
				unsigned int iIndex;
				ParseLV4MeshFloatTriple(&vTemp.x,iIndex);

				if (iIndex >= iNumVertices)
				{
					LogWarning("Vertex has an invalid index. It will be ignored");
				}
				else mesh.mPositions[iIndex] = vTemp;
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_VERTEX_LIST");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshFaceListBlock(unsigned int iNumFaces, ASE::Mesh& mesh)
{
	// allocate enough storage in the face array
	mesh.mFaces.resize(iNumFaces);
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Face entry
			if (0 == strncmp(m_szFile,"*MESH_FACE" ,10) &&
				IsSpaceOrNewLine(*(m_szFile+10)))
			{
				m_szFile+=11;

				ASE::Face mFace;
				ParseLV4MeshFace(mFace);

				if (mFace.iFace >= iNumFaces)
				{
					LogWarning("Face has an invalid index. It will be ignored");
				}
				else mesh.mFaces[mFace.iFace] = mFace;
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_FACE_LIST");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshTListBlock(unsigned int iNumVertices,
	ASE::Mesh& mesh, unsigned int iChannel)
{
	// allocate enough storage in the array
	mesh.amTexCoords[iChannel].resize(iNumVertices);
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Vertex entry
			if (0 == strncmp(m_szFile,"*MESH_TVERT" ,11) &&
				IsSpaceOrNewLine(*(m_szFile+11)))
			{
				m_szFile+=12;

				aiVector3D vTemp;
				unsigned int iIndex;
				ParseLV4MeshFloatTriple(&vTemp.x,iIndex);

				if (iIndex >= iNumVertices)
				{
					LogWarning("Tvertex has an invalid index. It will be ignored");
				}
				else mesh.amTexCoords[iChannel][iIndex] = vTemp;

				if (0.0f != vTemp.z)
				{
					// we need 3 coordinate channels
					mesh.mNumUVComponents[iChannel] = 3;
				}
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_TVERT_LIST");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshTFaceListBlock(unsigned int iNumFaces,
	ASE::Mesh& mesh, unsigned int iChannel)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Face entry
			if (0 == strncmp(m_szFile,"*MESH_TFACE" ,12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;

				unsigned int aiValues[3];
				unsigned int iIndex = 0;

				ParseLV4MeshLongTriple(aiValues,iIndex);
				if (iIndex >= iNumFaces || iIndex >= mesh.mFaces.size())
				{
					LogWarning("UV-Face has an invalid index. It will be ignored");
				}
				else
				{
					// copy UV indices
					mesh.mFaces[iIndex].amUVIndices[iChannel][0] = aiValues[0];
					mesh.mFaces[iIndex].amUVIndices[iChannel][1] = aiValues[1];
					mesh.mFaces[iIndex].amUVIndices[iChannel][2] = aiValues[2];
				}
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_TFACE_LIST");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MappingChannel(unsigned int iChannel, ASE::Mesh& mesh)
{
	unsigned int iNumTVertices = 0;
	unsigned int iNumTFaces = 0;

	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Number of texture coordinates in the mesh
			if (0 == strncmp(m_szFile,"*MESH_NUMTVERTEX" ,16) &&
				IsSpaceOrNewLine(*(m_szFile+16)))
			{
				m_szFile+=17;
				ParseLV4MeshLong(iNumTVertices);continue;
			}
			// Number of UVWed faces in the mesh
			if (0 == strncmp(m_szFile,"*MESH_NUMTVFACES" ,16) &&
				IsSpaceOrNewLine(*(m_szFile+16)))
			{
				m_szFile+=17;
				ParseLV4MeshLong(iNumTFaces);continue;
			}
			// mesh texture vertex list block
			if (0 == strncmp(m_szFile,"*MESH_TVERTLIST" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV3MeshTListBlock(iNumTVertices,mesh,iChannel);
				continue;
			}
			// mesh texture face block
			if (0 == strncmp(m_szFile,"*MESH_TFACELIST" ,15) &&
				IsSpaceOrNewLine(*(m_szFile+15)))
			{
				m_szFile+=16;
				ParseLV3MeshTFaceListBlock(iNumTFaces,mesh, iChannel);
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_MAPPING_CHANNEL");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshCListBlock(unsigned int iNumVertices, ASE::Mesh& mesh)
{
	// allocate enough storage in the array
	mesh.mVertexColors.resize(iNumVertices);
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Vertex entry
			if (0 == strncmp(m_szFile,"*MESH_VERTCOL" ,13) &&
				IsSpaceOrNewLine(*(m_szFile+13)))
			{
				m_szFile+=14;

				aiColor4D vTemp;
				vTemp.a = 1.0f;
				unsigned int iIndex;
				ParseLV4MeshFloatTriple(&vTemp.r,iIndex);

				if (iIndex >= iNumVertices)
				{
					LogWarning("Vertex color has an invalid index. It will be ignored");
				}
				else mesh.mVertexColors[iIndex] = vTemp;
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_CVERTEX_LIST");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshCFaceListBlock(unsigned int iNumFaces, ASE::Mesh& mesh)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			// Face entry
			if (0 == strncmp(m_szFile,"*MESH_CFACE" ,12) &&
				IsSpaceOrNewLine(*(m_szFile+12)))
			{
				m_szFile+=13;

				unsigned int aiValues[3];
				unsigned int iIndex = 0;

				ParseLV4MeshLongTriple(aiValues,iIndex);
				if (iIndex >= iNumFaces || iIndex >= mesh.mFaces.size())
				{
					LogWarning("UV-Face has an invalid index. It will be ignored");
				}
				else
				{
					// copy color indices
					mesh.mFaces[iIndex].mColorIndices[0] = aiValues[0];
					mesh.mFaces[iIndex].mColorIndices[1] = aiValues[1];
					mesh.mFaces[iIndex].mColorIndices[2] = aiValues[2];
				}
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_CFACE_LIST");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshNormalListBlock(ASE::Mesh& sMesh)
{
	// allocate enough storage for the normals
	sMesh.mNormals.resize(sMesh.mPositions.size(),aiVector3D( 0.f, 0.f, 0.f ));
	int iDepth = 0;
	unsigned int iIndex = 0;

	// just smooth both vertex and face normals together, so it will still
	// work if one oneof the two is missing ...
	// TODO: find out why many models have invalid normals ...
	while (true)
	{
		if ('*' == *m_szFile)
		{
			if (0 == strncmp(m_szFile,"*MESH_VERTEXNORMAL",18) && 
				IsSpaceOrNewLine(*(m_szFile+18)))
			{
				m_szFile += 19;
				aiVector3D vNormal;
				ParseLV4MeshFloatTriple(&vNormal.x,iIndex);

				if (iIndex >= sMesh.mNormals.size())
				{
					LogWarning("Normal index is too large");
					continue;
				}
				sMesh.mNormals[iIndex] += vNormal;
				continue;
			}
			if (0 == strncmp(m_szFile,"*MESH_FACENORMAL",16) && 
				IsSpaceOrNewLine(*(m_szFile+16)))
			{
				m_szFile += 17;
				aiVector3D vNormal;
				ParseLV4MeshFloatTriple(&vNormal.x,iIndex);

				if (iIndex >= sMesh.mFaces.size())
				{
					LogWarning("Face normal index is too large");
					continue;
				}
				for (unsigned int i = 0; i<3; ++i)
				{
					sMesh.mNormals[sMesh.mFaces[iIndex].mIndices[i]] += vNormal;
				}
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*MESH_NORMALS");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshFace(ASE::Face& out)
{	
	// skip spaces and tabs
	if(!SkipSpaces(&m_szFile))
	{
		LogWarning("Unable to parse *MESH_FACE Element: Unexpected EOL [#1]");
		SkipToNextToken();
		return;
	}

	// parse the face index
	out.iFace = strtol10(m_szFile,&m_szFile);

	// next character should be ':'
	if(!SkipSpaces(&m_szFile))
	{
		// FIX: there are some ASE files which haven't got : here ....
		LogWarning("Unable to parse *MESH_FACE Element: Unexpected EOL. \':\' expected [#2]");
		SkipToNextToken();
		return;
	}
	// FIX: there are some ASE files which haven't got : here ....
	if(':' == *m_szFile)++m_szFile;

	// parse all mesh indices
	for (unsigned int i = 0; i < 3;++i)
	{
		unsigned int iIndex = 0;
		if(!SkipSpaces(&m_szFile))
		{
			// LOG 
__EARTHQUAKE_XXL:
			LogWarning("Unable to parse *MESH_FACE Element: Unexpected EOL. "
				"A,B or C expected [#3]");
			SkipToNextToken();
			return;
		}
		switch (*m_szFile)
		{
		case 'A':
		case 'a':
			break;
		case 'B':
		case 'b':
			iIndex = 1;
			break;
		case 'C':
		case 'c':
			iIndex = 2;
			break;
		default: goto __EARTHQUAKE_XXL;
		};
		++m_szFile;

		// next character should be ':'
		if(!SkipSpaces(&m_szFile) || ':' != *m_szFile)
		{
			LogWarning("Unable to parse *MESH_FACE Element: "
				"Unexpected EOL. \':\' expected [#2]");
			SkipToNextToken();
			return;
		}

		++m_szFile;
		if(!SkipSpaces(&m_szFile))
		{
			LogWarning("Unable to parse *MESH_FACE Element: Unexpected EOL. "
				"Vertex index ecpected [#4]");
			SkipToNextToken();
			return;
		}
		out.mIndices[iIndex] = strtol10(m_szFile,&m_szFile);
	}

	// now we need to skip the AB, BC, CA blocks. 
	while (true)
	{
		if ('*' == *m_szFile)break;
		if (IsLineEnd(*m_szFile))
		{
			//iLineNumber++;
			return;
		}
		m_szFile++;
	}

	// parse the smoothing group of the face
	if (0 == strncmp(m_szFile,"*MESH_SMOOTHING",15) && 
		IsSpaceOrNewLine(*(m_szFile+15)))
	{
		m_szFile+=16;
		if(!SkipSpaces(&m_szFile))
		{
			LogWarning("Unable to parse *MESH_SMOOTHING Element: "
				"Unexpected EOL. Smoothing group(s) expected [#5]");
			SkipToNextToken();
			return;
		}
		
		// parse smoothing groups until we don_t anymore see commas
		// FIX: There needn't always be a value, sad but true
		while (true)
		{
			if (*m_szFile < '9' && *m_szFile >= '0')
			{
				out.iSmoothGroup |= (1 << strtol10(m_szFile,&m_szFile));
			}
			SkipSpaces(&m_szFile);
			if (',' != *m_szFile)
			{
				break;
			}
			++m_szFile;
			SkipSpaces(&m_szFile);
		}
	}

	// *MESH_MTLID  is optional, too
	while (true)
	{
		if ('*' == *m_szFile)break;
		if (IsLineEnd(*m_szFile))
		{
			//iLineNumber++;
			return;
		}
		m_szFile++;
	}

	if (0 == strncmp(m_szFile,"*MESH_MTLID",11) && IsSpaceOrNewLine(*(m_szFile+11)))
	{
		m_szFile+=12;
		if(!SkipSpaces(&m_szFile))
		{
			LogWarning("Unable to parse *MESH_MTLID Element: Unexpected EOL. "
				"Material index expected [#6]");
			SkipToNextToken();
			return;
		}
		out.iMaterial = strtol10(m_szFile,&m_szFile);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshLongTriple(unsigned int* apOut)
{
	ai_assert(NULL != apOut);

	for (unsigned int i = 0; i < 3;++i)
		ParseLV4MeshLong(apOut[i]);
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshLongTriple(unsigned int* apOut, unsigned int& rIndexOut)
{
	ai_assert(NULL != apOut);

	// parse the index
	ParseLV4MeshLong(rIndexOut);

	// parse the three others
	ParseLV4MeshLongTriple(apOut);
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshFloatTriple(float* apOut, unsigned int& rIndexOut)
{
	ai_assert(NULL != apOut);

	// parse the index
	ParseLV4MeshLong(rIndexOut);
	
	// parse the three others
	ParseLV4MeshFloatTriple(apOut);
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshFloatTriple(float* apOut)
{
	ai_assert(NULL != apOut);

	for (unsigned int i = 0; i < 3;++i)
		ParseLV4MeshFloat(apOut[i]);
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshFloat(float& fOut)
{
	// skip spaces and tabs
	if(!SkipSpaces(&m_szFile))
	{
		// LOG 
		LogWarning("Unable to parse float: unexpected EOL [#1]");
		fOut = 0.0f;
		++iLineNumber;
		return;
	}
	// parse the first float
	m_szFile = fast_atof_move(m_szFile,fOut);
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshLong(unsigned int& iOut)
{
	// skip spaces and tabs
	if(!SkipSpaces(&m_szFile))
	{
		// LOG 
		LogWarning("Unable to parse long: unexpected EOL [#1]");
		iOut = 0;
		++iLineNumber;
		return;
	}
	// parse the value
	iOut = strtol10(m_szFile,&m_szFile);
}
