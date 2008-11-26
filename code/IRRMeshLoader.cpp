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

/** @file Implementation of the IrrMesh importer class */

#include "AssimpPCH.h"


#include "IRRMeshLoader.h"
#include "ParsingUtils.h"
#include "fast_atof.h"

using namespace Assimp;

/**** AT FIRST: IrrlightBase, base class for IrrMesh and Irr *******/

// ------------------------------------------------------------------------------------------------
// read a property in hexadecimal format (i.e. ffffffff)
void IrrlichtBase::ReadHexProperty    (HexProperty&    out)
{
	for (int i = 0; i < reader->getAttributeCount();++i)
	{
		if (!ASSIMP_stricmp(reader->getAttributeName(i),"name"))
		{
			out.name = std::string( reader->getAttributeValue(i) );
		}
		else if (!ASSIMP_stricmp(reader->getAttributeName(i),"value"))
		{
			// parse the hexadecimal value
			out.value = strtol16(reader->getAttributeValue(i));
		}
	}
}

// ------------------------------------------------------------------------------------------------
// read a decimal property
void IrrlichtBase::ReadIntProperty    (IntProperty&    out)
{
	for (int i = 0; i < reader->getAttributeCount();++i)
	{
		if (!ASSIMP_stricmp(reader->getAttributeName(i),"name"))
		{
			out.name = std::string( reader->getAttributeValue(i) );
		}
		else if (!ASSIMP_stricmp(reader->getAttributeName(i),"value"))
		{
			// parse the ecimal value
			out.value = strtol10s(reader->getAttributeValue(i));
		}
	}
}

// ------------------------------------------------------------------------------------------------
// read a string property
void IrrlichtBase::ReadStringProperty (StringProperty& out)
{
	for (int i = 0; i < reader->getAttributeCount();++i)
	{
		if (!ASSIMP_stricmp(reader->getAttributeName(i),"name"))
		{
			out.name = std::string( reader->getAttributeValue(i) );
		}
		else if (!ASSIMP_stricmp(reader->getAttributeName(i),"value"))
		{
			// simple copy the string
			out.value = std::string (reader->getAttributeValue(i));
		}
	}
}

// ------------------------------------------------------------------------------------------------
// read a boolean property
void IrrlichtBase::ReadBoolProperty   (BoolProperty&   out)
{
	for (int i = 0; i < reader->getAttributeCount();++i)
	{
		if (!ASSIMP_stricmp(reader->getAttributeName(i),"name"))
		{
			out.name = std::string( reader->getAttributeValue(i) );
		}
		else if (!ASSIMP_stricmp(reader->getAttributeName(i),"value"))
		{
			// true or false, case insensitive
			out.value = (ASSIMP_stricmp( reader->getAttributeValue(i), 
				"true") ? false : true);
		}
	}
}

// ------------------------------------------------------------------------------------------------
// read a float property
void IrrlichtBase::ReadFloatProperty  (FloatProperty&  out)
{
	for (int i = 0; i < reader->getAttributeCount();++i)
	{
		if (!ASSIMP_stricmp(reader->getAttributeName(i),"name"))
		{
			out.name = std::string( reader->getAttributeValue(i) );
		}
		else if (!ASSIMP_stricmp(reader->getAttributeName(i),"value"))
		{
			// just parse the float
			out.value = fast_atof( reader->getAttributeValue(i) );
		}
	}
}

