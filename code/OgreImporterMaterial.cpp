/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
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

//#include "boost/format.hpp"
//#include "boost/foreach.hpp"
//using namespace boost;

#include "OgreImporter.h"
#include "irrXMLWrapper.h"
#include "TinyFormatter.h"

namespace Assimp
{
namespace Ogre
{



aiMaterial* OgreImporter::LoadMaterial(const std::string MaterialName) const
{
	const aiScene* const m_CurrentScene=this->m_CurrentScene;//make sure, that we can access but not change the scene

	MaterialHelper *NewMaterial=new MaterialHelper();

	aiString ts(MaterialName.c_str());
	NewMaterial->AddProperty(&ts, AI_MATKEY_NAME);
	/*For bettetr understanding of the material parser, here is a material example file:

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


	const string MaterialFileName=m_CurrentFilename.substr(0, m_CurrentFilename.find('.'))+".material";
	DefaultLogger::get()->info("Trying to load " +MaterialFileName);

	//Read the file into memory and put it in a stringstream
	stringstream ss;
	{// after this block, the temporarly loaded data will be released
		IOStream* MatFilePtr=m_CurrentIOHandler->Open(MaterialFileName);
		if(NULL==MatFilePtr)
		{
			MatFilePtr=m_CurrentIOHandler->Open(m_MaterialLibFilename);
			if(NULL==MatFilePtr)
			{
				DefaultLogger::get()->error(m_MaterialLibFilename+" and "+MaterialFileName + " could not be opened, Material will not be loaded!");
				return NewMaterial;
			}
		}
		boost::scoped_ptr<IOStream> MaterialFile(MatFilePtr);
		vector<char> FileData(MaterialFile->FileSize());
		MaterialFile->Read(&FileData[0], MaterialFile->FileSize(), 1);
		BaseImporter::ConvertToUTF8(FileData);

		ss << &FileData[0];
	}

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
				ss >> Line;
				if(Line!="{")
					throw DeadlyImportError("empty material!");

				while(Line!="}")//read until the end of the material
				{
					//Proceed to the first technique
					ss >> Line;
					if(Line=="technique")
					{
						ss >> Line;
						if(Line!="{")
							throw DeadlyImportError("empty technique!");
						while(Line!="}")//read until the end of the technique
						{
							ss >> Line;
							if(Line=="pass")
							{
								ss >> Line;
								if(Line!="{")
									throw DeadlyImportError("empty pass!");
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
										ss >> Line;
										if(Line!="{")
											throw DeadlyImportError("empty texture unit!");
										while(Line!="}")//read until the end of the texture_unit
										{
											ss >> Line;
											if(Line=="texture")
											{
												ss >> Line;
												aiString ts(Line.c_str());
												NewMaterial->AddProperty(&ts, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
											}
										}//end of texture unit
									}
								}
							}
						}//end of technique

						
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
					}					
				}//end of material
			}
			else {} //this is the wrong material, proceed the file until we reach the next material
		}
		ss >> Line;
	}

	return NewMaterial;
}



}//namespace Ogre
}//namespace Assimp

#endif  // !! ASSIMP_BUILD_NO_OGRE_IMPORTER
