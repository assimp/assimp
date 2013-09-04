/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

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

/**
This file contains material related code. This is
spilitted up from the main file OgreImporter.cpp
to make it shorter easier to maintain.
*/
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include <vector>
#include <sstream>
using namespace std;

#include "OgreImporter.hpp"
#include "irrXMLWrapper.h"
#include "TinyFormatter.h"

namespace Assimp
{
namespace Ogre
{



aiMaterial* OgreImporter::LoadMaterial(const std::string MaterialName) const
{
	/*For better understanding of the material parser, here is a material example file:

	material Sarg
	{
		receive_shadows on
		technique
		{
			pass
			{
				ambient 0.500000 0.500000 0.500000 1.000000
				diffuse 0.640000 0.640000 0.640000 1.000000
				specular 0.500000 0.500000 0.500000 1.000000 12.500000
				emissive 0.000000 0.000000 0.000000 1.000000
				texture_unit
				{
					texture SargTextur.tga
					tex_address_mode wrap
					filtering linear linear none
				}
			}
		}
	}

	*/

	/*and here is another one:

	import * from abstract_base_passes_depth.material
	import * from abstract_base.material
	import * from mat_shadow_caster.material
	import * from mat_character_singlepass.material

	material hero/hair/caster : mat_shadow_caster_skin_areject
	{
	  set $diffuse_map "hero_hair_alpha_c.dds"
	}
	
	material hero/hair_alpha : mat_char_cns_singlepass_areject_4weights
	{
	  set $diffuse_map  "hero_hair_alpha_c.dds"
	  set $specular_map "hero_hair_alpha_s.dds"
	  set $normal_map   "hero_hair_alpha_n.dds"
	  set $light_map    "black_lightmap.dds"
  
	  set $shadow_caster_material "hero/hair/caster"
	}
	*/

	//Read the file into memory and put it in a stringstream
	stringstream ss;
	{// after this block, the temporarly loaded data will be released

		/*
		We have 3 guesses for the Material filename:
		- the Material Name
		- the Name of the mesh file
		- the DefaultMaterialLib (which you can set before importing)
		*/
		
		IOStream* MatFilePtr=m_CurrentIOHandler->Open(MaterialName+".material");
		if(NULL==MatFilePtr)
		{
			//the filename typically ends with .mesh or .mesh.xml
			const string MaterialFileName=m_CurrentFilename.substr(0, m_CurrentFilename.rfind(".mesh"))+".material";

			MatFilePtr=m_CurrentIOHandler->Open(MaterialFileName);
			if(NULL==MatFilePtr)
			{
				//try the default mat Library
				if(NULL==MatFilePtr)
				{
				
					MatFilePtr=m_CurrentIOHandler->Open(m_MaterialLibFilename);
					if(NULL==MatFilePtr)
					{
						DefaultLogger::get()->error(m_MaterialLibFilename+" and "+MaterialFileName + " could not be opened, Material will not be loaded!");
						return new aiMaterial();
					}
				}
			}
		}
		//Fill the stream
		boost::scoped_ptr<IOStream> MaterialFile(MatFilePtr);
		if(MaterialFile->FileSize()>0)
		{
			vector<char> FileData(MaterialFile->FileSize());
			MaterialFile->Read(&FileData[0], MaterialFile->FileSize(), 1);
			BaseImporter::ConvertToUTF8(FileData);

			FileData.push_back('\0');//terminate the string with zero, so that the ss can parse it correctly
			ss << &FileData[0];
		}
		else
		{
			DefaultLogger::get()->warn("Material " + MaterialName + " seams to be empty");
			return NULL;
		}
	}

	//create the material
	aiMaterial *NewMaterial=new aiMaterial();

	aiString ts(MaterialName.c_str());
	NewMaterial->AddProperty(&ts, AI_MATKEY_NAME);

	string Line;
	ss >> Line;
//	unsigned int Level=0;//Hierarchielevels in the material file, like { } blocks into another
	while(!ss.eof())
	{
		if(Line=="material")
		{
			ss >> Line;
			if(Line==MaterialName)//Load the next material
			{
				string RestOfLine;
				getline(ss, RestOfLine);//ignore the rest of the line
				ss >> Line;

				if(Line!="{")
				{
					DefaultLogger::get()->warn("empyt material!");
					return NULL;
				}

				while(Line!="}")//read until the end of the material
				{
					//Proceed to the first technique
					ss >> Line;
					if(Line=="technique")
					{
						ReadTechnique(ss, NewMaterial);
					}

					DefaultLogger::get()->info(Line);
					//read informations from a custom material:
					if(Line=="set")
					{
						ss >> Line;
						if(Line=="$specular")//todo load this values:
						{
						}
						if(Line=="$diffuse")
						{
						}
						if(Line=="$ambient")
						{
						}
						if(Line=="$colormap")
						{
							ss >> Line;
							aiString ts(Line.c_str());
							NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
						}
						if(Line=="$normalmap")
						{
							ss >> Line;
							aiString ts(Line.c_str());
							NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0));
						}
						
						if(Line=="$shininess_strength")
						{
							ss >> Line;
							float Shininess=fast_atof(Line.c_str());
							NewMaterial->AddProperty(&Shininess, 1, AI_MATKEY_SHININESS_STRENGTH);
						}

						if(Line=="$shininess_exponent")
						{
							ss >> Line;
							float Shininess=fast_atof(Line.c_str());
							NewMaterial->AddProperty(&Shininess, 1, AI_MATKEY_SHININESS);
						}

						//Properties from Venetica:
						if(Line=="$diffuse_map")
						{
							ss >> Line;
							if(Line[0]=='"')// "file" -> file
								Line=Line.substr(1, Line.size()-2);
							aiString ts(Line.c_str());
							NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
						}
						if(Line=="$specular_map")
						{
							ss >> Line;
							if(Line[0]=='"')// "file" -> file
								Line=Line.substr(1, Line.size()-2);
							aiString ts(Line.c_str());
							NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_SHININESS, 0));
						}
						if(Line=="$normal_map")
						{
							ss >> Line;
							if(Line[0]=='"')// "file" -> file
								Line=Line.substr(1, Line.size()-2);
							aiString ts(Line.c_str());
							NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0));
						}
						if(Line=="$light_map")
						{
							ss >> Line;
							if(Line[0]=='"')// "file" -> file
								Line=Line.substr(1, Line.size()-2);
							aiString ts(Line.c_str());
							NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_LIGHTMAP, 0));
						}
					}					
				}//end of material
			}
			else {} //this is the wrong material, proceed the file until we reach the next material
		}
		ss >> Line;
	}

	return NewMaterial;
}