// ------------------------------------------------------------------------------------------------
// read a vector property
void IrrlichtBase::ReadVectorProperty  (VectorProperty&  out)
{
	for (int i = 0; i < reader->getAttributeCount();++i)
	{
		if (!ASSIMP_stricmp(reader->getAttributeName(i),"name"))
		{
			out.name = std::string( reader->getAttributeValue(i) );
		}
		else if (!ASSIMP_stricmp(reader->getAttributeName(i),"value"))
		{
			// three floats, separated with commas
			const char* ptr = reader->getAttributeValue(i);

			SkipSpaces(&ptr);
			ptr = fast_atof_move( ptr,(float&)out.value.x );
			SkipSpaces(&ptr);
			if (',' != *ptr)
			{
				DefaultLogger::get()->error("IRR(MESH): Expected comma in vector definition");
			}
			else SkipSpaces(ptr+1,&ptr);
			ptr = fast_atof_move( ptr,(float&)out.value.y );
			SkipSpaces(&ptr);
			if (',' != *ptr)
			{
				DefaultLogger::get()->error("IRR(MESH): Expected comma in vector definition");
			}
			else SkipSpaces(ptr+1,&ptr);
			ptr = fast_atof_move( ptr,(float&)out.value.z );
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ColorFromARGBPacked(uint32_t in, aiColor4D& clr)
{
	clr.a = ((in >> 24) & 0xff) / 255.f;
	clr.r = ((in >> 16) & 0xff) / 255.f;
	clr.g = ((in >>  8) & 0xff) / 255.f;
	clr.b = ((in      ) & 0xff) / 255.f;
}

// ------------------------------------------------------------------------------------------------
int ConvertMappingMode(const std::string& mode)
{
	if (mode == "texture_clamp_repeat")
	{
		return aiTextureMapMode_Wrap;
	}
	else if (mode == "texture_clamp_mirror")
		return aiTextureMapMode_Mirror;

	return aiTextureMapMode_Clamp;
}

// ------------------------------------------------------------------------------------------------
// Parse a material from the XML file
aiMaterial* IrrlichtBase::ParseMaterial(unsigned int& matFlags)
{
	MaterialHelper* mat = new MaterialHelper();
	aiColor4D clr;
	aiString s;

	matFlags = 0; // zero output flags
	int cnt  = 0; // number of used texture channels

	// Continue reading from the file
	while (reader->read())
	{
		switch (reader->getNodeType())
		{
		case EXN_ELEMENT:

			// Hex properties
			if (!ASSIMP_stricmp(reader->getNodeName(),"color"))
			{
				HexProperty prop;
				ReadHexProperty(prop);
				if (prop.name == "Diffuse")
				{
					ColorFromARGBPacked(prop.value,clr);
					mat->AddProperty(&clr,1,AI_MATKEY_COLOR_DIFFUSE);
				}
				else if (prop.name == "Ambient")
				{
					ColorFromARGBPacked(prop.value,clr);
					mat->AddProperty(&clr,1,AI_MATKEY_COLOR_AMBIENT);
				}
				else if (prop.name == "Specular")
				{
					ColorFromARGBPacked(prop.value,clr);
					mat->AddProperty(&clr,1,AI_MATKEY_COLOR_SPECULAR);
				}

				// NOTE: The 'emissive' property causes problems. It is
				// often != 0, even if there is obviously no light
				// emitted by the described surface. In fact I think
				// IRRLICHT ignores this property, too.
#if 0
				else if (prop.name == "Emissive")
				{
					ColorFromARGBPacked(prop.value,clr);
					mat->AddProperty(&clr,1,AI_MATKEY_COLOR_EMISSIVE);
				}
#endif
			}
			// Float properties
			else if (!ASSIMP_stricmp(reader->getNodeName(),"float"))
			{
				FloatProperty prop;
				ReadFloatProperty(prop);
				if (prop.name == "Shininess")
				{
					mat->AddProperty(&prop.value,1,AI_MATKEY_SHININESS);
				}
			}
			// Bool properties
			else if (!ASSIMP_stricmp(reader->getNodeName(),"bool"))
			{
				BoolProperty prop;
				ReadBoolProperty(prop);
				if (prop.name == "Wireframe")
				{
					int val = (prop.value ? true : false);
					mat->AddProperty(&val,1,AI_MATKEY_ENABLE_WIREFRAME);
				}
				else if (prop.name == "GouraudShading")
				{
					int val = (prop.value ? aiShadingMode_Gouraud 
						: aiShadingMode_NoShading);
					mat->AddProperty(&val,1,AI_MATKEY_SHADING_MODEL);
				}
			}
			// String properties - textures and texture related properties
			else if (!ASSIMP_stricmp(reader->getNodeName(),"texture") ||
				     !ASSIMP_stricmp(reader->getNodeName(),"enum"))
			{
				StringProperty prop;
				ReadStringProperty(prop);
				if (prop.value.length())
				{
					// material type (shader)
					if (prop.name == "Type")
					{
						if (prop.value == "trans_vertex_alpha")
						{
							matFlags = AI_IRRMESH_MAT_trans_vertex_alpha;
						}
						else if (prop.value == "lightmap")
						{
							matFlags = AI_IRRMESH_MAT_lightmap;
						}
						else if (prop.value == "solid_2layer")
						{
							matFlags = AI_IRRMESH_MAT_solid_2layer;
						}
						else if (prop.value == "lightmap_m2")
						{
							matFlags = AI_IRRMESH_MAT_lightmap_m2;
						}
						else if (prop.value == "lightmap_m4")
						{
							matFlags = AI_IRRMESH_MAT_lightmap_m4;
						}
						else if (prop.value == "lightmap_light")
						{
							matFlags = AI_IRRMESH_MAT_lightmap_light;
						}
						else if (prop.value == "lightmap_light_m2")
						{
							matFlags = AI_IRRMESH_MAT_lightmap_light_m2;
						}
						else if (prop.value == "lightmap_light_m4")
						{
							matFlags = AI_IRRMESH_MAT_lightmap_light_m4;
						}
						else if (prop.value == "lightmap_add")
						{
							matFlags = AI_IRRMESH_MAT_lightmap_add;
						}
						// Normal and parallax maps are treated equally
						else if (prop.value == "normalmap_solid" ||
							prop.value == "parallaxmap_solid")
						{
							matFlags = AI_IRRMESH_MAT_normalmap_solid;
						}
						else if (prop.value == "normalmap_trans_vertex_alpha" ||
							prop.value == "parallaxmap_trans_vertex_alpha")
						{
							matFlags = AI_IRRMESH_MAT_normalmap_tva;
						}
						else if (prop.value == "normalmap_trans_add" ||
							prop.value == "parallaxmap_trans_add")
						{
							matFlags = AI_IRRMESH_MAT_normalmap_ta;
						}
					}

					// Up to 4 texture channels are supported
					else if (prop.name == "Texture1")
					{
						// Always accept the primary texture channel
						++cnt;
						s.Set(prop.value);
						mat->AddProperty(&s,AI_MATKEY_TEXTURE_DIFFUSE(0));
					}
					else if (prop.name == "Texture2")
					{
						// 2-layer material lightmapped?
						if (matFlags & (AI_IRRMESH_MAT_solid_2layer | AI_IRRMESH_MAT_lightmap))
						{
							++cnt;
							s.Set(prop.value);
							mat->AddProperty(&s,AI_MATKEY_TEXTURE_DIFFUSE(1));

							// set the corresponding material flag
							matFlags |= AI_IRRMESH_EXTRA_2ND_TEXTURE;
						}
						// alternatively: normal or parallax mapping
						else if (matFlags & AI_IRRMESH_MAT_normalmap_solid)
						{
							++cnt;
							s.Set(prop.value);
							mat->AddProperty(&s,AI_MATKEY_TEXTURE_NORMALS(1));

							// set the corresponding material flag
							matFlags |= AI_IRRMESH_EXTRA_2ND_TEXTURE;
						}
					}
					else if (prop.name == "Texture3")
					{
						// We don't process the third texture channel as Irrlicht
						// does not seem to use it.
#if 0
						++cnt;
						s.Set(prop.value);
						mat->AddProperty(&s,AI_MATKEY_TEXTURE_DIFFUSE(2));
#endif
					}
					else if (prop.name == "Texture4" )
					{
						// We don't process the fourth texture channel as Irrlicht
						// does not seem to use it.
#if 0
						++cnt;
						s.Set(prop.value);
						mat->AddProperty(&s,AI_MATKEY_TEXTURE_DIFFUSE(3));
#endif
					}

					// Texture mapping options
					if (prop.name == "TextureWrap1" && cnt >= 1)
					{
						int map = ConvertMappingMode(prop.value);
						mat->AddProperty(&map,1,AI_MATKEY_MAPPINGMODE_U_DIFFUSE(0));
						mat->AddProperty(&map,1,AI_MATKEY_MAPPINGMODE_V_DIFFUSE(0));
					}
					else if (prop.name == "TextureWrap2" && cnt >= 2)
					{
						int map = ConvertMappingMode(prop.value);
						mat->AddProperty(&map,1,AI_MATKEY_MAPPINGMODE_U_DIFFUSE(1));
						mat->AddProperty(&map,1,AI_MATKEY_MAPPINGMODE_V_DIFFUSE(1));	
					}
					else if (prop.name == "TextureWrap3" && cnt >= 3)
					{
						int map = ConvertMappingMode(prop.value);
						mat->AddProperty(&map,1,AI_MATKEY_MAPPINGMODE_U_DIFFUSE(2));
						mat->AddProperty(&map,1,AI_MATKEY_MAPPINGMODE_V_DIFFUSE(2));
					}
					else if (prop.name == "TextureWrap4" && cnt >= 4)
					{
						int map = ConvertMappingMode(prop.value);
						mat->AddProperty(&map,1,AI_MATKEY_MAPPINGMODE_U_DIFFUSE(3));
						mat->AddProperty(&map,1,AI_MATKEY_MAPPINGMODE_V_DIFFUSE(3));
					}
				}
			}
			break;
			case EXN_ELEMENT_END:

				/* Assume there are no further nested nodes in <material> elements
				 */
				if (/* IRRMESH */ !ASSIMP_stricmp(reader->getNodeName(),"material") ||
					/* IRR     */ !ASSIMP_stricmp(reader->getNodeName(),"attributes"))
				{
					// Now process lightmapping flags
					// We should have at least one texture, however
					// if there are multiple textures we assign the
					// lightmap settings to the last texture.
					if (cnt && matFlags & AI_IRRMESH_MAT_lightmap)
					{
						float f = 1.f;

						// Additive lightmap?
						int op = (matFlags & AI_IRRMESH_MAT_lightmap_add
							? aiTextureOp_Add : aiTextureOp_Multiply);

						// Handle Irrlicht's lightmapping scaling factor
						if (matFlags & AI_IRRMESH_MAT_lightmap_m2 ||
							matFlags & AI_IRRMESH_MAT_lightmap_light_m2)
						{
							f = 2.f;
						}
						else if (matFlags & AI_IRRMESH_MAT_lightmap_m4 ||
							matFlags & AI_IRRMESH_MAT_lightmap_light_m4)
						{
							f = 4.f;
						}
						mat->AddProperty( &f, 1, AI_MATKEY_TEXBLEND_DIFFUSE(cnt-1));
						mat->AddProperty( &op,1, AI_MATKEY_TEXOP_DIFFUSE(cnt-1));
					}

					return mat;
				}
			default:

				// GCC complains here ...
				break;
		}
	}
	DefaultLogger::get()->error("IRRMESH: Unexpected end of file. Material is not complete");
	return mat;
}


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
IRRMeshImporter::IRRMeshImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
IRRMeshImporter::~IRRMeshImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool IRRMeshImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	/* NOTE: A simple check for the file extension is not enough
	 * here. Irrmesh and irr are easy, but xml is too generic
	 * and could be collada, too. So we need to open the file and
	 * search for typical tokens.
	 */

	std::string::size_type pos = pFile.find_last_of('.');

	// no file extension - can't read
	if( pos == std::string::npos)
		return false;

	std::string extension = pFile.substr( pos);
	for (std::string::iterator i = extension.begin(); i != extension.end();++i)
		*i = ::tolower(*i);

	if (extension == ".irrmesh")return true;
	else if (extension == ".xml")
	{
		/*  If CanRead() is called to check whether the loader
		 *  supports a specific file extension in general we
		 *  must return true here.
		 */
		if (!pIOHandler)return true;
		const char* tokens[] = {"irrmesh"};
		return SearchFileHeaderForToken(pIOHandler,pFile,tokens,1);
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void IRRMeshImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open IRRMESH file " + pFile + "");

	// Construct the irrXML parser
	CIrrXML_IOStreamReader st(file.get());
	reader = createIrrXMLReader((IFileReadCallBack*) &st);

	// final data
	std::vector<aiMaterial*> materials;
	std::vector<aiMesh*>     meshes;
	materials.reserve (5);
	meshes.reserve    (5);

	// temporary data - current mesh buffer
	aiMaterial* curMat	= NULL;
	aiMesh* curMesh		= NULL;
	unsigned int curMatFlags;

	std::vector<aiVector3D> curVertices,curNormals,curTangents,curBitangents;
	std::vector<aiColor4D>  curColors;
	std::vector<aiVector3D> curUVs,curUV2s;

	// some temporary variables
	int textMeaning = 0;
	int vertexFormat = 0; // 0 = normal; 1 = 2 tcoords, 2 = tangents
	bool useColors = false;
	bool needLightMap = false;

	// Parse the XML file
	while (reader->read())
	{
		switch (reader->getNodeType())
		{
		case EXN_ELEMENT:
			
			if (!ASSIMP_stricmp(reader->getNodeName(),"buffer") && (curMat || curMesh))
			{
				// end of previous buffer. A material and a mesh should be there
				if ( !curMat || !curMesh)
				{
					DefaultLogger::get()->error("IRRMESH: A buffer must contain a mesh and a material");
					delete curMat;
					delete curMesh;
				}
				else
				{
					materials.push_back(curMat); 
					meshes.push_back(curMesh);  
				}
				curMat  = NULL;
				curMesh = NULL;

				curVertices.clear();
				curColors.clear();
				curNormals.clear();
				curUV2s.clear();
				curUVs.clear();
				curTangents.clear();
				curBitangents.clear();
			}
			

			if (!ASSIMP_stricmp(reader->getNodeName(),"material"))
			{
				if (curMat)
				{
					DefaultLogger::get()->warn("IRRMESH: Only one material description per buffer, please");
					delete curMat;curMat = NULL;
				}
				curMat = ParseMaterial(curMatFlags);
			}
			/* no else here! */ if (!ASSIMP_stricmp(reader->getNodeName(),"vertices"))
			{
				int num = reader->getAttributeValueAsInt("vertexCount");

				if (!num)
				{
					// This is possible ... remove the mesh from the list
					// and skip further reading

					DefaultLogger::get()->warn("IRRMESH: Found mesh with zero vertices");

					delete curMat;curMat = NULL;

					curMesh = NULL;
					textMeaning = 0;
					continue;
				}

				curVertices.reserve (num);
				curNormals.reserve  (num);
				curColors.reserve   (num);
				curUVs.reserve      (num);

				// Determine the file format
				const char* t = reader->getAttributeValueSafe("type");
				if (!ASSIMP_stricmp("2tcoords", t))
				{
					curUV2s.reserve (num);
					vertexFormat = 1;

					if (curMatFlags & AI_IRRMESH_EXTRA_2ND_TEXTURE)
					{
						// *********************************************************
						// We have a second texture! So use this UV channel
						// for it. The 2nd texture can be either a normal
						// texture (solid_2layer or lightmap_xxx) or a normal 
						// map (normal_..., parallax_...)
						// *********************************************************
						int idx = 1;
						MaterialHelper* mat = ( MaterialHelper* ) curMat;

						if (curMatFlags & (AI_IRRMESH_MAT_solid_2layer | AI_IRRMESH_MAT_lightmap))
						{
							mat->AddProperty(&idx,1,AI_MATKEY_UVWSRC_DIFFUSE(0));
						}
						else if (curMatFlags & AI_IRRMESH_MAT_normalmap_solid)
						{
							mat->AddProperty(&idx,1,AI_MATKEY_UVWSRC_NORMALS(0));
						}
					}
				}
				else if (!ASSIMP_stricmp("tangents", t))
				{
					curTangents.reserve (num);
					curBitangents.reserve (num);
					vertexFormat = 2;
				}
				else if (ASSIMP_stricmp("standard", t))
				{
					delete curMat;
					DefaultLogger::get()->warn("IRRMESH: Unknown vertex format");
				}
				else vertexFormat = 0;
				textMeaning = 1;
			}
			else if (!ASSIMP_stricmp(reader->getNodeName(),"indices"))
			{
				if (curVertices.empty() && curMat)
				{
					delete curMat;
					throw new ImportErrorException("IRRMESH: indices must come after vertices");
				}

				textMeaning = 2;

				// start a new mesh
				curMesh = new aiMesh();

				// allocate storage for all faces
				curMesh->mNumVertices = reader->getAttributeValueAsInt("indexCount");
				if (!curMesh->mNumVertices)
				{
					// This is possible ... remove the mesh from the list
					// and skip further reading

					DefaultLogger::get()->warn("IRRMESH: Found mesh with zero indices");

					// mesh - away
					delete curMesh; curMesh = NULL;

					// material - away
					delete curMat;curMat = NULL;

					textMeaning = 0;
					continue;
				}

				if (curMesh->mNumVertices % 3)
				{
					DefaultLogger::get()->warn("IRRMESH: Number if indices isn't divisible by 3");
				}

				curMesh->mNumFaces = curMesh->mNumVertices / 3;
				curMesh->mFaces = new aiFace[curMesh->mNumFaces];

				// setup some members
				curMesh->mMaterialIndex = (unsigned int)materials.size();
				curMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

				// allocate storage for all vertices
				curMesh->mVertices = new aiVector3D[curMesh->mNumVertices];

				if (curNormals.size() == curVertices.size())
				{
					curMesh->mNormals = new aiVector3D[curMesh->mNumVertices];
				}
				if (curTangents.size() == curVertices.size())
				{
					curMesh->mTangents = new aiVector3D[curMesh->mNumVertices];
				}
				if (curBitangents.size() == curVertices.size())
				{
					curMesh->mBitangents = new aiVector3D[curMesh->mNumVertices];
				}
				if (curColors.size() == curVertices.size() && useColors)
				{
					curMesh->mColors[0] = new aiColor4D[curMesh->mNumVertices];
				}
				if (curUVs.size() == curVertices.size())
				{
					curMesh->mTextureCoords[0] = new aiVector3D[curMesh->mNumVertices];
				}
				if (curUV2s.size() == curVertices.size())
				{
					curMesh->mTextureCoords[1] = new aiVector3D[curMesh->mNumVertices];
				}
			}
			break;

		case EXN_TEXT:
			{
			const char* sz = reader->getNodeData();
			if (textMeaning == 1)
			{
				textMeaning = 0;

				// read vertices
				do
				{
					SkipSpacesAndLineEnd(&sz);
					aiVector3D temp;aiColor4D c;

					// Read the vertex position
					sz = fast_atof_move(sz,(float&)temp.x);
					SkipSpaces(&sz);

					sz = fast_atof_move(sz,(float&)temp.y);
					SkipSpaces(&sz);

					sz = fast_atof_move(sz,(float&)temp.z);
					SkipSpaces(&sz);
					curVertices.push_back(temp);

					// Read the vertex normals
					sz = fast_atof_move(sz,(float&)temp.x);
					SkipSpaces(&sz);

					sz = fast_atof_move(sz,(float&)temp.y);
					SkipSpaces(&sz);

					sz = fast_atof_move(sz,(float&)temp.z);
					SkipSpaces(&sz);
					curNormals.push_back(temp);

					// read the vertex colors
					uint32_t clr = strtol16(sz,&sz);	
					ColorFromARGBPacked(clr,c);

					if (!curColors.empty() && c != *(curColors.end()-1))
						useColors = true;

					curColors.push_back(c);
					SkipSpaces(&sz);


					// read the first UV coordinate set
					sz = fast_atof_move(sz,(float&)temp.x);
					SkipSpaces(&sz);

					sz = fast_atof_move(sz,(float&)temp.y);
					SkipSpaces(&sz);
					temp.z = 0.f;
					temp.y = 1.f - temp.y;  // DX to OGL
					curUVs.push_back(temp);

					// read the (optional) second UV coordinate set
					if (vertexFormat == 1)
					{
						sz = fast_atof_move(sz,(float&)temp.x);
						SkipSpaces(&sz);

						sz = fast_atof_move(sz,(float&)temp.y);
						temp.y = 1.f - temp.y; // DX to OGL
						curUV2s.push_back(temp);
					}
					// read optional tangent and bitangent vectors
					else if (vertexFormat == 2)
					{
						// tangents
						sz = fast_atof_move(sz,(float&)temp.x);
						SkipSpaces(&sz);

						sz = fast_atof_move(sz,(float&)temp.z);
						SkipSpaces(&sz);

						sz = fast_atof_move(sz,(float&)temp.y);
						SkipSpaces(&sz);
						temp.y *= -1.0f;
						curTangents.push_back(temp);

						// bitangents
						sz = fast_atof_move(sz,(float&)temp.x);
						SkipSpaces(&sz);

						sz = fast_atof_move(sz,(float&)temp.z);
						SkipSpaces(&sz);

						sz = fast_atof_move(sz,(float&)temp.y);
						SkipSpaces(&sz);
						temp.y *= -1.0f;
						curBitangents.push_back(temp);
					}
				}

				/* IMPORTANT: We assume that each vertex is specified in one
				   line. So we can skip the rest of the line - unknown vertex
				   elements are ignored.
				 */

				while (SkipLine(&sz));
			}
			else if (textMeaning == 2)
			{
				textMeaning = 0;

				// read indices
				aiFace* curFace = curMesh->mFaces;
				aiFace* const faceEnd = curMesh->mFaces  + curMesh->mNumFaces;

				aiVector3D* pcV  = curMesh->mVertices;
				aiVector3D* pcN  = curMesh->mNormals;
				aiVector3D* pcT  = curMesh->mTangents;
				aiVector3D* pcB  = curMesh->mBitangents;
				aiColor4D* pcC0  = curMesh->mColors[0];
				aiVector3D* pcT0 = curMesh->mTextureCoords[0];
				aiVector3D* pcT1 = curMesh->mTextureCoords[1];

				unsigned int curIdx = 0;
				unsigned int total = 0;
				while(SkipSpacesAndLineEnd(&sz))
				{
					if (curFace >= faceEnd)
					{
						DefaultLogger::get()->error("IRRMESH: Too many indices");
						break;
					}
					if (!curIdx)
					{
						curFace->mNumIndices = 3;
						curFace->mIndices = new unsigned int[3];
					}

					unsigned int idx = strtol10(sz,&sz);
					if (idx >= curVertices.size())
					{
						DefaultLogger::get()->error("IRRMESH: Index out of range");
						idx = 0;
					}

					curFace->mIndices[curIdx] = total++;

					*pcV++ = curVertices[idx];
					if (pcN)*pcN++ = curNormals[idx];
					if (pcT)*pcT++ = curTangents[idx];
					if (pcB)*pcB++ = curBitangents[idx];
					if (pcC0)*pcC0++ = curColors[idx];
					if (pcT0)*pcT0++ = curUVs[idx];
					if (pcT1)*pcT1++ = curUV2s[idx];

					if (++curIdx == 3)
					{
						++curFace;
						curIdx = 0;
					}
				}

				if (curFace != faceEnd)
					DefaultLogger::get()->error("IRRMESH: Not enough indices");

				// Finish processing the mesh - do some small material workarounds
				if (curMatFlags & AI_IRRMESH_MAT_trans_vertex_alpha && !useColors)
				{
					// Take the opacity value of the current material
					// from the common vertex color alpha
					MaterialHelper* mat = (MaterialHelper*)curMat;
					mat->AddProperty(&curColors[0].a,1,AI_MATKEY_OPACITY);
				}
			}}
			break;

			default:

				// GCC complains here ...
				break;

		};
	}

	// End of the last buffer. A material and a mesh should be there
	if (curMat || curMesh)
	{
		if ( !curMat || !curMesh)
		{
			DefaultLogger::get()->error("IRRMESH: A buffer must contain a mesh and a material");
			delete curMat;
			delete curMesh;
		}
		else
		{
			materials.push_back(curMat); 
			meshes.push_back(curMesh);  
		}
	}

	if (materials.empty())
		throw new ImportErrorException("IRRMESH: Unable to read a mesh from this file");


	// now generate the output scene
	pScene->mNumMeshes = (unsigned int)meshes.size();
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
	{
		pScene->mMeshes[i] = meshes[i];

		// clean this value ...
		pScene->mMeshes[i]->mNumUVComponents[3] = 0;
	}

	pScene->mNumMaterials = (unsigned int)materials.size();
	pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials];
	::memcpy(pScene->mMaterials,&materials[0],sizeof(void*)*pScene->mNumMaterials);

	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mName.Set("<IRRMesh>");
	pScene->mRootNode->mNumMeshes = pScene->mNumMeshes;
	pScene->mRootNode->mMeshes = new unsigned int[pScene->mNumMeshes];

	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		pScene->mRootNode->mMeshes[i] = i;

	// transformation matrix to convert from IRRMESH to ASSIMP coordinates
	pScene->mRootNode->mTransformation *= aiMatrix4x4(
		1.0f, 0.0f, 0.0f, 0.f, 0.0f, 0.0f, -1.0f, 0.f, 0.0f, 1.0f, 0.0f, 0.f, 0.f, 0.f, 0.f, 1.f);

	delete reader;
	AI_DEBUG_INVALIDATE_PTR(reader);
}