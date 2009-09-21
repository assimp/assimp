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


	DefaultLogger::get()->debug("Mesh File opened");
	
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
		DefaultLogger::get()->debug("Loading Submehs with Material: "+NewSubMesh.MaterialName);
		ReadSubMesh(NewSubMesh, MeshFile);
	}
	//____________________________________________________________


	//-----------------Create the root node-----------------------
	pScene->mRootNode=new aiNode("root");

	//link the mesh with the root node:
	pScene->mRootNode->mMeshes=new unsigned int[1];
	pScene->mRootNode->mMeshes[0]=0;
	pScene->mRootNode->mNumMeshes=1;
	//____________________________________________________________


	//----------------Load the skeleton: -------------------------------
	if(MeshFile->getNodeName()==string("skeletonlink"))
	{
		string SkeletonFile=GetAttribute<string>(MeshFile, "name");
		LoadSkeleton(SkeletonFile);
	}
	else
		DefaultLogger::get()->warn(MeshFile->getNodeName());
	//__________________________________________________________________


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
	vector< vector<Weight> > Weights;

	XmlRead(Reader);
	//TODO: maybe we have alsways just 1 faces and 1 geometry and always in this order. this loop will only work correct, wenn the order
	//of faces and geometry changed, and not if we habe more than one of one
	while(Reader->getNodeName()==string("faces") || string(Reader->getNodeName())=="geometry" || Reader->getNodeName()==string("boneassignments"))
	{
		if(string(Reader->getNodeName())=="faces")//Read the face list
		{
			//some info logging:
			unsigned int NumFaces=GetAttribute<int>(Reader, "count");
			stringstream ss; ss <<"Submesh has " << NumFaces << " Faces.";
			DefaultLogger::get()->debug(ss.str());

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

		}//end of faces
		else if(string(Reader->getNodeName())=="geometry")//Read the vertexdata
		{	
			//some info logging:
			unsigned int NumVertices=GetAttribute<int>(Reader, "vertexcount");
			stringstream ss; ss<<"VertexCount: "<<NumVertices;
			DefaultLogger::get()->debug(ss.str());
			
			//General Informations about vertices
			XmlRead(Reader);
			if(!(Reader->getNodeName()==string("vertexbuffer")))
			{
				throw new ImportErrorException("vertexbuffer node is not first in geometry node!");
			}
			HasPositions=GetAttribute<bool>(Reader, "positions");
			HasNormals=GetAttribute<bool>(Reader, "normals");
			if(!Reader->getAttributeValue("texture_coords"))//we can have 1 or 0 uv channels, and if the mesh has no uvs, it also doesn't have the attribute
				NumUvs=0;
			else
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
					NewUv.y=GetAttribute<float>(Reader, "v")*(-1)+1;//flip the uv vertikal, blender exports them so!
					Uvs.push_back(NewUv);
				}
				XmlRead(Reader);
			}

		}//end of "geometry
		else if(string(Reader->getNodeName())=="boneassignments")
		{
			Weights.resize(Positions.size());
			while(XmlRead(Reader) && Reader->getNodeName()==string("vertexboneassignment"))
			{
				Weight NewWeight;
				unsigned int VertexId=GetAttribute<int>(Reader, "vertexindex");
				NewWeight.BoneId=GetAttribute<int>(Reader, "boneindex");
				NewWeight.Value=GetAttribute<float>(Reader, "weight");
				
				Weights[VertexId].push_back(NewWeight);

				//XmlRead(Reader);//Once i had this line, and than i got only every second boneassignment, but my first test models had even boneassignment counts, so i thougt, everything would work. And yes, i HATE irrXML!!!
			}

		}//end of boneassignments
	}
	DefaultLogger::get()->debug(str(format("Positionen: %1% Normale: %2% TexCoords: %3%") % Positions.size() % Normals.size() % Uvs.size()));
	DefaultLogger::get()->warn(Reader->getNodeName());



	//---------------Make all Vertexes unique: (this is required by assimp)-----------------------
	vector<Face> UniqueFaceList(FaceList.size());
	vector<aiVector3D> UniquePositions(FaceList.size()*3);//*3 because each face consits of 3 vertexes, because we only support triangles^^
	vector<aiVector3D> UniqueNormals(FaceList.size()*3);
	vector<aiVector3D> UniqueUvs(FaceList.size()*3);
	vector< vector<Weight> > UniqueWeights(FaceList.size()*3);

	for(unsigned int i=0; i<FaceList.size(); ++i)
	{
		UniquePositions[3*i+0]=Positions[FaceList[i].VertexIndices[0]];
		UniquePositions[3*i+1]=Positions[FaceList[i].VertexIndices[1]];
		UniquePositions[3*i+2]=Positions[FaceList[i].VertexIndices[2]];

		UniqueNormals[3*i+0]=Normals[FaceList[i].VertexIndices[0]];
		UniqueNormals[3*i+1]=Normals[FaceList[i].VertexIndices[1]];
		UniqueNormals[3*i+2]=Normals[FaceList[i].VertexIndices[2]];

		if(1==NumUvs)
		{
			UniqueUvs[3*i+0]=Uvs[FaceList[i].VertexIndices[0]];
			UniqueUvs[3*i+1]=Uvs[FaceList[i].VertexIndices[1]];
			UniqueUvs[3*i+2]=Uvs[FaceList[i].VertexIndices[2]];
		}

		UniqueWeights[3*i+0]=UniqueWeights[FaceList[i].VertexIndices[0]];
		UniqueWeights[3*i+1]=UniqueWeights[FaceList[i].VertexIndices[1]];
		UniqueWeights[3*i+2]=UniqueWeights[FaceList[i].VertexIndices[2]];

		UniqueFaceList[i].VertexIndices[0]=3*i+0;
		UniqueFaceList[i].VertexIndices[1]=3*i+1;
		UniqueFaceList[i].VertexIndices[2]=3*i+2;
	}
	//_________________________________________________________________________________________

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
	if(0!=NumUvs)
	{
		NewAiMesh->mNumUVComponents[0]=2;
		NewAiMesh->mTextureCoords[0]= new aiVector3D[UniqueUvs.size()];
		memcpy(NewAiMesh->mTextureCoords[0], &UniqueUvs[0], UniqueUvs.size()*sizeof(aiVector3D));
	}

	//Bones
	/*NewAiMesh->mNumBones=UniqueWeights.size();
	NewAiMesh->mBones=new aiBone*[UniqueWeights.size()];
	for(un*/



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
	//_____________________________________________________________________________


	//Attach the mesh to the scene:
	m_CurrentScene->mNumMeshes=1;
	m_CurrentScene->mMeshes=new aiMesh*;
	m_CurrentScene->mMeshes[0]=NewAiMesh;
}