void OgreImporter::ReadTechnique(stringstream &ss, aiMaterial* NewMaterial) const
{
	unsigned int CurrentDiffuseTextureId=0;
	unsigned int CurrentSpecularTextureId=0;
	unsigned int CurrentNormalTextureId=0;
	unsigned int CurrentLightTextureId=0;


	string RestOfLine;
	getline(ss, RestOfLine);//ignore the rest of the line

	string Line;
	ss >> Line;
	if(Line!="{")
	{
		DefaultLogger::get()->warn("empty technique!");
		return;
	}
	while(Line!="}")//read until the end of the technique
	{
		ss >> Line;
		if(Line=="pass")
		{
			getline(ss, RestOfLine);//ignore the rest of the line

			ss >> Line;
			if(Line!="{")
			{
				DefaultLogger::get()->warn("empty pass!");
				return;
			}
			while(Line!="}")//read until the end of the pass
			{
				ss >> Line;
				if(Line=="ambient")
				{
					float r,g,b;
					ss >> r >> g >> b;
					const aiColor3D Color(r,g,b);
					NewMaterial->AddProperty(&Color, 1, AI_MATKEY_COLOR_AMBIENT);
				}
				else if(Line=="diffuse")
				{
					float r,g,b;
					ss >> r >> g >> b;
					const aiColor3D Color(r,g,b);
					NewMaterial->AddProperty(&Color, 1, AI_MATKEY_COLOR_DIFFUSE);
				}
				else if(Line=="specular")
				{
					float r,g,b;
					ss >> r >> g >> b;
					const aiColor3D Color(r,g,b);
					NewMaterial->AddProperty(&Color, 1, AI_MATKEY_COLOR_SPECULAR);
				}
				else if(Line=="emmisive")
				{
					float r,g,b;
					ss >> r >> g >> b;
					const aiColor3D Color(r,g,b);
					NewMaterial->AddProperty(&Color, 1, AI_MATKEY_COLOR_EMISSIVE);
				}
				else if(Line=="texture_unit")
				{
					getline(ss, RestOfLine);//ignore the rest of the line

					std::string TextureName;
					int TextureType=-1;
					int UvSet=0;

					ss >> Line;
					if(Line!="{")
						throw DeadlyImportError("empty texture unit!");
					while(Line!="}")//read until the end of the texture_unit
					{
						ss >> Line;
						if(Line=="texture")
						{
							ss >> Line;
							TextureName=Line;

							if(m_TextureTypeFromFilename)
							{
								if(Line.find("_n.")!=string::npos)// Normalmap
								{
									TextureType=aiTextureType_NORMALS;
								}
								else if(Line.find("_s.")!=string::npos)// Specularmap
								{
									TextureType=aiTextureType_SPECULAR;
								}
								else if(Line.find("_l.")!=string::npos)// Lightmap
								{
									TextureType=aiTextureType_LIGHTMAP;
								}
								else// colormap
								{
									TextureType=aiTextureType_DIFFUSE;
								}
							}
							else
							{
								TextureType=aiTextureType_DIFFUSE;
							}
						}
						else if(Line=="tex_coord_set")
						{
							ss >> UvSet;
						}
						else if(Line=="colour_op")//TODO implement this
						{
							/*
							ss >> Line;
							if("replace"==Line)//I don't think, assimp has something for this...
							{
							}
							else if("modulate"==Line)
							{
								//TODO: set value
								//NewMaterial->AddProperty(aiTextureOp_Multiply)
							}
							*/
						}
						
					}//end of texture unit
					Line="";//clear the } that would end the outer loop

					//give the texture to assimp:
					
					aiString ts(TextureName.c_str());
					switch(TextureType)
					{
					case aiTextureType_DIFFUSE:
						NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, CurrentDiffuseTextureId));
						NewMaterial->AddProperty(&UvSet, 1, AI_MATKEY_UVWSRC(0, CurrentDiffuseTextureId));
						CurrentDiffuseTextureId++;
						break;
					case aiTextureType_NORMALS:
						NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_NORMALS, CurrentNormalTextureId));
						NewMaterial->AddProperty(&UvSet, 1, AI_MATKEY_UVWSRC(0, CurrentNormalTextureId));
						CurrentNormalTextureId++;
						break;
					case aiTextureType_SPECULAR:
						NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, CurrentSpecularTextureId));
						NewMaterial->AddProperty(&UvSet, 1, AI_MATKEY_UVWSRC(0, CurrentSpecularTextureId));
						CurrentSpecularTextureId++;
						break;
					case aiTextureType_LIGHTMAP:
						NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_LIGHTMAP, CurrentLightTextureId));
						NewMaterial->AddProperty(&UvSet, 1, AI_MATKEY_UVWSRC(0, CurrentLightTextureId));
						CurrentLightTextureId++;
						break;
					default:
						DefaultLogger::get()->warn("Invalid Texture Type!");
						break;
					}
				}
			}
			Line="";//clear the } that would end the outer loop
		}
	}//end of technique
}


}//namespace Ogre
}//namespace Assimp

#endif  // !! ASSIMP_BUILD_NO_OGRE_IMPORTER
