#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include <vector>
#include <sstream>
using namespace std;

#include "boost/format.hpp"
using namespace boost;

#include "OgreImporter.h"
#include "irrXMLWrapper.h"



namespace Assimp
{
namespace Ogre
{


bool OgreImporter::CanRead(const std::string &pFile, Assimp::IOSystem *pIOHandler, bool checkSig) const
{
	if(!checkSig)//Check File Extension
	{
		std::string extension("mesh.xml");
		int l=extension.length();
		return pFile.substr(pFile.length()-l, l)==extension;
	}
	else//Check file Header
	{
		const char* tokens[] = {"<mesh>"};
		return BaseImporter::SearchFileHeaderForToken(pIOHandler, pFile, tokens, 1);
	}
}



void OgreImporter::InternReadFile(const std::string &pFile, aiScene *pScene, Assimp::IOSystem *pIOHandler)
{
	m_CurrentFilename=pFile;
	m_CurrentIOHandler=pIOHandler;
	m_CurrentScene=pScene;

	//Open the File:
	boost::scoped_ptr<IOStream> file(pIOHandler->Open(pFile));
	if( file.get() == NULL)
		throw new ImportErrorException("Failed to open file "+pFile+".");

	//Read the Mesh File:
	boost::scoped_ptr<CIrrXML_IOStreamReader> mIOWrapper( new CIrrXML_IOStreamReader( file.get()));
	XmlReader* MeshFile = irr::io::createIrrXMLReader(mIOWrapper.get());
	if(!MeshFile)//parse the xml file
		throw new ImportErrorException("Failed to create XML Reader for "+pFile);


	DefaultLogger::get()->info("Mesh File opened");
	
	//Read root Node:
	if(!(XmlRead(MeshFile) && string(MeshFile->getNodeName())=="mesh"))
	{
		throw new ImportErrorException("Root Node is not <mesh>! "+pFile+"  "+MeshFile->getNodeName());
	}

	//Go to the submeshs:
	if(!(XmlRead(MeshFile) && string(MeshFile->getNodeName())=="submeshes"))
	{
		throw new ImportErrorException("No <submeshes> node in <mesh> node! "+pFile);
	}


	//-------------------Read all submeshs:-----------------------
	XmlRead(MeshFile);
	while(string(MeshFile->getNodeName())=="submesh")//read the index values (the faces):
	{
		SubMesh NewSubMesh;
		NewSubMesh.MaterialName=GetAttribute<string>(MeshFile, "material");
		DefaultLogger::get()->info("Loading Submehs with Material: "+NewSubMesh.MaterialName);
		ReadSubMesh(NewSubMesh, MeshFile);
	}
	//_______________________________________________________________-



	//-----------------Read the skeleton:----------------------
	//Create the root node
	pScene->mRootNode=new aiNode("root");

	//link the mesh with the root node:
	pScene->mRootNode->mMeshes=new unsigned int[1];
	pScene->mRootNode->mMeshes[0]=0;
	pScene->mRootNode->mNumMeshes=1;
	//_________________________________________________________
}



void OgreImporter::GetExtensionList(std::string &append)
{
	append+="*.mesh.xml";
}


void OgreImporter::SetupProperties(const Importer* pImp)
{
	m_MaterialLibFilename=pImp->GetPropertyString(AI_CONFIG_IMPORT_OGRE_MATERIAL_FILE, "Scene.material");
}

void OgreImporter::ReadSubMesh(SubMesh &theSubMesh, XmlReader *Reader)
{
	vector<Face> FaceList;
	vector<aiVector3D> Positions; bool HasPositions=false;
	vector<aiVector3D> Normals; bool HasNormals=false;
	vector<aiVector3D> Uvs; unsigned int NumUvs=0;//nearly always 2d, but assimp has always 3d texcoords

	XmlRead(Reader);
	//TODO: maybe we have alsways just 1 faces and 1 geometry and always in this order. this loop will only work korrekt, wenn the order
	//of faces and geometry changed, and not if we habe more than one of one
	while(Reader->getNodeName()==string("faces") || string(Reader->getNodeName())=="geometry")
	{
		if(string(Reader->getNodeName())=="faces")//Read the face list
		{
			//some info logging:
			unsigned int NumFaces=GetAttribute<int>(Reader, "count");
			stringstream ss; ss <<"Submesh has " << NumFaces << " Faces.";
			DefaultLogger::get()->info(ss.str());

			while(XmlRead(Reader) && Reader->getNodeName()==string("face"))
			{
				Face NewFace;
				NewFace.VertexIndices[0]=GetAttribute<int>(Reader, "v1");
				NewFace.VertexIndices[1]=GetAttribute<int>(Reader, "v2");
				NewFace.VertexIndices[2]=GetAttribute<int>(Reader, "v3");
				if(Reader->getAttributeValue("v4"))//this should be supported in the future
				{
					throw new ImportErrorException("Submesh has quads, only traingles are supported!");
				}
				FaceList.push_back(NewFace);
			}

		}
		else if(string(Reader->getNodeName())=="geometry")//Read the vertexdata
		{	
			//some info logging:
			unsigned int NumVertices=GetAttribute<int>(Reader, "vertexcount");
			stringstream ss; ss<<"VertexCount: "<<NumVertices;
			DefaultLogger::get()->info(ss.str());
			
			//General Informations about vertices
			XmlRead(Reader);
			if(!(Reader->getNodeName()==string("vertexbuffer")))
			{
				throw new ImportErrorException("vertexbuffer node is not first in geometry node!");
			}
			HasPositions=GetAttribute<bool>(Reader, "positions");
			HasNormals=GetAttribute<bool>(Reader, "normals");
			NumUvs=GetAttribute<int>(Reader, "texture_coords");
			if(NumUvs>1)
				throw new ImportErrorException("too many texcoords (just 1 supported!)");

			//read all the vertices:
			XmlRead(Reader);
			while(Reader->getNodeName()==string("vertex"))
			{
				//read all vertex attributes:

				//Position
				if(HasPositions)
				{
					XmlRead(Reader);
					aiVector3D NewPos;
					NewPos.x=GetAttribute<float>(Reader, "x");
					NewPos.y=GetAttribute<float>(Reader, "y");
					NewPos.z=GetAttribute<float>(Reader, "z");
					Positions.push_back(NewPos);
				}
				
				//Normal
				if(HasNormals)
				{
					XmlRead(Reader);
					aiVector3D NewNormal;
					NewNormal.x=GetAttribute<float>(Reader, "x");
					NewNormal.y=GetAttribute<float>(Reader, "y");
					NewNormal.z=GetAttribute<float>(Reader, "z");
					Normals.push_back(NewNormal);
				}

				//Uv:
				if(1==NumUvs)
				{
					XmlRead(Reader);
					aiVector3D NewUv;
					NewUv.x=GetAttribute<float>(Reader, "u");
					NewUv.y=GetAttribute<float>(Reader, "v");
					Uvs.push_back(NewUv);
				}
				XmlRead(Reader);
			}

		}
	}
	DefaultLogger::get()->info(str(format("Positionen: %1% Normale: %2% TexCoords: %3%") % Positions.size() % Normals.size() % Uvs.size()));


	//Make all Vertexes unique: (this is required by assimp)
	vector<Face> UniqueFaceList(FaceList.size());
	vector<aiVector3D> UniquePositions(FaceList.size()*3);//*3 because each face consits of 3 vertexes, because we only support triangles^^
	vector<aiVector3D> UniqueNormals(FaceList.size()*3);
	vector<aiVector3D> UniqueUvs(FaceList.size()*3);

	for(unsigned int i=0; i<FaceList.size(); ++i)
	{
		UniquePositions[3*i+0]=Positions[FaceList[i].VertexIndices[0]];
		UniquePositions[3*i+1]=Positions[FaceList[i].VertexIndices[1]];
		UniquePositions[3*i+2]=Positions[FaceList[i].VertexIndices[2]];

		UniqueNormals[3*i+0]=Normals[FaceList[i].VertexIndices[0]];
		UniqueNormals[3*i+1]=Normals[FaceList[i].VertexIndices[1]];
		UniqueNormals[3*i+2]=Normals[FaceList[i].VertexIndices[2]];

		UniqueUvs[3*i+0]=Uvs[FaceList[i].VertexIndices[0]];
		UniqueUvs[3*i+1]=Uvs[FaceList[i].VertexIndices[1]];
		UniqueUvs[3*i+2]=Uvs[FaceList[i].VertexIndices[2]];

		UniqueFaceList[i].VertexIndices[0]=3*i+0;
		UniqueFaceList[i].VertexIndices[1]=3*i+1;
		UniqueFaceList[i].VertexIndices[2]=3*i+2;
	}

	//----------------Load the Material:-------------------------------
	aiMaterial* MeshMat=LoadMaterial(theSubMesh.MaterialName);
	//_________________________________________________________________

	//Mesh is fully loaded, copy it into the aiScene:
	if(m_CurrentScene->mNumMeshes!=0)
		throw new ImportErrorException("Currently only one mesh per File is allowed!!");

	//---------------------Create the aiMesh:-----------------------
	aiMesh* NewAiMesh=new aiMesh();
	
	//Positions
	NewAiMesh->mVertices=new aiVector3D[UniquePositions.size()];
	memcpy(NewAiMesh->mVertices, &UniquePositions[0], UniquePositions.size()*sizeof(aiVector3D));
	NewAiMesh->mNumVertices=UniquePositions.size();

	//Normals
	NewAiMesh->mNormals=new aiVector3D[UniqueNormals.size()];
	memcpy(NewAiMesh->mNormals, &UniqueNormals[0], UniqueNormals.size()*sizeof(aiVector3D));

	//Uvs
	NewAiMesh->mNumUVComponents[0]=2;
	//NewAiMesh->mTextureCoords=new aiVector3D*[1];
	NewAiMesh->mTextureCoords[0]= new aiVector3D[UniqueUvs.size()];
	memcpy(NewAiMesh->mTextureCoords[0], &UniqueUvs[0], UniqueUvs.size()*sizeof(aiVector3D));

	//Faces
	NewAiMesh->mFaces=new aiFace[UniqueFaceList.size()];
	for(unsigned int i=0; i<UniqueFaceList.size(); ++i)
	{
		NewAiMesh->mFaces[i].mNumIndices=3;
		NewAiMesh->mFaces[i].mIndices=new unsigned int[3];

		NewAiMesh->mFaces[i].mIndices[0]=UniqueFaceList[i].VertexIndices[0];
		NewAiMesh->mFaces[i].mIndices[1]=UniqueFaceList[i].VertexIndices[1];
		NewAiMesh->mFaces[i].mIndices[2]=UniqueFaceList[i].VertexIndices[2];
	}
	NewAiMesh->mNumFaces=UniqueFaceList.size();

	//Set the Material:
	NewAiMesh->mMaterialIndex=0;
	if(m_CurrentScene->mMaterials)
		throw new ImportErrorException("only 1 material supported at this time!");
	m_CurrentScene->mMaterials=new aiMaterial*[1];
	m_CurrentScene->mNumMaterials=1;
	m_CurrentScene->mMaterials[0]=MeshMat;
	//________________________________________________________________


	//Attach the mesh to the scene:
	m_CurrentScene->mNumMeshes=1;
	m_CurrentScene->mMeshes=new aiMesh*;
	m_CurrentScene->mMeshes[0]=NewAiMesh;


	//stringstream ss; ss <<"Last Node: <" << Reader->getNodeName() << ">";
	//throw new ImportErrorException(ss.str());
}

aiMaterial* OgreImporter::LoadMaterial(std::string MaterialName)
{
	MaterialHelper *NewMaterial=new MaterialHelper();
	NewMaterial->AddProperty(&aiString(MaterialName.c_str()), AI_MATKEY_NAME);
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
				DefaultLogger::get()->error(m_MaterialLibFilename+" and "+MaterialFileName + " could not be opned, Material will not be loaded!");
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
	unsigned int Level=0;//Hierarchielevels in the material file, like { } blocks into another
	while(!ss.eof())
	{
		if(Line=="material")
		{
			ss >> Line;
			if(Line==MaterialName)//Load the next material
			{
				ss >> Line;
				if(Line!="{")
					throw new ImportErrorException("empty material!");

				while(Line!="}")//read until the end of the material
				{
					//Proceed to the first technique
					ss >> Line;
					if(Line=="technique")
					{
						ss >> Line;
						if(Line!="{")
							throw new ImportErrorException("empty technique!");
						while(Line!="}")//read until the end of the technique
						{
							ss >> Line;
							if(Line=="pass")
							{
								ss >> Line;
								if(Line!="{")
									throw new ImportErrorException("empty pass!");
								while(Line!="}")//read until the end of the pass
								{
									ss >> Line;
									if(Line=="ambient")
									{
										//read the ambient light values:
									}
									else if(Line=="diffuse")
									{
									}
									else if(Line=="specular")
									{
									}
									else if(Line=="emmisive")
									{
									}
									else if(Line=="texture_unit")
									{
										ss >> Line;
										if(Line!="{")
											throw new ImportErrorException("empty texture unit!");
										while(Line!="}")//read until the end of the texture_unit
										{
											ss >> Line;
											if(Line=="texture")
											{
												ss >> Line;
												NewMaterial->AddProperty(&aiString(Line.c_str()), AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
											}
										}//end of texture unit
									}
								}
							}
						}//end of technique
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