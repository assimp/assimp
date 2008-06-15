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

#include "TextureTransform.h"
#include "ASELoader.h"
#include "MaterialSystem.h"

#include "../include/DefaultLogger.h"
#include "fast_atof.h"

#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"

#include <boost/scoped_ptr.hpp>

using namespace Assimp;
using namespace Assimp::ASE;

#if (defined BLUBB)
#	undef BLUBB
#endif
#define BLUBB(_message_) \
	{this->LogError(_message_);return;}

// ------------------------------------------------------------------------------------------------
Parser::Parser (const char* szFile)
{
	ai_assert(NULL != szFile);
	this->m_szFile = szFile;

	// makre sure that the color values are invalid
	this->m_clrBackground.r = std::numeric_limits<float>::quiet_NaN();
	this->m_clrAmbient.r = std::numeric_limits<float>::quiet_NaN();

	this->iLineNumber = 0;
}
// ------------------------------------------------------------------------------------------------
void Parser::LogWarning(const char* szWarn)
{
	ai_assert(NULL != szWarn);

	char szTemp[1024];
#if _MSC_VER >= 1400
	sprintf_s(szTemp,"Line %i: %s",this->iLineNumber,szWarn);
#else
	ai_assert(strlen(szWarn) < 950);
	sprintf(szTemp,"Line %i: %s",this->iLineNumber,szWarn);
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
	sprintf_s(szTemp,"Line %i: %s",this->iLineNumber,szWarn);
#else
	ai_assert(strlen(szWarn) < 950);
	sprintf(szTemp,"Line %i: %s",this->iLineNumber,szWarn);
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
	sprintf_s(szTemp,"Line %i: %s",this->iLineNumber,szWarn);
#else
	ai_assert(strlen(szWarn) < 950);
	sprintf(szTemp,"Line %i: %s",this->iLineNumber,szWarn);
#endif

	// throw an exception
	throw new ImportErrorException(szTemp);
}
// ------------------------------------------------------------------------------------------------
bool Parser::SkipToNextToken()
{
	while (true)
	{
		char me = *this->m_szFile;

		// increase the line number counter if necessary
		if (IsLineEnd(me))++this->iLineNumber;
		else if ('*' == me || '}' == me || '{' == me)return true;
		else if ('\0' == me)return false;

		++this->m_szFile;
	}
}
// ------------------------------------------------------------------------------------------------
bool Parser::SkipOpeningBracket()
{
	if (!SkipSpaces(this->m_szFile,&this->m_szFile))return false;
	if ('{' != *this->m_szFile)
	{
		this->LogWarning("Unable to parse block: Unexpected character, \'{\' expected [#1]");
		return false;
	}
	this->SkipToNextToken();
	return true;
}
// ------------------------------------------------------------------------------------------------
bool Parser::SkipSection()
{
	// must handle subsections ...
	int iCnt = 0;
	while (true)
	{
		if ('}' == *this->m_szFile)
		{
			--iCnt;
			if (0 == iCnt)
			{
				// go to the next valid token ...
				++this->m_szFile;
				this->SkipToNextToken();
				return true;
			}
		}
		else if ('{' == *this->m_szFile)
		{
			++iCnt;
		}
		else if ('\0' == *this->m_szFile)
		{
			this->LogWarning("Unable to parse block: Unexpected EOF, closing bracket \'}\' was expected [#1]");	
			return false;
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::Parse()
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			// version should be 200. Validate this ...
			if (0 == strncmp(this->m_szFile,"*3DSMAX_ASCIIEXPORT",19) &&
				IsSpaceOrNewLine(*(this->m_szFile+19)))
			{
				this->m_szFile+=20;

				unsigned int iVersion;
				this->ParseLV4MeshLong(iVersion);

				if (200 != iVersion)
				{
					this->LogWarning("Unknown file format version: *3DSMAX_ASCIIEXPORT should \
						be 200. Continuing happily ...");
				}
				continue;
			}
			// main scene information
			if (0 == strncmp(this->m_szFile,"*SCENE",6) &&
				IsSpaceOrNewLine(*(this->m_szFile+6)))
			{
				this->m_szFile+=7;
				this->ParseLV1SceneBlock();
				continue;
			}
			// material list
			if (0 == strncmp(this->m_szFile,"*MATERIAL_LIST",14) &&
				IsSpaceOrNewLine(*(this->m_szFile+14)))
			{
				this->m_szFile+=15;
				this->ParseLV1MaterialListBlock();
				continue;
			}
			// geometric object (mesh)
			if (0 == strncmp(this->m_szFile,"*GEOMOBJECT",11) &&
				IsSpaceOrNewLine(*(this->m_szFile+11)))
			{
				this->m_szFile+=12;
				this->m_vMeshes.push_back(Mesh());
				this->ParseLV1GeometryObjectBlock(this->m_vMeshes.back());
				continue;
			}
			// ignore comments, lights and cameras
			// (display comments on the console)
			if (0 == strncmp(this->m_szFile,"*LIGHTOBJECT",12) &&
				IsSpaceOrNewLine(*(this->m_szFile+12)))
			{
				this->m_szFile+=13;
				this->LogInfo("Found light source (*LIGHTOBJECT chunk). It will be ignored");
				continue;
			}
			if (0 == strncmp(this->m_szFile,"*CAMERAOBJECT",13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;
				this->LogInfo("Found virtual camera (*CAMERAOBJECT chunk). It will be ignored");
				continue;
			}
			if (0 == strncmp(this->m_szFile,"*COMMENT",8) &&
				IsSpaceOrNewLine(*(this->m_szFile+8)))
			{
				this->m_szFile+=9;
				std::string out = "<unknown>";
				this->ParseString(out,"*COMMENT");
				this->LogInfo(("Comment: " + out).c_str());
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... why not?
			return;
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV1SceneBlock()
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			if (0 == strncmp(this->m_szFile,"*SCENE_BACKGROUND_STATIC",24) &&
				IsSpaceOrNewLine(*(this->m_szFile+24)))
			{
				this->m_szFile+=25;

				// parse a color triple and assume it is really the bg color
				this->ParseLV4MeshFloatTriple( &this->m_clrBackground.r );
				continue;
			}
			if (0 == strncmp(this->m_szFile,"*SCENE_AMBIENT_STATIC",21) &&
				IsSpaceOrNewLine(*(this->m_szFile+21)))
			{
				this->m_szFile+=22;

				// parse a color triple and assume it is really the bg color
				this->ParseLV4MeshFloatTriple( &this->m_clrAmbient.r );
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... why not?
			return;
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV1MaterialListBlock()
{
	int iDepth = 0;
	unsigned int iMaterialCount = 0;
	unsigned int iOldMaterialCount = (unsigned int)this->m_vMaterials.size();
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			if (0 == strncmp(this->m_szFile,"*MATERIAL_COUNT",15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->ParseLV4MeshLong(iMaterialCount);

				// now allocate enough storage to hold all materials
				this->m_vMaterials.resize(iOldMaterialCount+iMaterialCount);
				continue;
			}
			if (0 == strncmp(this->m_szFile,"*MATERIAL",9) &&
				IsSpaceOrNewLine(*(this->m_szFile+9)))
			{
				this->m_szFile+=10;
				unsigned int iIndex = 0;
				this->ParseLV4MeshLong(iIndex);

				if (iIndex >= iMaterialCount)
				{
					this->LogWarning("Out of range: material index is too large");
					iIndex = iMaterialCount-1;
				}

				// get a reference to the material
				Material& sMat = this->m_vMaterials[iIndex+iOldMaterialCount];

				// skip the '{'
				this->SkipOpeningBracket();

				// parse the material block
				this->ParseLV2MaterialBlock(sMat);
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... why not?
			return;
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV2MaterialBlock(ASE::Material& mat)
{
	int iDepth = 0;
	unsigned int iNumSubMaterials = 0;
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			if (0 == strncmp(this->m_szFile,"*MATERIAL_NAME",14) &&
				IsSpaceOrNewLine(*(this->m_szFile+14)))
			{
				this->m_szFile+=15;
			
				if (!this->ParseString(mat.mName,"*MATERIAL_NAME"))this->SkipToNextToken();
				continue;
			}
			// ambient material color
			if (0 == strncmp(this->m_szFile,"*MATERIAL_AMBIENT",17) &&
				IsSpaceOrNewLine(*(this->m_szFile+17)))
			{
				this->m_szFile+=18;
				this->ParseLV4MeshFloatTriple(&mat.mAmbient.r);continue;
			}
			// diffuse material color
			if (0 == strncmp(this->m_szFile,"*MATERIAL_DIFFUSE",17) &&
				IsSpaceOrNewLine(*(this->m_szFile+17)))
			{
				this->m_szFile+=18;
				this->ParseLV4MeshFloatTriple(&mat.mDiffuse.r);continue;
			}
			// specular material color
			if (0 == strncmp(this->m_szFile,"*MATERIAL_SPECULAR",18) &&
				IsSpaceOrNewLine(*(this->m_szFile+18)))
			{
				this->m_szFile+=19;
				this->ParseLV4MeshFloatTriple(&mat.mSpecular.r);continue;
			}
			// material shading type
			if (0 == strncmp(this->m_szFile,"*MATERIAL_SHADING",17) &&
				IsSpaceOrNewLine(*(this->m_szFile+17)))
			{
				this->m_szFile+=18;
				
				if (0 == strncmp(this->m_szFile,"Blinn",5) && 
					IsSpaceOrNewLine(*(this->m_szFile+5)))
				{
					mat.mShading = Dot3DSFile::Blinn;
					this->m_szFile+=6;
				}
				else if (0 == strncmp(this->m_szFile,"Phong",5) && 
					IsSpaceOrNewLine(*(this->m_szFile+5)))
				{
					mat.mShading = Dot3DSFile::Phong;
					this->m_szFile+=6;
				}
				else if (0 == strncmp(this->m_szFile,"Flat",4) && 
					IsSpaceOrNewLine(*(this->m_szFile+4)))
				{
					mat.mShading = Dot3DSFile::Flat;
					this->m_szFile+=5;
				}
				else if (0 == strncmp(this->m_szFile,"Wire",4) && 
					IsSpaceOrNewLine(*(this->m_szFile+4)))
				{
					mat.mShading = Dot3DSFile::Wire;
					this->m_szFile+=5;
				}
				else
				{
					// assume gouraud shading
					mat.mShading = Dot3DSFile::Gouraud;
					this->SkipToNextToken();
				}
				continue;
			}
			// material transparency
			if (0 == strncmp(this->m_szFile,"*MATERIAL_TRANSPARENCY",22) &&
				IsSpaceOrNewLine(*(this->m_szFile+22)))
			{
				this->m_szFile+=23;
				this->ParseLV4MeshFloat(mat.mTransparency);
				mat.mTransparency = 1.0f - mat.mTransparency;continue;
			}
			// material self illumination
			if (0 == strncmp(this->m_szFile,"*MATERIAL_SELFILLUM",19) &&
				IsSpaceOrNewLine(*(this->m_szFile+19)))
			{
				this->m_szFile+=20;
				float f = 0.0f;
				this->ParseLV4MeshFloat(f);

				mat.mEmissive.r = f;
				mat.mEmissive.g = f;
				mat.mEmissive.b = f;
				continue;
			}
			// material shininess
			if (0 == strncmp(this->m_szFile,"*MATERIAL_SHINE",15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->ParseLV4MeshFloat(mat.mSpecularExponent);
				mat.mSpecularExponent *= 15;continue;
			}
			// material shininess strength
			if (0 == strncmp(this->m_szFile,"*MATERIAL_SHINESTRENGTH",23) &&
				IsSpaceOrNewLine(*(this->m_szFile+23)))
			{
				this->m_szFile+=24;
				this->ParseLV4MeshFloat(mat.mShininessStrength);continue;
			}
			// diffuse color map
			if (0 == strncmp(this->m_szFile,"*MAP_DIFFUSE",12) &&
				IsSpaceOrNewLine(*(this->m_szFile+12)))
			{
				this->m_szFile+=13;
				// skip the opening bracket
				this->SkipOpeningBracket();
				// parse the texture block
				this->ParseLV3MapBlock(mat.sTexDiffuse);continue;
			}
			// ambient color map
			if (0 == strncmp(this->m_szFile,"*MAP_AMBIENT",12) &&
				IsSpaceOrNewLine(*(this->m_szFile+12)))
			{
				this->m_szFile+=13;
				// skip the opening bracket
				this->SkipOpeningBracket();
				// parse the texture block
				this->ParseLV3MapBlock(mat.sTexAmbient);continue;
			}
			// specular color map
			if (0 == strncmp(this->m_szFile,"*MAP_SPECULAR",13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;
				// skip the opening bracket
				this->SkipOpeningBracket();
				// parse the texture block
				this->ParseLV3MapBlock(mat.sTexSpecular);continue;
			}
			// opacity map
			if (0 == strncmp(this->m_szFile,"*MAP_OPACITY",12) &&
				IsSpaceOrNewLine(*(this->m_szFile+12)))
			{
				this->m_szFile+=13;
				// skip the opening bracket
				this->SkipOpeningBracket();
				// parse the texture block
				this->ParseLV3MapBlock(mat.sTexOpacity);continue;
			}
			// emissive map
			if (0 == strncmp(this->m_szFile,"*MAP_SELFILLUM",14) &&
				IsSpaceOrNewLine(*(this->m_szFile+14)))
			{
				this->m_szFile+=15;
				// skip the opening bracket
				this->SkipOpeningBracket();
				// parse the texture block
				this->ParseLV3MapBlock(mat.sTexEmissive);continue;
			}
			// bump map
			if (0 == strncmp(this->m_szFile,"*MAP_BUMP",9) &&
				IsSpaceOrNewLine(*(this->m_szFile+9)))
			{
				this->m_szFile+=10;
				// skip the opening bracket
				this->SkipOpeningBracket();
				// parse the texture block
				this->ParseLV3MapBlock(mat.sTexBump);
			}
			// specular/shininess map
			if (0 == strncmp(this->m_szFile,"*MAP_SHINE",10) &&
				IsSpaceOrNewLine(*(this->m_szFile+10)))
			{
				this->m_szFile+=11;
				// skip the opening bracket
				this->SkipOpeningBracket();
				// parse the texture block
				this->ParseLV3MapBlock(mat.sTexShininess);continue;
			}
			// number of submaterials
			if (0 == strncmp(this->m_szFile,"*NUMSUBMTLS",11) &&
				IsSpaceOrNewLine(*(this->m_szFile+11)))
			{
				this->m_szFile+=12;
				this->ParseLV4MeshLong(iNumSubMaterials);

				// allocate enough storage
				mat.avSubMaterials.resize(iNumSubMaterials);
			}
			// submaterial chunks
			if (0 == strncmp(this->m_szFile,"*SUBMATERIAL",12) &&
				IsSpaceOrNewLine(*(this->m_szFile+12)))
			{
				this->m_szFile+=13;
				
				unsigned int iIndex = 0;
				this->ParseLV4MeshLong(iIndex);

				if (iIndex >= iNumSubMaterials)
				{
					this->LogWarning("Out of range: submaterial index is too large");
					iIndex = iNumSubMaterials-1;
				}

				// get a reference to the material
				Material& sMat = mat.avSubMaterials[iIndex];

				// skip the '{'
				this->SkipOpeningBracket();

				// parse the material block
				this->ParseLV2MaterialBlock(sMat);
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level2 block, this can't be
			BLUBB("Unable to finish parsing a lv2 material block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MapBlock(Texture& map)
{
	int iDepth = 0;
	unsigned int iNumSubMaterials = 0;
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			// path to the texture
			if (0 == strncmp(this->m_szFile,"*BITMAP" ,7) &&
				IsSpaceOrNewLine(*(this->m_szFile+7)))
			{
				this->m_szFile+=8;
				if(!this->ParseString(map.mMapName,"*BITMAP"))SkipToNextToken();
				continue;
			}
			// offset on the u axis
			if (0 == strncmp(this->m_szFile,"*UVW_U_OFFSET" ,13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;
				this->ParseLV4MeshFloat(map.mOffsetU);continue;
			}
			// offset on the v axis
			if (0 == strncmp(this->m_szFile,"*UVW_V_OFFSET" ,13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;
				this->ParseLV4MeshFloat(map.mOffsetV);continue;
			}
			// tiling on the u axis
			if (0 == strncmp(this->m_szFile,"*UVW_U_TILING" ,13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;
				this->ParseLV4MeshFloat(map.mScaleU);continue;
			}
			// tiling on the v axis
			if (0 == strncmp(this->m_szFile,"*UVW_V_TILING" ,13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;
				this->ParseLV4MeshFloat(map.mScaleV);continue;
			}
			// rotation around the z-axis
			if (0 == strncmp(this->m_szFile,"*UVW_ANGLE" ,10) &&
				IsSpaceOrNewLine(*(this->m_szFile+10)))
			{
				this->m_szFile+=11;
				this->ParseLV4MeshFloat(map.mRotation);continue;
			}
			// map blending factor
			if (0 == strncmp(this->m_szFile,"*MAP_AMOUNT" ,11) &&
				IsSpaceOrNewLine(*(this->m_szFile+11)))
			{
				this->m_szFile+=12;
				this->ParseLV4MeshFloat(map.mTextureBlend);continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level3 block, this can't be
			BLUBB("Unable to finish parsing a lv3 map block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
bool Parser::ParseString(std::string& out,const char* szName)
{
	char szBuffer[1024];

#if (!defined _MSC_VER) || ( _MSC_VER < 1400)
	ai_assert(strlen(szName) < 750);
#endif

	// NOTE: The name could also be the texture in some cases
	// be prepared that this might occur ...
	if (!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
#if _MSC_VER >= 1400
		sprintf_s(szBuffer,"Unable to parse %s block: Unexpected EOL",szName);
#else
		sprintf(szBuffer,"Unable to parse %s block: Unexpected EOL",szName);
#endif
		this->LogWarning(szBuffer);
		return false;
	}
	// there must be "
	if ('\"' != *this->m_szFile)
	{
#if _MSC_VER >= 1400
		sprintf_s(szBuffer,"Unable to parse %s block: String is expected "
			"to be enclosed in double quotation marks",szName);
#else
		sprintf(szBuffer,"Unable to parse %s block: String is expected "
			"to be enclosed in double quotation marks",szName);
#endif
		this->LogWarning(szBuffer);
		return false;
	}
	++this->m_szFile;
	const char* sz = this->m_szFile;
	while (true)
	{
		if ('\"' == *sz)break;
		else if ('\0' == sz)
		{
#if _MSC_VER >= 1400
			sprintf_s(szBuffer,"Unable to parse %s block: String is expected to be "
				"enclosed in double quotation marks but EOF was reached before a closing "
				"quotation mark was found",szName);
#else
			sprintf(szBuffer,"Unable to parse %s block: String is expected to be "
				"enclosed in double quotation marks but EOF was reached before a closing "
				"quotation mark was found",szName);
#endif
			this->LogWarning(szBuffer);
			return false;
		}
		sz++;
	}
	out = std::string(this->m_szFile,(uintptr_t)sz-(uintptr_t)this->m_szFile);
	this->m_szFile = sz;
	return true;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV1GeometryObjectBlock(ASE::Mesh& mesh)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			// name of the mesh/node
			if (0 == strncmp(this->m_szFile,"*NODE_NAME" ,10) &&
				IsSpaceOrNewLine(*(this->m_szFile+10)))
			{
				this->m_szFile+=11;
				if(!this->ParseString(mesh.mName,"*NODE_NAME"))this->SkipToNextToken();
				continue;
			}
			// name of the parent of the node
			if (0 == strncmp(this->m_szFile,"*NODE_PARENT" ,12) &&
				IsSpaceOrNewLine(*(this->m_szFile+12)))
			{
				this->m_szFile+=13;
				if(!this->ParseString(mesh.mParent,"*NODE_PARENT"))this->SkipToNextToken();
				continue;
			}
			// transformation matrix of the node
			if (0 == strncmp(this->m_szFile,"*NODE_TM" ,8) &&
				IsSpaceOrNewLine(*(this->m_szFile+8)))
			{
				this->m_szFile+=9;
				this->ParseLV2NodeTransformBlock(mesh);continue;
			}
			// mesh data
			if (0 == strncmp(this->m_szFile,"*MESH" ,5) &&
				IsSpaceOrNewLine(*(this->m_szFile+5)))
			{
				this->m_szFile+=6;
				this->ParseLV2MeshBlock(mesh);continue;
			}
			// mesh material index
			if (0 == strncmp(this->m_szFile,"*MATERIAL_REF" ,13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;
				this->ParseLV4MeshLong(mesh.iMaterialIndex);continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level1 block, this can be
			return;
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV2NodeTransformBlock(ASE::Mesh& mesh)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			// first row of the transformation matrix
			if (0 == strncmp(this->m_szFile,"*TM_ROW0" ,8) &&
				IsSpaceOrNewLine(*(this->m_szFile+8)))
			{
				this->m_szFile+=9;
				this->ParseLV4MeshFloatTriple(mesh.mTransform[0]);continue;
			}
			// second row of the transformation matrix
			if (0 == strncmp(this->m_szFile,"*TM_ROW1" ,8) &&
				IsSpaceOrNewLine(*(this->m_szFile+8)))
			{
				this->m_szFile+=9;
				this->ParseLV4MeshFloatTriple(mesh.mTransform[1]);continue;
			}
			// third row of the transformation matrix
			if (0 == strncmp(this->m_szFile,"*TM_ROW2" ,8) &&
				IsSpaceOrNewLine(*(this->m_szFile+8)))
			{
				this->m_szFile+=9;
				this->ParseLV4MeshFloatTriple(mesh.mTransform[2]);continue;
			}
			// fourth row of the transformation matrix
			if (0 == strncmp(this->m_szFile,"*TM_ROW3" ,8) &&
				IsSpaceOrNewLine(*(this->m_szFile+8)))
			{
				this->m_szFile+=9;
				this->ParseLV4MeshFloatTriple(mesh.mTransform[3]);continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level2 block, this can't be
			BLUBB("Unable to finish parsing a lv2 node transform block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
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
		if ('*' == *this->m_szFile)
		{
			// Number of vertices in the mesh
			if (0 == strncmp(this->m_szFile,"*MESH_NUMVERTEX" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->ParseLV4MeshLong(iNumVertices);continue;
			}
			// Number of texture coordinates in the mesh
			if (0 == strncmp(this->m_szFile,"*MESH_NUMTVERTEX" ,16) &&
				IsSpaceOrNewLine(*(this->m_szFile+16)))
			{
				this->m_szFile+=17;
				this->ParseLV4MeshLong(iNumTVertices);continue;
			}
			// Number of vertex colors in the mesh
			if (0 == strncmp(this->m_szFile,"*MESH_NUMCVERTEX" ,16) &&
				IsSpaceOrNewLine(*(this->m_szFile+16)))
			{
				this->m_szFile+=17;
				this->ParseLV4MeshLong(iNumCVertices);continue;
			}
			// Number of regular faces in the mesh
			if (0 == strncmp(this->m_szFile,"*MESH_NUMFACES" ,14) &&
				IsSpaceOrNewLine(*(this->m_szFile+14)))
			{
				this->m_szFile+=15;
				this->ParseLV4MeshLong(iNumFaces);continue;
				// fix ...
				//mesh.mFaces.resize(iNumFaces);
			}
			// Number of UVWed faces in the mesh
			if (0 == strncmp(this->m_szFile,"*MESH_NUMTVFACES" ,16) &&
				IsSpaceOrNewLine(*(this->m_szFile+16)))
			{
				this->m_szFile+=17;
				this->ParseLV4MeshLong(iNumTFaces);continue;
			}
			// Number of colored faces in the mesh
			if (0 == strncmp(this->m_szFile,"*MESH_NUMCVFACES" ,16) &&
				IsSpaceOrNewLine(*(this->m_szFile+16)))
			{
				this->m_szFile+=17;
				this->ParseLV4MeshLong(iNumCFaces);continue;
			}
			// mesh vertex list block
			if (0 == strncmp(this->m_szFile,"*MESH_VERTEX_LIST" ,17) &&
				IsSpaceOrNewLine(*(this->m_szFile+17)))
			{
				this->m_szFile+=18;
				this->ParseLV3MeshVertexListBlock(iNumVertices,mesh);
				continue;
			}
			// mesh face list block
			if (0 == strncmp(this->m_szFile,"*MESH_FACE_LIST" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->SkipOpeningBracket();
				this->ParseLV3MeshFaceListBlock(iNumFaces,mesh);continue;
			}
			// mesh texture vertex list block
			if (0 == strncmp(this->m_szFile,"*MESH_TVERTLIST" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->SkipOpeningBracket();
				this->ParseLV3MeshTListBlock(iNumTVertices,mesh);continue;
			}
			// mesh texture face block
			if (0 == strncmp(this->m_szFile,"*MESH_TFACELIST" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->SkipOpeningBracket();
				this->ParseLV3MeshTFaceListBlock(iNumTFaces,mesh);continue;
			}
			// mesh color vertex list block
			if (0 == strncmp(this->m_szFile,"*MESH_CVERTLIST" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->SkipOpeningBracket();
				this->ParseLV3MeshCListBlock(iNumCVertices,mesh);continue;
			}
			// mesh color face block
			if (0 == strncmp(this->m_szFile,"*MESH_CFACELIST" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->SkipOpeningBracket();
				this->ParseLV3MeshCFaceListBlock(iNumCFaces,mesh);continue;
			}
			// another mesh UV channel ...
			if (0 == strncmp(this->m_szFile,"*MESH_MAPPINGCHANNEL" ,20) &&
				IsSpaceOrNewLine(*(this->m_szFile+20)))
			{
				this->m_szFile+=21;

				unsigned int iIndex = 0;
				this->ParseLV4MeshLong(iIndex);

				if (iIndex < 2)
				{
					this->LogWarning("Mapping channel has an invalid index. Skipping UV channel");
					// skip it ...
					this->SkipOpeningBracket();
					this->SkipSection();
				}
				if (iIndex > AI_MAX_NUMBER_OF_TEXTURECOORDS)
				{
					this->LogWarning("Too many UV channels specified. Skipping channel ..");
					// skip it ...
					this->SkipOpeningBracket();
					this->SkipSection();
				}
				else
				{
					// skip the '{'
					this->SkipOpeningBracket();

					// parse the mapping channel
					this->ParseLV3MappingChannel(iIndex-1,mesh);
				}
				continue;
			}
			// mesh animation keyframe. Not supported
			if (0 == strncmp(this->m_szFile,"*MESH_ANIMATION" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				
				this->LogWarning("Found *MESH_ANIMATION element in ASE/ASK file. "
					"Keyframe animation is not supported by Assimp, this element "
					"will be ignored");
				//this->SkipSection();
				continue;
			}
			// mesh animation keyframe. Not supported
			if (0 == strncmp(this->m_szFile,"*MESH_WEIGHTS" ,13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;
				this->ParseLV3MeshWeightsBlock(mesh);continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level2 block, this can't be
			BLUBB("Unable to finish parsing a lv2 mesh block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
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
		if ('*' == *this->m_szFile)
		{
			// Number of bone vertices ...
			if (0 == strncmp(this->m_szFile,"*MESH_NUMVERTEX" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->ParseLV4MeshLong(iNumVertices);continue;
			}
			// Number of bones
			if (0 == strncmp(this->m_szFile,"*MESH_NUMBONE" ,13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;
				this->ParseLV4MeshLong(iNumBones);continue;
			}
			// parse the list of bones
			if (0 == strncmp(this->m_szFile,"*MESH_BONE_LIST" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->SkipOpeningBracket();
				this->ParseLV4MeshBones(iNumBones,mesh);continue;
			}
			// parse the list of bones vertices
			if (0 == strncmp(this->m_szFile,"*MESH_BONE_VERTEX_LIST" ,22) &&
				IsSpaceOrNewLine(*(this->m_szFile+22)))
			{
				this->m_szFile+=23;
				this->SkipOpeningBracket();
				this->ParseLV4MeshBonesVertices(iNumVertices,mesh);
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level2 block, this can't be
			BLUBB("Unable to finish parsing a lv2 *MESH_WEIGHTS block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
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
		if ('*' == *this->m_szFile)
		{
			// Mesh bone with name ...
			if (0 == strncmp(this->m_szFile,"*MESH_BONE_NAME" ,17) &&
				IsSpaceOrNewLine(*(this->m_szFile+17)))
			{
				this->m_szFile+=18;

				// parse an index ...
				if(SkipSpaces(this->m_szFile,&this->m_szFile))
				{
					unsigned int iIndex = strtol10(this->m_szFile,&this->m_szFile);
					if (iIndex >= iNumBones)
					{
						iIndex = iNumBones-1;
						this->LogWarning("Bone index is out of bounds. Using the largest valid "
							"bone index instead");
					}
					if (!this->ParseString(mesh.mBones[iIndex].mName,"*MESH_BONE_NAME"))						
						this->SkipToNextToken();
					continue;
				}
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level4 block, this can't be
			BLUBB("Unable to finish parsing a lv4 *MESH_BONE_LIST block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshBonesVertices(unsigned int iNumVertices,ASE::Mesh& mesh)
{
	mesh.mBoneVertices.resize(iNumVertices);
	int iDepth = 0;
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			// Mesh bone vertex
			if (0 == strncmp(this->m_szFile,"*MESH_BONE_VERTEX" ,17) &&
				IsSpaceOrNewLine(*(this->m_szFile+17)))
			{
				this->m_szFile+=18;

				// read the vertex index
				unsigned int iIndex = strtol10(this->m_szFile,&this->m_szFile);
				if (iIndex >= mesh.mPositions.size())
				{
					iIndex = (unsigned int)mesh.mPositions.size()-1;
					this->LogWarning("Bone vertex index is out of bounds. Using the largest valid "
						"bone vertex index instead");
				}

				// now there there are 3 normal floats, the
				// should be identical to the vertex positions
				// contained in the *VERTEX_LIST block. Well, we check this
				// in debug builds to be sure ;-)
				float afVert[3];
				this->ParseLV4MeshFloatTriple(afVert);

				std::pair<int,float> pairOut;
				while (true)
				{
					// first parse the bone index ...
					if (!SkipSpaces(this->m_szFile,&this->m_szFile))break;
					pairOut.first = strtol10(this->m_szFile,&this->m_szFile);

					// then parse the vertex weight
					if (!SkipSpaces(this->m_szFile,&this->m_szFile))break;
					this->m_szFile = fast_atof_move(this->m_szFile,pairOut.second);

					// -1 designates unused entries
					if (-1 != pairOut.first)
					{
						mesh.mBoneVertices[iIndex].mBoneWeights.push_back(pairOut);
					}
				}
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level4 block, this can't be
			BLUBB("Unable to finish parsing a lv4 *MESH_BONE_VERTEX_LIST block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
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
		if ('*' == *this->m_szFile)
		{
			// Vertex entry
			if (0 == strncmp(this->m_szFile,"*MESH_VERTEX" ,12) &&
				IsSpaceOrNewLine(*(this->m_szFile+12)))
			{
				this->m_szFile+=13;

				aiVector3D vTemp;
				unsigned int iIndex;
				this->ParseLV4MeshFloatTriple(&vTemp.x,iIndex);

				if (iIndex >= iNumVertices)
				{
					this->LogWarning("Vertex has an invalid index. It will be ignored");
				}
				else mesh.mPositions[iIndex] = vTemp;
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level3 block, this can't be
			BLUBB("Unable to finish parsing a lv3 vertex list block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
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
		if ('*' == *this->m_szFile)
		{
			// Face entry
			if (0 == strncmp(this->m_szFile,"*MESH_FACE" ,10) &&
				IsSpaceOrNewLine(*(this->m_szFile+10)))
			{
				this->m_szFile+=11;

				ASE::Face mFace;
				this->ParseLV4MeshFace(mFace);

				if (mFace.iFace >= iNumFaces)
				{
					this->LogWarning("Face has an invalid index. It will be ignored");
				}
				else mesh.mFaces[mFace.iFace] = mFace;
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level3 block, this can't be
			BLUBB("Unable to finish parsing LV3 *MESH_FACE_LIST block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
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
		if ('*' == *this->m_szFile)
		{
			// Vertex entry
			if (0 == strncmp(this->m_szFile,"*MESH_TVERT" ,11) &&
				IsSpaceOrNewLine(*(this->m_szFile+11)))
			{
				this->m_szFile+=12;

				aiVector3D vTemp;
				unsigned int iIndex;
				this->ParseLV4MeshFloatTriple(&vTemp.x,iIndex);

				if (iIndex >= iNumVertices)
				{
					this->LogWarning("Tvertex has an invalid index. It will be ignored");
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
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level3 block, this can't be
			BLUBB("Unable to finish parsing LV3 *MESH_VERTEX_LIST block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
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
		if ('*' == *this->m_szFile)
		{
			// Face entry
			if (0 == strncmp(this->m_szFile,"*MESH_TFACE" ,12) &&
				IsSpaceOrNewLine(*(this->m_szFile+12)))
			{
				this->m_szFile+=13;

				unsigned int aiValues[3];
				unsigned int iIndex = 0;

				this->ParseLV4MeshLongTriple(aiValues,iIndex);
				if (iIndex >= iNumFaces || iIndex >= mesh.mFaces.size())
				{
					this->LogWarning("UV-Face has an invalid index. It will be ignored");
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
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level3 block, this can't be
			BLUBB("Unable to finish parsing LV3 *MESH_TFACELIST block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
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
		if ('*' == *this->m_szFile)
		{
			// Number of texture coordinates in the mesh
			if (0 == strncmp(this->m_szFile,"*MESH_NUMTVERTEX" ,16) &&
				IsSpaceOrNewLine(*(this->m_szFile+16)))
			{
				this->m_szFile+=17;
				this->ParseLV4MeshLong(iNumTVertices);continue;
			}
			// Number of UVWed faces in the mesh
			if (0 == strncmp(this->m_szFile,"*MESH_NUMTVFACES" ,16) &&
				IsSpaceOrNewLine(*(this->m_szFile+16)))
			{
				this->m_szFile+=17;
				this->ParseLV4MeshLong(iNumTFaces);continue;
			}
			// mesh texture vertex list block
			if (0 == strncmp(this->m_szFile,"*MESH_TVERTLIST" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->SkipOpeningBracket();
				this->ParseLV3MeshTListBlock(iNumTVertices,mesh,iChannel);
				continue;
			}
			// mesh texture face block
			if (0 == strncmp(this->m_szFile,"*MESH_TFACELIST" ,15) &&
				IsSpaceOrNewLine(*(this->m_szFile+15)))
			{
				this->m_szFile+=16;
				this->SkipOpeningBracket();
				this->ParseLV3MeshTFaceListBlock(iNumTFaces,mesh, iChannel);
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level2 block, this can't be
			BLUBB("Unable to finish parsing a LV3 *MESH_MAPPINGCHANNEL block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
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
		if ('*' == *this->m_szFile)
		{
			// Vertex entry
			if (0 == strncmp(this->m_szFile,"*MESH_VERTCOL" ,13) &&
				IsSpaceOrNewLine(*(this->m_szFile+13)))
			{
				this->m_szFile+=14;

				aiColor4D vTemp;
				vTemp.a = 1.0f;
				unsigned int iIndex;
				this->ParseLV4MeshFloatTriple(&vTemp.r,iIndex);

				if (iIndex >= iNumVertices)
				{
					this->LogWarning("Vertex color has an invalid index. It will be ignored");
				}
				else mesh.mVertexColors[iIndex] = vTemp;
				continue;
			}
		}
		if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level3 block, this can't be
			BLUBB("Unable to finish parsing LV3 *MESH_CVERTLIST block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshCFaceListBlock(unsigned int iNumFaces, ASE::Mesh& mesh)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			// Face entry
			if (0 == strncmp(this->m_szFile,"*MESH_CFACE" ,12) &&
				IsSpaceOrNewLine(*(this->m_szFile+12)))
			{
				this->m_szFile+=13;

				unsigned int aiValues[3];
				unsigned int iIndex = 0;

				this->ParseLV4MeshLongTriple(aiValues,iIndex);
				if (iIndex >= iNumFaces || iIndex >= mesh.mFaces.size())
				{
					this->LogWarning("UV-Face has an invalid index. It will be ignored");
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
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		else if ('\0' == *this->m_szFile)
		{
			// END OF FILE ... this is a level3 block, this can't be
			BLUBB("Unable to finish parsing LV3 *MESH_CFACELIST block. Unexpected EOF")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		++this->m_szFile;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3MeshNormalListBlock(ASE::Mesh& sMesh)
{
	// allocate enough storage for the normals
	sMesh.mNormals.resize(sMesh.mPositions.size());
	int iDepth = 0;

	// we need the *MESH_VERTEXNORMAL blocks, ignore the face normals
	// if there are only face normals we calculate them outselfes using the SGs
	while (true)
	{
		if ('*' == *this->m_szFile)
		{
			if (0 == strncmp(this->m_szFile,"*MESH_VERTEXNORMAL",18) && IsSpaceOrNewLine(*(this->m_szFile+18)))
			{
				this->m_szFile += 19;

				// parse a simple float triple
				aiVector3D vNormal;
				unsigned int iIndex = 0;
				this->ParseLV4MeshFloatTriple(&vNormal.x,iIndex);

				if (iIndex >= sMesh.mNormals.size())
				{
					this->LogWarning("Normal index is too large");
					iIndex = (unsigned int)sMesh.mNormals.size()-1;
				}

				// important: this->m_szFile might now point to '}' ...
				sMesh.mNormals[iIndex] = vNormal;
				continue;
			}
		}
		else if ('{' == *this->m_szFile)iDepth++;
		else if ('}' == *this->m_szFile)
		{
			if (0 == --iDepth){++this->m_szFile;this->SkipToNextToken();return;}
		}
		// seems we have reached the end of the file ... 
		else if ('\0' == *this->m_szFile)
		{
			BLUBB("Unable to parse *MESH_NORMALS Element: Unexpected EOL [#1]")
		}
		else if(IsLineEnd(*this->m_szFile))++this->iLineNumber;
		this->m_szFile++;
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshFace(ASE::Face& out)
{	
	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		this->LogWarning("Unable to parse *MESH_FACE Element: Unexpected EOL [#1]");
		this->SkipToNextToken();
		return;
	}

	// parse the face index
	out.iFace = strtol10(this->m_szFile,&this->m_szFile);

	// next character should be ':'
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// FIX: there are some ASE files which haven't got : here ....
		this->LogWarning("Unable to parse *MESH_FACE Element: Unexpected EOL. \':\' expected [#2]");
		this->SkipToNextToken();
		return;
	}
	// FIX: there are some ASE files which haven't got : here ....
	if(':' == *this->m_szFile)++this->m_szFile;

	// parse all mesh indices
	for (unsigned int i = 0; i < 3;++i)
	{
		unsigned int iIndex = 0;
		if(!SkipSpaces(this->m_szFile,&this->m_szFile))
		{
			// LOG 
__EARTHQUAKE_XXL:
			this->LogWarning("Unable to parse *MESH_FACE Element: Unexpected EOL. "
				"A,B or C expected [#3]");
			this->SkipToNextToken();
			return;
		}
		switch (*this->m_szFile)
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
		++this->m_szFile;

		// next character should be ':'
		if(!SkipSpaces(this->m_szFile,&this->m_szFile) || ':' != *this->m_szFile)
		{
			this->LogWarning("Unable to parse *MESH_FACE Element: "
				"Unexpected EOL. \':\' expected [#2]");
			this->SkipToNextToken();
			return;
		}

		++this->m_szFile;
		if(!SkipSpaces(this->m_szFile,&this->m_szFile))
		{
			this->LogWarning("Unable to parse *MESH_FACE Element: Unexpected EOL. "
				"Vertex index ecpected [#4]");
			this->SkipToNextToken();
			return;
		}
		out.mIndices[iIndex] = strtol10(this->m_szFile,&this->m_szFile);
	}

	// now we need to skip the AB, BC, CA blocks. 
	while (true)
	{
		if ('*' == *this->m_szFile)break;
		if (IsLineEnd(*this->m_szFile))
		{
			//this->iLineNumber++;
			return;
		}
		this->m_szFile++;
	}

	// parse the smoothing group of the face
	if (0 == strncmp(this->m_szFile,"*MESH_SMOOTHING",15) && 
		IsSpaceOrNewLine(*(this->m_szFile+15)))
	{
		this->m_szFile+=16;
		if(!SkipSpaces(this->m_szFile,&this->m_szFile))
		{
			this->LogWarning("Unable to parse *MESH_SMOOTHING Element: "
				"Unexpected EOL. Smoothing group(s) expected [#5]");
			this->SkipToNextToken();
			return;
		}
		
		// parse smoothing groups until we don_t anymore see commas
		// FIX: There needn't always be a value, sad but true
		while (true)
		{
			if (*this->m_szFile < '9' && *this->m_szFile >= '0')
			{
				out.iSmoothGroup |= (1 << strtol10(this->m_szFile,&this->m_szFile));
			}
			SkipSpaces(this->m_szFile,&this->m_szFile);
			if (',' != *this->m_szFile)
			{
				break;
			}
			++this->m_szFile;
			SkipSpaces(this->m_szFile,&this->m_szFile);
		}
	}

	// *MESH_MTLID  is optional, too
	while (true)
	{
		if ('*' == *this->m_szFile)break;
		if (IsLineEnd(*this->m_szFile))
		{
			//this->iLineNumber++;
			return;
		}
		this->m_szFile++;
	}

	if (0 == strncmp(this->m_szFile,"*MESH_MTLID",11) && IsSpaceOrNewLine(*(this->m_szFile+11)))
	{
		this->m_szFile+=12;
		if(!SkipSpaces(this->m_szFile,&this->m_szFile))
		{
			this->LogWarning("Unable to parse *MESH_MTLID Element: Unexpected EOL. "
				"Material index expected [#6]");
			this->SkipToNextToken();
			return;
		}
		out.iMaterial = strtol10(this->m_szFile,&this->m_szFile);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshLongTriple(unsigned int* apOut)
{
	ai_assert(NULL != apOut);

	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse indexable long triple: unexpected EOL [#1]");
		++this->iLineNumber;
		apOut[0] = apOut[1] = apOut[2] = 0;
		return;
	}
	apOut[0] = strtol10(this->m_szFile,&this->m_szFile);

	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse indexable long triple: unexpected EOL [#2]");
		++this->iLineNumber;
		apOut[1] = apOut[2] = 0;
		return;
	}
	apOut[1] = strtol10(this->m_szFile,&this->m_szFile);

	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse indexable long triple: unexpected EOL [#3]");
		apOut[2] = 0;
		++this->iLineNumber;
		return;
	}
	apOut[2] = strtol10(this->m_szFile,&this->m_szFile);
	// go to the next valid sequence
	//SkipSpacesAndLineEnd(this->m_szFile,&this->m_szFile);
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshLongTriple(unsigned int* apOut, unsigned int& rIndexOut)
{
	ai_assert(NULL != apOut);

	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse indexable long triple: unexpected EOL [#4]");
		rIndexOut = 0;
		apOut[0] = apOut[1] = apOut[2] = 0;
		++this->iLineNumber;
		return;
	}
	// parse the index
	rIndexOut = strtol10(this->m_szFile,&this->m_szFile);

	// parse the three others
	this->ParseLV4MeshLongTriple(apOut);
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshFloatTriple(float* apOut, unsigned int& rIndexOut)
{
	ai_assert(NULL != apOut);

	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse indexable float triple: unexpected EOL [#1]");
		rIndexOut = 0;
		apOut[0] = apOut[1] = apOut[2] = 0.0f;
		++this->iLineNumber;
		return;
	}
	// parse the index
	rIndexOut = strtol10(this->m_szFile,&this->m_szFile);
	
	// parse the three others
	this->ParseLV4MeshFloatTriple(apOut);
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshFloatTriple(float* apOut)
{
	ai_assert(NULL != apOut);
	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse float triple: unexpected EOL [#5]");
		apOut[0] = apOut[1] = apOut[2] = 0.0f;
		++this->iLineNumber;
		return;
	}
	// parse the first float
	this->m_szFile = fast_atof_move(this->m_szFile,apOut[0]);
	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse float triple: unexpected EOL [#6]");
		apOut[1] = apOut[2] = 0.0f;
		++this->iLineNumber;
		return;
	}
	// parse the second float
	this->m_szFile = fast_atof_move(this->m_szFile,apOut[1]);
	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse float triple: unexpected EOL [#7]");
		apOut[2] = 0.0f;
		++this->iLineNumber;
		return;
	}
	// parse the third float
	this->m_szFile = fast_atof_move(this->m_szFile,apOut[2]);
	// go to the next valid sequence
	//this->SkipToNextToken();
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshFloat(float& fOut)
{
	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse float: unexpected EOL [#1]");
		fOut = 0.0f;
		++this->iLineNumber;
		return;
	}
	// parse the first float
	this->m_szFile = fast_atof_move(this->m_szFile,fOut);
	// go to the next valid sequence
	//this->SkipToNextToken();
	return;
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV4MeshLong(unsigned int& iOut)
{
	// skip spaces and tabs
	if(!SkipSpaces(this->m_szFile,&this->m_szFile))
	{
		// LOG 
		this->LogWarning("Unable to parse long: unexpected EOL [#1]");
		iOut = 0;
		++this->iLineNumber;
		return;
	}
	// parse the value
	iOut = strtol10(this->m_szFile,&this->m_szFile);
	// go to the next valid sequence
	//this->SkipToNextToken();
	return;
}