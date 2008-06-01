#include "ObjFileImporter.h"
#include "ObjFileParser.h"
#include "ObjFileData.h"
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "MaterialSystem.h"
#include "../include/DefaultLogger.h"

#include <boost/scoped_ptr.hpp>
#include <boost/format.hpp>

namespace Assimp
{
// ------------------------------------------------------------------------------------------------

using namespace std;

//!	Obj-file-format extention
const string ObjFileImporter::OBJ_EXT = "obj";

// ------------------------------------------------------------------------------------------------
//	Default constructor
ObjFileImporter::ObjFileImporter() :
	m_pRootObject(NULL),
	m_strAbsPath("\\")
{
}

// ------------------------------------------------------------------------------------------------
//	Destructor
ObjFileImporter::~ObjFileImporter()
{
	// Release root object instance
	if (NULL != m_pRootObject)
	{
		delete m_pRootObject;
		m_pRootObject = NULL;
	}
}

// ------------------------------------------------------------------------------------------------
//	Returns true, fi file is an obj file
bool ObjFileImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	if (pFile.empty())
		return false;

	string::size_type pos = pFile.find_last_of(".");
	if (string::npos == pos)
		return false;
	
	const string ext = pFile.substr(pos+1, pFile.size() - pos - 1);
	if (ext == OBJ_EXT)
		return true;

	return false;
}

// ------------------------------------------------------------------------------------------------
//	Obj-file import implementation
void ObjFileImporter::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler)
{
	// Read file into memory
	const std::string mode  = "rb";
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile, mode));
	if (NULL == file.get())
		throw new ImportErrorException( "Failed to open file " + pFile + ".");

	// Get the filesize and vaslidate it, throwing an exception when failes
	size_t fileSize = file->FileSize();
	if( fileSize < 16)
		throw new ImportErrorException( "OBJ-file is too small.");

	// Allocate buffer and read file into it
	m_Buffer.resize( fileSize );
	const size_t readsize = file->Read(&m_Buffer.front(), sizeof(char), fileSize);
	assert (readsize == fileSize);

	//
	std::string strDirectory("\\"), strModelName;
	std::string::size_type pos = pFile.find_last_of("\\");
	if (pos != std::string::npos)
	{
		strDirectory = pFile.substr(0, pos);
		strModelName = pFile.substr(pos+1, pFile.size() - pos - 1);
	}
	else
	{
		strModelName = pFile;
	}
	
	// parse the file into a temporary representation
	ObjFileParser parser(m_Buffer, strDirectory, strModelName);

	// And create the proper return structures out of it
	CreateDataFromImport(parser.GetModel(), pScene);
}

// ------------------------------------------------------------------------------------------------
//	Create the data from parsed obj-file
void ObjFileImporter::CreateDataFromImport(const ObjFile::Model* pModel, aiScene* pScene)
{
	if (0L == pModel)
		return;
		
	// Create the root node of the scene
	pScene->mRootNode = new aiNode();
	if (!pModel->m_ModelName.empty())
	{
		// Set the name of the scene
		pScene->mRootNode->mName.Set(pModel->m_ModelName);
	}
	else
	{
		// This is an error, so break down the application
		assert (false);
	}

	// Create nodes for the whole scene	
	std::vector<aiMesh*> MeshArray;
	for (size_t index = 0; index < pModel->m_Objects.size(); index++)
	{
		createNodes(pModel, pModel->m_Objects[ index ], pScene->mRootNode, pScene, MeshArray);
	}

	// Create mesh pointer buffer for this scene
	if (pScene->mNumMeshes > 0)
	{
		pScene->mMeshes = new aiMesh*[ pScene->mNumMeshes ];
		for (size_t index =0; index < pScene->mNumMeshes; index++)
		{
			pScene->mMeshes [ index ] = MeshArray[ index ];
		}
	}

	// Create all materials
	for (size_t index = 0; index < pModel->m_Objects.size(); index++)
	{
		createMaterial(pModel, pModel->m_Objects[ index ], pScene);
	}
}

// ------------------------------------------------------------------------------------------------
//	Creates all nodes of the model
aiNode *ObjFileImporter::createNodes(const ObjFile::Model* pModel, const ObjFile::Object* pData, 
									 aiNode *pParent, aiScene* pScene, 
									 std::vector<aiMesh*> &MeshArray)
{
	if (NULL == pData)
		return NULL;
	
	// Store older mesh size to be able to computate mesh offsets for new mesh instances
	size_t oldMeshSize = MeshArray.size();
	aiNode *pNode = new aiNode();
	
	if (pParent != NULL)
		this->appendChildToParentNode(pParent, pNode);

	aiMesh *pMesh = new aiMesh();
	MeshArray.push_back(pMesh);
	createTopology(pModel, pData, pMesh);

	// Create all nodes from the subobjects stored in the current object
	if (!pData->m_SubObjects.empty())
	{
		pNode->mNumChildren = (unsigned int)pData->m_SubObjects.size();
		pNode->mChildren = new aiNode*[pData->m_SubObjects.size()];
		pNode->mNumMeshes = 1;
		pNode->mMeshes = new unsigned int[1];

		// Loop over all child objects
		for (size_t index = 0; index < pData->m_SubObjects.size(); index++)
		{
			// Create all child nodes
			pNode->mChildren[index] = createNodes(pModel, pData, pNode, pScene, MeshArray);
			
			// Create meshes of this object
			pMesh = new aiMesh();
			MeshArray.push_back(pMesh);
			createTopology(pModel, pData->m_SubObjects[ index ], pMesh);

			// Create material of this object
			createMaterial(pModel, pData->m_SubObjects[ index ], pScene);
		}
	}

	// Set mesh instances into scene- and node-instances
	const size_t meshSizeDiff = MeshArray.size()- oldMeshSize;
	if (meshSizeDiff > 0 )
	{
		pNode->mMeshes = new unsigned int[ meshSizeDiff ];
		pNode->mNumMeshes++;
		size_t index = 0;
		for (size_t i = oldMeshSize; i < MeshArray.size(); i++)
		{
			pNode->mMeshes[ index ] = pScene->mNumMeshes;
			pScene->mNumMeshes++;
			index++;
		}
	}
	
	return pNode;
}