aiMaterial* OgreImporter::LoadMaterial(std::string MaterialName)
{
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

void OgreImporter::LoadSkeleton(std::string FileName)
{
	//most likely the skeleton file will only end with .skeleton
	//But this is a xml reader, so we need: .skeleton.xml
	FileName+=".xml";

	DefaultLogger::get()->debug(string("Loading Skeleton: ")+FileName);

	//Open the File:
	boost::scoped_ptr<IOStream> File(m_CurrentIOHandler->Open(FileName));
	if(NULL==File.get())
		throw new ImportErrorException("Failed to open skeleton file "+FileName+".");

	//Read the Mesh File:
	boost::scoped_ptr<CIrrXML_IOStreamReader> mIOWrapper(new CIrrXML_IOStreamReader(File.get()));
	XmlReader* SkeletonFile = irr::io::createIrrXMLReader(mIOWrapper.get());
	if(!SkeletonFile)
		throw new ImportErrorException(string("Failed to create XML Reader for ")+FileName);

	//Variables to store the data from the skeleton file:
	vector<Bone> Bones;

	//Quick note: Whoever read this should know this one thing: irrXml fucking sucks!!!

	XmlRead(SkeletonFile);
	if(string("skeleton")!=SkeletonFile->getNodeName())
		throw new ImportErrorException("No <skeleton> node in SkeletonFile: "+FileName);



	//------------------------------------load bones-----------------------------------------
	XmlRead(SkeletonFile);
	if(string("bones")!=SkeletonFile->getNodeName())
		throw new ImportErrorException("No bones node in skeleton "+FileName);

	XmlRead(SkeletonFile);

	while(string("bone")==SkeletonFile->getNodeName())
	{
		//TODO: Maybe we can have bone ids for the errrors, but normaly, they should never appera, so what....

		//read a new bone:
		Bone NewBone;
		NewBone.Id=GetAttribute<int>(SkeletonFile, "id");
		NewBone.Name=GetAttribute<string>(SkeletonFile, "name");

		//load the position:
		XmlRead(SkeletonFile);
		if(string("position")!=SkeletonFile->getNodeName())
			throw new ImportErrorException("Position is not first node in Bone!");
		NewBone.Position.x=GetAttribute<float>(SkeletonFile, "x");
		NewBone.Position.y=GetAttribute<float>(SkeletonFile, "y");
		NewBone.Position.z=GetAttribute<float>(SkeletonFile, "z");

		//Rotation:
		XmlRead(SkeletonFile);
		if(string("rotation")!=SkeletonFile->getNodeName())
			throw new ImportErrorException("Rotation is not the second node in Bone!");
		NewBone.RotationAngle=GetAttribute<float>(SkeletonFile, "angle");
		XmlRead(SkeletonFile);
		if(string("axis")!=SkeletonFile->getNodeName())
			throw new ImportErrorException("No axis specified for bone rotation!");
		NewBone.RotationAxis.x=GetAttribute<float>(SkeletonFile, "x");
		NewBone.RotationAxis.y=GetAttribute<float>(SkeletonFile, "y");
		NewBone.RotationAxis.z=GetAttribute<float>(SkeletonFile, "z");

		//append the newly loaded bone to the bone list
		Bones.push_back(NewBone);

		//Proceed to the next bone:
		XmlRead(SkeletonFile);
	}
	//The bones in the file a not neccesarly ordered by there id's so we do it now:
	std::sort(Bones.begin(), Bones.end());
	//now the id of each bone should be equal to its position in the vector:
	//so we do a simple check:
	{
		bool IdsOk=true;
		for(int i=0; i<static_cast<signed int>(Bones.size()); ++i)//i is signed, because all Id's are also signed!
		{
			if(Bones[i].Id!=i)
				IdsOk=false;
		}
		if(!IdsOk)
			throw new ImportErrorException("Bone Ids are not valid!"+FileName);
	}
	DefaultLogger::get()->debug(str(format("Number of bones: %1%") % Bones.size()));
	//________________________________________________________________________________






	//----------------------------load bonehierarchy--------------------------------
	if(string("bonehierarchy")!=SkeletonFile->getNodeName())
		throw new ImportErrorException("no bonehierarchy node in "+FileName);

	DefaultLogger::get()->debug("loading bonehierarchy...");
	XmlRead(SkeletonFile);
	while(string("boneparent")==SkeletonFile->getNodeName())
	{
		string Child, Parent;
		Child=GetAttribute<string>(SkeletonFile, "bone");
		Parent=GetAttribute<string>(SkeletonFile, "parent");

		unsigned int ChildId, ParentId;
		ChildId=find(Bones.begin(), Bones.end(), Child)->Id;
		ParentId=find(Bones.begin(), Bones.end(), Parent)->Id;

		Bones[ChildId].ParentId=ParentId;
		Bones[ParentId].Children.push_back(ChildId);

		XmlRead(SkeletonFile);//i once forget this line, which led to an endless loop, did i mentioned, that irrxml sucks??
	}
	//_____________________________________________________________________________

	


	//---------------------------load animations-----------------------------
	vector<Animation> Animations;
	if(string("animations")==SkeletonFile->getNodeName())//animations are optional values
	{
		DefaultLogger::get()->debug("Loading Animations");
		XmlRead(SkeletonFile);
		while(string("animation")==SkeletonFile->getNodeName())
		{
			Animation NewAnimation;
			NewAnimation.Name=GetAttribute<string>(SkeletonFile, "name");
			NewAnimation.Length=GetAttribute<float>(SkeletonFile, "length");
			
			//Load all Tracks
			XmlRead(SkeletonFile);
			if(string("tracks")!=SkeletonFile->getNodeName())
				throw new ImportErrorException("no tracks node in animation");
			XmlRead(SkeletonFile);
			while(string("track")==SkeletonFile->getNodeName())
			{
				Track NewTrack;
				NewTrack.BoneName=GetAttribute<string>(SkeletonFile, "bone");

				//Load all keyframes;
				XmlRead(SkeletonFile);
				if(string("keyframes")!=SkeletonFile->getNodeName())
					throw new ImportErrorException("no keyframes node!");
				XmlRead(SkeletonFile);
				while(string("keyframe")==SkeletonFile->getNodeName())
				{
					Keyframe NewKeyframe;
					NewKeyframe.Time=GetAttribute<float>(SkeletonFile, "time");

					//Position:
					XmlRead(SkeletonFile);
					if(string("translate")!=SkeletonFile->getNodeName())
						throw new ImportErrorException("translate node not first in keyframe");
					NewKeyframe.Position.x=GetAttribute<float>(SkeletonFile, "x");
					NewKeyframe.Position.y=GetAttribute<float>(SkeletonFile, "y");
					NewKeyframe.Position.z=GetAttribute<float>(SkeletonFile, "z");

					//Rotation:
					XmlRead(SkeletonFile);
					if(string("rotate")!=SkeletonFile->getNodeName())
						throw new ImportErrorException("rotate is not second node in keyframe");
					float RotationAngle=GetAttribute<float>(SkeletonFile, "angle");
					aiVector3D RotationAxis;
					XmlRead(SkeletonFile);
					if(string("axis")!=SkeletonFile->getNodeName())
						throw new ImportErrorException("No axis for keyframe rotation!");
					RotationAxis.x=GetAttribute<float>(SkeletonFile, "x");
					RotationAxis.y=GetAttribute<float>(SkeletonFile, "y");
					RotationAxis.z=GetAttribute<float>(SkeletonFile, "z");
					NewKeyframe.Rotation=aiQuaternion(RotationAxis, RotationAngle);

					//Scaling:
					XmlRead(SkeletonFile);
					if(string("scale")!=SkeletonFile->getNodeName())
						throw new ImportErrorException("no scalling key in keyframe!");
					NewKeyframe.Scaling.x=GetAttribute<float>(SkeletonFile, "x");
					NewKeyframe.Scaling.y=GetAttribute<float>(SkeletonFile, "y");
					NewKeyframe.Scaling.z=GetAttribute<float>(SkeletonFile, "z");


					NewTrack.Keyframes.push_back(NewKeyframe);
					XmlRead(SkeletonFile);
				}


				NewAnimation.Tracks.push_back(NewTrack);
			}

			Animations.push_back(NewAnimation);
		}
	}
	//_____________________________________________________________________________





	//-----------------skeleton is completly loaded, now but it in the assimp scene:-------------------------------
	
	if(!m_CurrentScene->mRootNode)
		throw new ImportErrorException("No root node exists!!");
	if(0!=m_CurrentScene->mRootNode->mNumChildren)
		throw new ImportErrorException("Root Node already has childnodes!");

	//--------------Creatre the assimp bone hierarchy-----------------
	DefaultLogger::get()->debug("Root Bones");
	vector<aiNode*> RootBoneNodes;
	BOOST_FOREACH(Bone theBone, Bones)
	{
		if(-1==theBone.ParentId) //the bone is a root bone
		{
			DefaultLogger::get()->debug(theBone.Name);
			RootBoneNodes.push_back(CreateAiNodeFromBone(theBone.Id, Bones, m_CurrentScene->mRootNode));
		}
	}
	m_CurrentScene->mRootNode->mNumChildren=RootBoneNodes.size();
	m_CurrentScene->mRootNode->mChildren=new aiNode*[RootBoneNodes.size()];
	memcpy(m_CurrentScene->mRootNode->mChildren, &RootBoneNodes[0], sizeof(aiNode*)*RootBoneNodes.size());
	//_______________________________________________________________


	//-----------------Create the Assimp Animations --------------------
	if(Animations.size()>0)//Maybe the model had only a skeleton and no animations. (If it also has no skeleton, this function would'nt have benn called
	{
		m_CurrentScene->mNumAnimations=Animations.size();
		m_CurrentScene->mAnimations=new aiAnimation*[Animations.size()];
		for(unsigned int i=0; i<Animations.size(); ++i)//create all animations
		{
			aiAnimation* NewAnimation=new aiAnimation();
			NewAnimation->mName=Animations[i].Name;
			NewAnimation->mDuration=Animations[i].Length;
			NewAnimation->mTicksPerSecond=0.05f;

			//Create all tracks in this animation
			NewAnimation->mNumChannels=Animations[i].Tracks.size();
			NewAnimation->mChannels=new aiNodeAnim*[Animations[i].Tracks.size()];
			for(unsigned int j=0; j<Animations[i].Tracks.size(); ++j)
			{
				aiNodeAnim* NewNodeAnim=new aiNodeAnim();
				NewNodeAnim->mNodeName=Animations[i].Tracks[j].BoneName;

				//Create the keyframe arrays...
				unsigned int KeyframeCount=Animations[i].Tracks[j].Keyframes.size();
				NewNodeAnim->mNumPositionKeys=KeyframeCount;
				NewNodeAnim->mPositionKeys=new aiVectorKey[KeyframeCount];
				NewNodeAnim->mNumRotationKeys=KeyframeCount;
				NewNodeAnim->mRotationKeys=new aiQuatKey[KeyframeCount];
				NewNodeAnim->mNumScalingKeys=KeyframeCount;
				NewNodeAnim->mScalingKeys=new aiVectorKey[KeyframeCount];
				
				//...and fill them
				for(unsigned int k=0; k<KeyframeCount; ++k)
				{
					NewNodeAnim->mPositionKeys[k].mTime=Animations[i].Tracks[j].Keyframes[k].Time;
					NewNodeAnim->mPositionKeys[k].mValue=Animations[i].Tracks[j].Keyframes[k].Position;
					
					NewNodeAnim->mRotationKeys[k].mTime=Animations[i].Tracks[j].Keyframes[k].Time;
					NewNodeAnim->mRotationKeys[k].mValue=Animations[i].Tracks[j].Keyframes[k].Rotation;

					NewNodeAnim->mScalingKeys[k].mTime=Animations[i].Tracks[j].Keyframes[k].Time;
					NewNodeAnim->mScalingKeys[k].mValue=Animations[i].Tracks[j].Keyframes[k].Scaling;
				}
				
				NewAnimation->mChannels[j]=NewNodeAnim;
			}

			m_CurrentScene->mAnimations[i]=NewAnimation;
		}
	}
	//__________________________________________________________________
}




aiNode* CreateAiNodeFromBone(int BoneId, std::vector<Bone> Bones, aiNode* ParentNode)
{
	//----Create the node for this bone and set its values-----
	aiNode* NewNode=new aiNode(Bones[BoneId].Name);
	NewNode->mParent=ParentNode;

	aiMatrix4x4 t0,t1;
	//create a matrix from the transformation values of the ogre bone
	NewNode->mTransformation=aiMatrix4x4::Translation(Bones[BoneId].Position, t0)
							*
							aiMatrix4x4::Rotation(Bones[BoneId].RotationAngle, Bones[BoneId].RotationAxis, t1)
							;
	//__________________________________________________________


	//----recursivly create all children Nodes:------
	NewNode->mNumChildren=Bones[BoneId].Children.size();
	NewNode->mChildren=new aiNode*[Bones[BoneId].Children.size()];
	for(unsigned int i=0; i<Bones[BoneId].Children.size(); ++i)
	{
		NewNode->mChildren[i]=CreateAiNodeFromBone(Bones[BoneId].Children[i], Bones, NewNode);
	}
	//____________________________________________________


	return NewNode;
}


}//namespace Ogre
}//namespace Assimp

#endif  // !! ASSIMP_BUILD_NO_OGRE_IMPORTER