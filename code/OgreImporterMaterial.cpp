/*
This file contains material related code. This is
spilitted up from the main file OgreImporter.cpp
to make it shorter and better amintainable.
*/
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include <vector>
#include <sstream>
using namespace std;

//#include "boost/format.hpp"
//#include "boost/foreach.hpp"
using namespace boost;

#include "OgreImporter.h"
#include "irrXMLWrapper.h"


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


	string MaterialFileName=m_CurrentFilename.substr(0, m_CurrentFilename.find('.'))+".material";
	DefaultLogger::get()->info(str(format("Trying to load %1%") % MaterialFileName));

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
		scoped_ptr<IOStream> MaterialFile(MatFilePtr);
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