// ------------------------------------------------------------------------------------------------
//	Create topology data
void ObjFileImporter::createTopology(const ObjFile::Model* pModel, const ObjFile::Object* pData, 
									 aiMesh* pMesh)
{
	if (NULL == pData)
		return;
	
	// Create mesh vertices
	createVertexArray(pModel, pData, pMesh);

	// Create faces
	pMesh->mNumFaces = (unsigned int)pData->m_Faces.size();
	pMesh->mFaces = new aiFace[pMesh->mNumFaces];
	for (size_t index = 0; index < pMesh->mNumFaces; index++)
	{
		aiFace *pFace = &pMesh->mFaces[ index ];
		pFace->mNumIndices = (unsigned int)pData->m_Faces[index]->m_pVertices->size();
		if (pFace->mNumIndices > 0)
		{
			pFace->mIndices = new unsigned int[pMesh->mFaces[index].mNumIndices];
			ObjFile::Face::IndexArray *pArray = pData->m_Faces[index]->m_pVertices;
			
			// TODO:	Should be implement much better
			//memcpy(pFace->mIndices, &pData->m_Faces[index]->m_pVertices[0], pFace->mNumIndices  * sizeof(unsigned int));
			if (pArray != NULL)
			{
				for (size_t a=0; a<pFace->mNumIndices; a++)
				{
					pFace->mIndices[a] = pArray->at( a );
				}
			}
			else
			{
				ai_assert (false);
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ObjFileImporter::createVertexArray(const ObjFile::Model* pModel, 
										const ObjFile::Object* pCurrentObject, 
										aiMesh* pMesh)
{
	// Break, if no faces are stored in object
	if (pCurrentObject->m_Faces.empty())
		return;
	
	// Copy all stored vertices, normals and so on
	pMesh->mNumVertices = (unsigned int)pModel->m_Vertices.size();
	pMesh->mVertices = new aiVector3D[pMesh->mNumVertices];
	for (size_t index=0; index < pModel->m_Vertices.size(); index++)
	{
		pMesh->mVertices[ index ] = *pModel->m_Vertices[ index ];
	}
	
	if (!pModel->m_Normals.empty())
	{
		pMesh->mNormals = new aiVector3D[pModel->m_Normals.size()];
		for (size_t index = 0; index < pModel->m_Normals.size(); index++)
		{
			pMesh->mNormals[ index ] = *pModel->m_Normals[ index ];
		}
	}

	if (!pModel->m_TextureCoord.empty())
	{
		// TODO: Implement texture coodinates
	}
}

// ------------------------------------------------------------------------------------------------
void ObjFileImporter::countObjects(const std::vector<ObjFile::Object*> &rObjects, int &iNumMeshes)
{
	iNumMeshes = 0;
	if (rObjects.empty())	
		return;

	iNumMeshes += (unsigned int)rObjects.size();
	for (std::vector<ObjFile::Object*>::const_iterator it = rObjects.begin();
		it != rObjects.end(); 
		++it)
	{
		if (!(*it)->m_SubObjects.empty())
		{
			countObjects((*it)->m_SubObjects, iNumMeshes);
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ObjFileImporter::createMaterial(const ObjFile::Model* pModel, const ObjFile::Object* pData, 
									 aiScene* pScene)
{
	ai_assert (NULL != pScene);
	if (NULL == pData)
		return;

	// Create only a dumy material to enshure a running viewer
	pScene->mNumMaterials = 1;
	Assimp::MaterialHelper* mat = new Assimp::MaterialHelper;
	
	// Create a new material
	pScene->mMaterials = new aiMaterial*[1];
	pScene->mMaterials[0] = mat;
}

// ------------------------------------------------------------------------------------------------
//	Appends this node to the parent node
void ObjFileImporter::appendChildToParentNode(aiNode *pParent, aiNode *pChild)
{
	// Checking preconditions
	ai_assert (NULL != pParent);
	ai_assert (NULL != pChild);

	// Assign parent to child
	pChild->mParent = pParent;
	size_t sNumChildren = 0;
	
	// If already children was assigned to the parent node, store them in a 
	std::vector<aiNode*> temp;
	if (pParent->mChildren != NULL)
	{
		sNumChildren = pParent->mNumChildren;
		ai_assert (0 != sNumChildren);
		for (size_t index = 0; index < pParent->mNumChildren; index++)
		{
			temp.push_back(pParent->mChildren [ index ] );
		}
		delete [] pParent->mChildren;
	}
	
	// Copy node instances into parent node
	pParent->mNumChildren++;
	pParent->mChildren = new aiNode*[ pParent->mNumChildren ];
	for (size_t index = 0; index < pParent->mNumChildren-1; index++)
	{
		pParent->mChildren[ index ] = temp [ index ];
	}
	pParent->mChildren[ pParent->mNumChildren-1 ] = pChild;

}

// ------------------------------------------------------------------------------------------------

}	// Namespace Assimp
