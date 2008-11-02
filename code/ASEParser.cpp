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

#include "AssimpPCH.h"

// internal headers
#include "TextureTransform.h"
#include "ASELoader.h"
#include "MaterialSystem.h"
#include "fast_atof.h"

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
	snprintf(szTemp,1024,"Line %i: %s",iLineNumber,szWarn);
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
	snprintf(szTemp,1024,"Line %i: %s",iLineNumber,szWarn);
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
	snprintf(szTemp,1024,"Line %i: %s",iLineNumber,szWarn);
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
			++m_szFile;

			// version should be 200. Validate this ...
			if (TokenMatch(m_szFile,"3DSMAX_ASCIIEXPORT",18))
			{
				unsigned int iVersion;
				ParseLV4MeshLong(iVersion);

				if (iVersion > 200)
				{
					LogWarning("Unknown file format version: *3DSMAX_ASCIIEXPORT should \
						be <=200. Continuing happily ...");
				}
				continue;
			}
			// main scene information
			if (TokenMatch(m_szFile,"SCENE",5))
			{
				ParseLV1SceneBlock();
				continue;
			}
			// "group" - no implementation yet, in facte
			// we're just ignoring them for the moment
			if (TokenMatch(m_szFile,"GROUP",5)) 
			{
				Parse();
				continue;
			}
			// material list
			if (TokenMatch(m_szFile,"MATERIAL_LIST",13)) 
			{
				ParseLV1MaterialListBlock();
				continue;
			}
			// geometric object (mesh)
			if (TokenMatch(m_szFile,"GEOMOBJECT",10)) 
				
			{
				m_vMeshes.push_back(Mesh());
				ParseLV1ObjectBlock(m_vMeshes.back());
				continue;
			}
			// helper object = dummy in the hierarchy
			if (TokenMatch(m_szFile,"HELPEROBJECT",12)) 
				
			{
				m_vDummies.push_back(Dummy());
				ParseLV1ObjectBlock(m_vDummies.back());
				continue;
			}
			// light object
			if (TokenMatch(m_szFile,"LIGHTOBJECT",11)) 
				
			{
				m_vLights.push_back(Light());
				ParseLV1ObjectBlock(m_vLights.back());
				continue;
			}
			// camera object
			if (TokenMatch(m_szFile,"CAMERAOBJECT",12)) 
			{
				m_vCameras.push_back(Camera());
				ParseLV1ObjectBlock(m_vCameras.back());
				continue;
			}
			// comment - print it on the console
			if (TokenMatch(m_szFile,"COMMENT",7)) 
			{
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
			++m_szFile;
			if (TokenMatch(m_szFile,"SCENE_BACKGROUND_STATIC",23)) 
				
			{
				// parse a color triple and assume it is really the bg color
				ParseLV4MeshFloatTriple( &m_clrBackground.r );
				continue;
			}
			if (TokenMatch(m_szFile,"SCENE_AMBIENT_STATIC",20)) 
				
			{
				// parse a color triple and assume it is really the bg color
				ParseLV4MeshFloatTriple( &m_clrAmbient.r );
				continue;
			}
			if (TokenMatch(m_szFile,"SCENE_FIRSTFRAME",16)) 
			{
				ParseLV4MeshLong(iFirstFrame);
				continue;
			}
			if (TokenMatch(m_szFile,"SCENE_LASTFRAME",15))
			{
				ParseLV4MeshLong(iLastFrame);
				continue;
			}
			if (TokenMatch(m_szFile,"SCENE_FRAMESPEED",16)) 
			{
				ParseLV4MeshLong(iFrameSpeed);
				continue;
			}
			if (TokenMatch(m_szFile,"SCENE_TICKSPERFRAME",19))
			{
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
			++m_szFile;
			if (TokenMatch(m_szFile,"MATERIAL_COUNT",14))
			{
				ParseLV4MeshLong(iMaterialCount);

				// now allocate enough storage to hold all materials
				m_vMaterials.resize(iOldMaterialCount+iMaterialCount);
				continue;
			}
			if (TokenMatch(m_szFile,"MATERIAL",8))
			{
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
			++m_szFile;
			if (TokenMatch(m_szFile,"MATERIAL_NAME",13))
			{
				if (!ParseString(mat.mName,"*MATERIAL_NAME"))
					SkipToNextToken();
				continue;
			}
			// ambient material color
			if (TokenMatch(m_szFile,"MATERIAL_AMBIENT",16))
			{
				ParseLV4MeshFloatTriple(&mat.mAmbient.r);
				continue;
			}
			// diffuse material color
			if (TokenMatch(m_szFile,"MATERIAL_DIFFUSE",16) )
			{
				ParseLV4MeshFloatTriple(&mat.mDiffuse.r);
				continue;
			}
			// specular material color
			if (TokenMatch(m_szFile,"MATERIAL_SPECULAR",17))
			{
				ParseLV4MeshFloatTriple(&mat.mSpecular.r);
				continue;
			}
			// material shading type
			if (TokenMatch(m_szFile,"MATERIAL_SHADING",16))
			{
				if (TokenMatch(m_szFile,"Blinn",5))
				{
					mat.mShading = Discreet3DS::Blinn;
				}
				else if (TokenMatch(m_szFile,"Phong",5))
				{
					mat.mShading = Discreet3DS::Phong;
				}
				else if (TokenMatch(m_szFile,"Flat",4))
				{
					mat.mShading = Discreet3DS::Flat;
				}
				else if (TokenMatch(m_szFile,"Wire",4))
				{
					mat.mShading = Discreet3DS::Wire;
				}
				else
				{
					// assume gouraud shading
					mat.mShading = Discreet3DS::Gouraud;
					SkipToNextToken();
				}
				continue;
			}
			// material transparency
			if (TokenMatch(m_szFile,"MATERIAL_TRANSPARENCY",21))
			{
				ParseLV4MeshFloat(mat.mTransparency);
				mat.mTransparency = 1.0f - mat.mTransparency;continue;
			}
			// material self illumination
			if (TokenMatch(m_szFile,"MATERIAL_SELFILLUM",18))
			{
				float f = 0.0f;
				ParseLV4MeshFloat(f);

				mat.mEmissive.r = f;
				mat.mEmissive.g = f;
				mat.mEmissive.b = f;
				continue;
			}
			// material shininess
			if (TokenMatch(m_szFile,"MATERIAL_SHINE",14) )
			{
				ParseLV4MeshFloat(mat.mSpecularExponent);
				mat.mSpecularExponent *= 15;
				continue;
			}
			// material shininess strength
			if (TokenMatch(m_szFile,"MATERIAL_SHINESTRENGTH",22))
			{
				ParseLV4MeshFloat(mat.mShininessStrength);
				continue;
			}
			// diffuse color map
			if (TokenMatch(m_szFile,"MAP_DIFFUSE",11))
			{
				// parse the texture block
				ParseLV3MapBlock(mat.sTexDiffuse);
				continue;
			}
			// ambient color map
			if (TokenMatch(m_szFile,"MAP_AMBIENT",11))
			{
				// parse the texture block
				ParseLV3MapBlock(mat.sTexAmbient);
				continue;
			}
			// specular color map
			if (TokenMatch(m_szFile,"MAP_SPECULAR",12))
			{
				// parse the texture block
				ParseLV3MapBlock(mat.sTexSpecular);
				continue;
			}
			// opacity map
			if (TokenMatch(m_szFile,"MAP_OPACITY",11))
			{
				// parse the texture block
				ParseLV3MapBlock(mat.sTexOpacity);
				continue;
			}
			// emissive map
			if (TokenMatch(m_szFile,"MAP_SELFILLUM",13))
			{
				// parse the texture block
				ParseLV3MapBlock(mat.sTexEmissive);
				continue;
			}
			// bump map
			if (TokenMatch(m_szFile,"MAP_BUMP",8))
			{
				// parse the texture block
				ParseLV3MapBlock(mat.sTexBump);
			}
			// specular/shininess map
			if (TokenMatch(m_szFile,"MAP_SHINESTRENGTH",17))
			{
				// parse the texture block
				ParseLV3MapBlock(mat.sTexShininess);
				continue;
			}
			// number of submaterials
			if (TokenMatch(m_szFile,"NUMSUBMTLS",10))
			{
				ParseLV4MeshLong(iNumSubMaterials);

				// allocate enough storage
				mat.avSubMaterials.resize(iNumSubMaterials);
			}
			// submaterial chunks
			if (TokenMatch(m_szFile,"SUBMATERIAL",11))
			{
			
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

	// *BITMAP should not be there if *MAP_CLASS is not BITMAP,
	// but we need to expect that case ... if the path is
	// empty the texture won't be used later.
	bool parsePath = true; 
	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;
			// type of map
			if (TokenMatch(m_szFile,"MAP_CLASS" ,9))
			{
				std::string temp;
				if(!ParseString(temp,"*MAP_CLASS"))
					SkipToNextToken();
				if (temp != "Bitmap")
				{
					DefaultLogger::get()->warn("ASE: Skipping unknown map type: " + temp);
					parsePath = false; 
				}
				continue;
			}
			// path to the texture
			if (parsePath && TokenMatch(m_szFile,"BITMAP" ,6))
			{
				if(!ParseString(map.mMapName,"*BITMAP"))
					SkipToNextToken();

				if (map.mMapName == "None")
				{
					// Files with 'None' as map name are produced by
					// an Maja to ASE exporter which name I forgot ..
					DefaultLogger::get()->warn("ASE: Skipping invalid map entry");
					map.mMapName = "";
				}

				continue;
			}
			// offset on the u axis
			if (TokenMatch(m_szFile,"UVW_U_OFFSET" ,12))
			{
				ParseLV4MeshFloat(map.mOffsetU);
				continue;
			}
			// offset on the v axis
			if (TokenMatch(m_szFile,"UVW_V_OFFSET" ,12))
			{
				ParseLV4MeshFloat(map.mOffsetV);
				continue;
			}
			// tiling on the u axis
			if (TokenMatch(m_szFile,"UVW_U_TILING" ,12))
			{
				ParseLV4MeshFloat(map.mScaleU);
				continue;
			}
			// tiling on the v axis
			if (TokenMatch(m_szFile,"UVW_V_TILING" ,12))
			{
				ParseLV4MeshFloat(map.mScaleV);
				continue;
			}
			// rotation around the z-axis
			if (TokenMatch(m_szFile,"UVW_ANGLE" ,9))
			{
				ParseLV4MeshFloat(map.mRotation);
				continue;
			}
			// map blending factor
			if (TokenMatch(m_szFile,"MAP_AMOUNT" ,10))
			{
				ParseLV4MeshFloat(map.mTextureBlend);
				continue;
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
	if (!SkipSpaces(&m_szFile))
	{
		sprintf(szBuffer,"Unable to parse %s block: Unexpected EOL",szName);
		LogWarning(szBuffer);
		return false;
	}
	// there must be "
	if ('\"' != *m_szFile)
	{
		sprintf(szBuffer,"Unable to parse %s block: Strings are expected "
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
			sprintf(szBuffer,"Unable to parse %s block: Strings are expected to be "
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
void Parser::ParseLV1ObjectBlock(ASE::BaseNode& node)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;

			// first process common tokens such as node name and transform
			// name of the mesh/node
			if (TokenMatch(m_szFile,"NODE_NAME" ,9))
			{
				if(!ParseString(node.mName,"*NODE_NAME"))
					SkipToNextToken();
				continue;
			}
			// name of the parent of the node
			if (TokenMatch(m_szFile,"NODE_PARENT" ,11) )
			{
				if(!ParseString(node.mParent,"*NODE_PARENT"))
					SkipToNextToken();
				continue;
			}
			// transformation matrix of the node
			if (TokenMatch(m_szFile,"NODE_TM" ,7))
			{
				ParseLV2NodeTransformBlock(node);
				continue;
			}
			// animation data of the node
			if (TokenMatch(m_szFile,"TM_ANIMATION" ,12))
			{
				ParseLV2AnimationBlock(node);
				continue;
			}

			if (node.mType == BaseNode::Light)
			{
				// light settings
				if (TokenMatch(m_szFile,"LIGHT_SETTINGS" ,14))
				{
					ParseLV2LightSettingsBlock((ASE::Light&)node);
					continue;
				}
				// type of the light source
				if (TokenMatch(m_szFile,"LIGHT_TYPE" ,10))
				{
					if (!ASSIMP_strincmp("omni",m_szFile,4))
					{
						((ASE::Light&)node).mLightType = ASE::Light::OMNI;
					}
					else if (!ASSIMP_strincmp("target",m_szFile,6))
					{
						((ASE::Light&)node).mLightType = ASE::Light::TARGET;
					}
					else if (!ASSIMP_strincmp("free",m_szFile,4))
					{
						((ASE::Light&)node).mLightType = ASE::Light::FREE;
					}
					else if (!ASSIMP_strincmp("directional",m_szFile,11))
					{
						((ASE::Light&)node).mLightType = ASE::Light::DIRECTIONAL;
					}
					else
					{
						// TODO: use std::string as parameter for LogWarning
						LogWarning("Unknown kind of light source");
					}
					continue;
				}
			}
			else if (node.mType == BaseNode::Camera)
			{
				// Camera settings
				if (TokenMatch(m_szFile,"CAMERA_SETTINGS" ,15))
				{
					ParseLV2CameraSettingsBlock((ASE::Camera&)node);
					continue;
				}
			}
			else if (node.mType == BaseNode::Mesh)
			{
				// mesh data
				if (TokenMatch(m_szFile,"MESH" ,4))
				{
					ParseLV2MeshBlock((ASE::Mesh&)node);
					continue;
				}
				// mesh material index
				if (TokenMatch(m_szFile,"MATERIAL_REF" ,12))
				{
					ParseLV4MeshLong(((ASE::Mesh&)node).iMaterialIndex);
					continue;
				}
			}
		}
		AI_ASE_HANDLE_TOP_LEVEL_SECTION(iDepth);
	}
	return;
}

// ------------------------------------------------------------------------------------------------
void Parser::ParseLV2CameraSettingsBlock(ASE::Camera& camera)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;
			if (TokenMatch(m_szFile,"CAMERA_NEAR" ,11))
			{
				ParseLV4MeshFloat(camera.mNear);
				continue;
			}
			if (TokenMatch(m_szFile,"CAMERA_FAR" ,10))
			{
				ParseLV4MeshFloat(camera.mFar);
				continue;
			}
			if (TokenMatch(m_szFile,"CAMERA_FOV" ,10))
			{
				ParseLV4MeshFloat(camera.mFOV);
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"2","CAMERA_SETTINGS");
	}
	return;
}

// ------------------------------------------------------------------------------------------------
void Parser::ParseLV2LightSettingsBlock(ASE::Light& light)
{
	int iDepth = 0;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;
			if (TokenMatch(m_szFile,"LIGHT_COLOR" ,11))
			{
				ParseLV4MeshFloatTriple(&light.mColor.r);
				continue;
			}
			if (TokenMatch(m_szFile,"LIGHT_INTENS" ,12))
			{
				ParseLV4MeshFloat(light.mIntensity);
				continue;
			}
			if (TokenMatch(m_szFile,"LIGHT_HOTSPOT" ,13))
			{
				ParseLV4MeshFloat(light.mAngle);
				continue;
			}
			if (TokenMatch(m_szFile,"LIGHT_FALLOFF" ,13))
			{
				ParseLV4MeshFloat(light.mFalloff);
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"2","LIGHT_SETTINGS");
	}
	return;
}

// ------------------------------------------------------------------------------------------------
void Parser::ParseLV2AnimationBlock(ASE::BaseNode& mesh)
{
	int iDepth = 0;

	ASE::Animation* anim = &mesh.mAnim;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;
			if (TokenMatch(m_szFile,"NODE_NAME" ,9))
			{
				std::string temp;
				if(!ParseString(temp,"*NODE_NAME"))
					SkipToNextToken();

				// If the name of the node contains .target it 
				// represents an animated camera or spot light
				// target.
				if (std::string::npos != temp.find(".Target"))
				{
					if  (mesh.mType != BaseNode::Camera  &&
						(mesh.mType != BaseNode::Light || 
						((ASE::Light&)mesh).mLightType != ASE::Light::TARGET))
					{   /* it is not absolutely sure we know the type of the light source yet */

						DefaultLogger::get()->error("ASE: Found target animation channel "
							"but the node is neither a camera nor a spot light");
						anim = NULL;
					}
					else anim = &mesh.mTargetAnim;
				}
				continue;
			}

			// position keyframes
			if (TokenMatch(m_szFile,"CONTROL_POS_TRACK"  ,17)  ||
				TokenMatch(m_szFile,"CONTROL_POS_BEZIER" ,18)  ||
				TokenMatch(m_szFile,"CONTROL_POS_TCB"    ,15))
			{
				if (!anim)SkipSection();
				else ParseLV3PosAnimationBlock(*anim);
				continue;
			}
			// scaling keyframes
			if (TokenMatch(m_szFile,"CONTROL_SCALE_TRACK"  ,19) ||
				TokenMatch(m_szFile,"CONTROL_SCALE_BEZIER" ,20) ||
				TokenMatch(m_szFile,"CONTROL_SCALE_TCB"    ,17))
			{
				if (!anim || anim == &mesh.mTargetAnim)
				{
					// Target animation channels may have no rotation channels
					DefaultLogger::get()->error("ASE: Ignoring scaling channel in target animation");
					SkipSection();
				}
				else ParseLV3ScaleAnimationBlock(*anim);
				continue;
			}
			// rotation keyframes
			if (TokenMatch(m_szFile,"CONTROL_ROT_TRACK"  ,17) ||
				TokenMatch(m_szFile,"CONTROL_ROT_BEZIER" ,18) ||
				TokenMatch(m_szFile,"CONTROL_ROT_TCB"    ,15))
			{
				if (!anim || anim == &mesh.mTargetAnim)
				{
					// Target animation channels may have no rotation channels
					DefaultLogger::get()->error("ASE: Ignoring rotation channel in target animation");
					SkipSection();
				}
				else ParseLV3RotAnimationBlock(*anim);
				continue;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"2","TM_ANIMATION");
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3ScaleAnimationBlock(ASE::Animation& anim)
{
	int iDepth = 0;
	unsigned int iIndex;

	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;

			bool b = false;

			// For the moment we're just reading the three floats -
			// we ignore the ádditional information for bezier's and TCBs

			// simple scaling keyframe
			if (TokenMatch(m_szFile,"CONTROL_SCALE_SAMPLE" ,20))
			{
				b = true;
				anim.mScalingType = ASE::Animation::TRACK;
			}

			// Bezier scaling keyframe
			if (TokenMatch(m_szFile,"CONTROL_BEZIER_SCALE_KEY" ,24))
			{
				b = true;
				anim.mScalingType = ASE::Animation::BEZIER;
			}
			// TCB scaling keyframe
			if (TokenMatch(m_szFile,"CONTROL_TCB_SCALE_KEY" ,21))
			{
				b = true;
				anim.mScalingType = ASE::Animation::TCB;
			}
			if (b)
			{
				anim.akeyScaling.push_back(aiVectorKey());
				aiVectorKey& key = anim.akeyScaling.back();
				ParseLV4MeshFloatTriple(&key.mValue.x,iIndex);
				key.mTime = (double)iIndex;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*CONTROL_POS_TRACK");
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3PosAnimationBlock(ASE::Animation& anim)
{
	int iDepth = 0;
	unsigned int iIndex;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;
			
			bool b = false;

			// For the moment we're just reading the three floats -
			// we ignore the ádditional information for bezier's and TCBs

			// simple scaling keyframe
			if (TokenMatch(m_szFile,"CONTROL_POS_SAMPLE" ,18))
			{
				b = true;
				anim.mPositionType = ASE::Animation::TRACK;
			}

			// Bezier scaling keyframe
			if (TokenMatch(m_szFile,"CONTROL_BEZIER_POS_KEY" ,22))
			{
				b = true;
				anim.mPositionType = ASE::Animation::BEZIER;
			}
			// TCB scaling keyframe
			if (TokenMatch(m_szFile,"CONTROL_TCB_POS_KEY" ,19))
			{
				b = true;
				anim.mPositionType = ASE::Animation::TCB;
			}
			if (b)
			{
				anim.akeyPositions.push_back(aiVectorKey());
				aiVectorKey& key = anim.akeyPositions.back();
				ParseLV4MeshFloatTriple(&key.mValue.x,iIndex);
				key.mTime = (double)iIndex;
			}
		}
		AI_ASE_HANDLE_SECTION(iDepth,"3","*CONTROL_POS_TRACK");
	}
}
// ------------------------------------------------------------------------------------------------
void Parser::ParseLV3RotAnimationBlock(ASE::Animation& anim)
{
	int iDepth = 0;
	unsigned int iIndex;
	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;

			bool b = false;

			// For the moment we're just reading the  floats -
			// we ignore the ádditional information for bezier's and TCBs

			// simple scaling keyframe
			if (TokenMatch(m_szFile,"CONTROL_ROT_SAMPLE" ,18))
			{
				b = true;
				anim.mRotationType = ASE::Animation::TRACK;
			}

			// Bezier scaling keyframe
			if (TokenMatch(m_szFile,"CONTROL_BEZIER_ROT_KEY" ,22))
			{
				b = true;
				anim.mRotationType = ASE::Animation::BEZIER;
			}
			// TCB scaling keyframe
			if (TokenMatch(m_szFile,"CONTROL_TCB_ROT_KEY" ,19))
			{
				b = true;
				anim.mRotationType = ASE::Animation::TCB;
			}
			if (b)
			{
				anim.akeyRotations.push_back(aiQuatKey());
				aiQuatKey& key = anim.akeyRotations.back();
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
void Parser::ParseLV2NodeTransformBlock(ASE::BaseNode& mesh)
{
	int iDepth = 0;
	int mode   = 0; 
	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;
			// name of the node
			if (TokenMatch(m_szFile,"NODE_NAME" ,9))
			{
				std::string temp;
				if(!ParseString(temp,"*NODE_NAME"))
					SkipToNextToken();

				std::string::size_type s;
				if (temp == mesh.mName)
				{
					mode = 1;
				}
				else if (std::string::npos != (s = temp.find(".Target")) &&
					mesh.mName == temp.substr(0,s))
				{
					// This should be either a target light or a target camera
					if ( mesh.mType == BaseNode::Light &&  ((ASE::Light&)mesh) .mLightType  == ASE::Light::TARGET ||
						 mesh.mType == BaseNode::Camera && ((ASE::Camera&)mesh).mCameraType == ASE::Camera::TARGET)
					{
						mode = 2;
					}
					else DefaultLogger::get()->error("ASE: Ignoring target transform, "
						"this is no spot light or target camera");
				}
				else
				{
					DefaultLogger::get()->error("ASE: Unknown node transformation: " + temp);
					// mode = 0
				}
				continue;
			}
			if (mode)
			{
				// fourth row of the transformation matrix - and also the 
				// only information here that is interesting for targets
				if (TokenMatch(m_szFile,"TM_ROW3" ,7))
				{
					ParseLV4MeshFloatTriple((mode == 1 ? mesh.mTransform[3] : &mesh.mTargetPosition.x));
					continue;
				}
				if (mode == 1)
				{
					// first row of the transformation matrix
					if (TokenMatch(m_szFile,"TM_ROW0" ,7))
					{
						ParseLV4MeshFloatTriple(mesh.mTransform[0]);
						continue;
					}
					// second row of the transformation matrix
					if (TokenMatch(m_szFile,"TM_ROW1" ,7))
					{
						ParseLV4MeshFloatTriple(mesh.mTransform[1]);
						continue;
					}
					// third row of the transformation matrix
					if (TokenMatch(m_szFile,"TM_ROW2" ,7))
					{
						ParseLV4MeshFloatTriple(mesh.mTransform[2]);
						continue;
					}
					// inherited position axes
					if (TokenMatch(m_szFile,"INHERIT_POS" ,11))
					{
						unsigned int aiVal[3];
						ParseLV4MeshLongTriple(aiVal);

						for (unsigned int i = 0; i < 3;++i)
							mesh.inherit.abInheritPosition[i] = aiVal[i] != 0;
						continue;
					}
					// inherited rotation axes
					if (TokenMatch(m_szFile,"INHERIT_ROT" ,11))
					{
						unsigned int aiVal[3];
						ParseLV4MeshLongTriple(aiVal);

						for (unsigned int i = 0; i < 3;++i)
							mesh.inherit.abInheritRotation[i] = aiVal[i] != 0;
						continue;
					}
					// inherited scaling axes
					if (TokenMatch(m_szFile,"INHERIT_SCL" ,11))
					{
						unsigned int aiVal[3];
						ParseLV4MeshLongTriple(aiVal);

						for (unsigned int i = 0; i < 3;++i)
							mesh.inherit.abInheritScaling[i] = aiVal[i] != 0;
						continue;
					}
				}
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
			++m_szFile;
			// Number of vertices in the mesh
			if (TokenMatch(m_szFile,"MESH_NUMVERTEX" ,14))
			{
				ParseLV4MeshLong(iNumVertices);
				continue;
			}
			// Number of texture coordinates in the mesh
			if (TokenMatch(m_szFile,"MESH_NUMTVERTEX" ,15))
			{
				ParseLV4MeshLong(iNumTVertices);
				continue;
			}
			// Number of vertex colors in the mesh
			if (TokenMatch(m_szFile,"MESH_NUMCVERTEX" ,14))
			{
				ParseLV4MeshLong(iNumCVertices);
				continue;
			}
			// Number of regular faces in the mesh
			if (TokenMatch(m_szFile,"MESH_NUMFACES" ,13))
			{
				ParseLV4MeshLong(iNumFaces);
				continue;
			}
			// Number of UVWed faces in the mesh
			if (TokenMatch(m_szFile,"MESH_NUMTVFACES" ,15))
			{
				ParseLV4MeshLong(iNumTFaces);
				continue;
			}
			// Number of colored faces in the mesh
			if (TokenMatch(m_szFile,"MESH_NUMCVFACES" ,15))
			{
				ParseLV4MeshLong(iNumCFaces);
				continue;
			}
			// mesh vertex list block
			if (TokenMatch(m_szFile,"MESH_VERTEX_LIST" ,16))
			{
				ParseLV3MeshVertexListBlock(iNumVertices,mesh);
				continue;
			}
			// mesh face list block
			if (TokenMatch(m_szFile,"MESH_FACE_LIST" ,14))
			{
				ParseLV3MeshFaceListBlock(iNumFaces,mesh);
				continue;
			}
			// mesh texture vertex list block
			if (TokenMatch(m_szFile,"MESH_TVERTLIST" ,14))
			{
				ParseLV3MeshTListBlock(iNumTVertices,mesh);
				continue;
			}
			// mesh texture face block
			if (TokenMatch(m_szFile,"MESH_TFACELIST" ,14))
			{
				ParseLV3MeshTFaceListBlock(iNumTFaces,mesh);
				continue;
			}
			// mesh color vertex list block
			if (TokenMatch(m_szFile,"MESH_CVERTLIST" ,14))
			{
				ParseLV3MeshCListBlock(iNumCVertices,mesh);
				continue;
			}
			// mesh color face block
			if (TokenMatch(m_szFile,"MESH_CFACELIST" ,14))
			{
				ParseLV3MeshCFaceListBlock(iNumCFaces,mesh);
				continue;
			}
			// mesh normals
			if (TokenMatch(m_szFile,"MESH_NORMALS" ,12))
			{
				ParseLV3MeshNormalListBlock(mesh);
				continue;
			}
			// another mesh UV channel ...
			if (TokenMatch(m_szFile,"MESH_MAPPINGCHANNEL" ,19))
			{

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
			if (TokenMatch(m_szFile,"MESH_ANIMATION" ,14))
			{
				
				LogWarning("Found *MESH_ANIMATION element in ASE/ASK file. "
					"Keyframe animation is not supported by Assimp, this element "
					"will be ignored");
				//SkipSection();
				continue;
			}
			if (TokenMatch(m_szFile,"MESH_WEIGHTS" ,12))
			{
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
			++m_szFile;

			// Number of bone vertices ...
			if (TokenMatch(m_szFile,"MESH_NUMVERTEX" ,14))
			{
				ParseLV4MeshLong(iNumVertices);
				continue;
			}
			// Number of bones
			if (TokenMatch(m_szFile,"MESH_NUMBONE" ,11))
			{
				ParseLV4MeshLong(iNumBones);
				continue;
			}
			// parse the list of bones
			if (TokenMatch(m_szFile,"MESH_BONE_LIST" ,14))
			{
				ParseLV4MeshBones(iNumBones,mesh);
				continue;
			}
			// parse the list of bones vertices
			if (TokenMatch(m_szFile,"MESH_BONE_VERTEX_LIST" ,21) )
			{
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
			++m_szFile;

			// Mesh bone with name ...
			if (TokenMatch(m_szFile,"MESH_BONE_NAME" ,16))
			{
				// parse an index ...
				if(SkipSpaces(&m_szFile))
				{
					unsigned int iIndex = strtol10(m_szFile,&m_szFile);
					if (iIndex >= iNumBones)
					{
						continue;
						LogWarning("Bone index is out of bounds");
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
			++m_szFile;

			// Mesh bone vertex
			if (TokenMatch(m_szFile,"MESH_BONE_VERTEX" ,16))
			{
				// read the vertex index
				unsigned int iIndex = strtol10(m_szFile,&m_szFile);
				if (iIndex >= mesh.mPositions.size())
				{
					iIndex = (unsigned int)mesh.mPositions.size()-1;
					LogWarning("Bone vertex index is out of bounds. Using the largest valid "
						"bone vertex index instead");
				}

				// --- ignored
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
			++m_szFile;

			// Vertex entry
			if (TokenMatch(m_szFile,"MESH_VERTEX" ,11))
			{
				
				aiVector3D vTemp;
				unsigned int iIndex;
				ParseLV4MeshFloatTriple(&vTemp.x,iIndex);

				if (iIndex >= iNumVertices)
				{
					LogWarning("Invalid vertex index. It will be ignored");
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
			++m_szFile;

			// Face entry
			if (TokenMatch(m_szFile,"MESH_FACE" ,9))
			{

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
			++m_szFile;

			// Vertex entry
			if (TokenMatch(m_szFile,"MESH_TVERT" ,10))
			{
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
			++m_szFile;

			// Face entry
			if (TokenMatch(m_szFile,"MESH_TFACE" ,10))
			{
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
			++m_szFile;

			// Number of texture coordinates in the mesh
			if (TokenMatch(m_szFile,"MESH_NUMTVERTEX" ,15))
			{
				ParseLV4MeshLong(iNumTVertices);
				continue;
			}
			// Number of UVWed faces in the mesh
			if (TokenMatch(m_szFile,"MESH_NUMTVFACES" ,15))
			{
				ParseLV4MeshLong(iNumTFaces);
				continue;
			}
			// mesh texture vertex list block
			if (TokenMatch(m_szFile,"MESH_TVERTLIST" ,14))
			{
				ParseLV3MeshTListBlock(iNumTVertices,mesh,iChannel);
				continue;
			}
			// mesh texture face block
			if (TokenMatch(m_szFile,"MESH_TFACELIST" ,14))
			{
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
			++m_szFile;

			// Vertex entry
			if (TokenMatch(m_szFile,"MESH_VERTCOL" ,12))
			{
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
			++m_szFile;

			// Face entry
			if (TokenMatch(m_szFile,"MESH_CFACE" ,11))
			{
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
	sMesh.mNormals.resize(sMesh.mFaces.size()*3,aiVector3D( 0.f, 0.f, 0.f ));
	
	int iDepth = 0;
	unsigned int iIndex = 0, faceIdx = 0xffffffff;

	// just smooth both vertex and face normals together, so it will still
	// work if one one of the two is missing ...
	while (true)
	{
		if ('*' == *m_szFile)
		{
			++m_szFile;
			if (0xffffffff != faceIdx && TokenMatch(m_szFile,"MESH_VERTEXNORMAL",17))
			{
				aiVector3D vNormal;
				ParseLV4MeshFloatTriple(&vNormal.x,iIndex);

				if (iIndex == sMesh.mFaces[faceIdx].mIndices[0])
				{
					iIndex = 0;
				}
				else if (iIndex != sMesh.mFaces[faceIdx].mIndices[1])
				{
					iIndex = 1;
				}
				else if (iIndex != sMesh.mFaces[faceIdx].mIndices[2])
				{
					iIndex = 2;
				}
				else
				{
					LogWarning("Normal index doesn't fit to face index");
					continue;
				}
				// We'll renormalized later
				sMesh.mNormals[faceIdx*3 + iIndex] += vNormal;
				continue;
			}
			if (TokenMatch(m_szFile,"MESH_FACENORMAL",15))
			{
				aiVector3D vNormal;
				ParseLV4MeshFloatTriple(&vNormal.x,faceIdx);

				if (iIndex >= sMesh.mFaces.size())
				{
					LogWarning("Face normal index is too large");
					faceIdx = 0xffffffff;
					continue;
				}
				sMesh.mNormals[faceIdx*3]    += vNormal;
				sMesh.mNormals[faceIdx*3 +1] += vNormal;
				sMesh.mNormals[faceIdx*3 +2] += vNormal;
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
	if (TokenMatch(m_szFile,"*MESH_SMOOTHING",15))
	{
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
			return;
		}
		m_szFile++;
	}

	if (TokenMatch(m_szFile,"*MESH_MTLID",11))
	{
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
