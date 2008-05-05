#ifndef OBJ_FILEDATA_H_INC
#define OBJ_FILEDATA_H_INC

#include <vector>
#include <map>
#include "aiTypes.h"

namespace Assimp
{

namespace ObjFile
{

struct Object;
struct Face;
struct Material;

// ------------------------------------------------------------------------------------------------
//!	\struct	Face
//!	\brief	Datastructure for a simple obj-face, descripes discredisation and materials
struct Face
{
	typedef std::vector<unsigned int> IndexArray;

	//!	Primitive type
	int m_PrimitiveType;
	//!	Vertex indices
	IndexArray *m_pVertices;
	//!	Normal indices
	IndexArray *m_pNormals;
	//!	Texture coordinates indices
	IndexArray *m_pTexturCoords;
	//!	Pointer to assigned material
	Material *m_pMaterial;
	
	//!	\brief	Default constructor
	//!	\param	pVertices	Pointer to assigned vertex indexbuffer
	//!	\param	pNormals	Pointer to assigned normals indexbuffer
	//!	\param	pTexCoords	Pointer to assigned texture indexbuffer
	Face(std::vector<unsigned int> *pVertices, 
			std::vector<unsigned int> *pNormals, 
			std::vector<unsigned int> *pTexCoords) : 
		m_PrimitiveType(2), 
		m_pVertices(pVertices), 
		m_pNormals(pNormals),
		m_pTexturCoords(pTexCoords), 
		m_pMaterial(0L)
	{
		// empty
	}
	
	//!	\brief	Destructor	
	~Face()
	{	
		// empty
	}
};

// ------------------------------------------------------------------------------------------------
//!	\struct	Object
//!	\brief	Stores all objects of an objfile object definition
struct Object
{
	//!	Obejct name
	std::string m_strObjName;
	//!	Assigend face instances
	std::vector<Face*> m_Faces;
	//!	Transformation matrix, stored in OpenGL format
	aiMatrix4x4 m_Transformation;
	//!	All subobjects references by this object
	std::vector<Object*> m_SubObjects;

	//!	\brief	Default constructor
	Object() :
		m_strObjName("")
	{
		// empty
	}
	
	//!	\brief	Destructor	
	~Object()
	{
		for (std::vector<Object*>::iterator it = m_SubObjects.begin();
			it != m_SubObjects.end(); ++it)
		{
			delete *it;
		}
		m_SubObjects.clear();
	}
};

// ------------------------------------------------------------------------------------------------
//!	\struct	Material
//!	\brief	Data structure to store all material specific data
struct Material
{
	aiString MaterialName;
	aiString texture;
	aiColor3D ambient;
	aiColor3D diffuse;
	aiColor3D specular;
	float alpha;
	float shineness;
	int illumination_model;
};

// ------------------------------------------------------------------------------------------------
//!	\struct	Model
//!	\brief	Data structure to store all obj-specific model datas
struct Model
{
	typedef std::map<std::string*, std::vector<unsigned int>* > GroupMap;
	typedef std::map<std::string*, std::vector<unsigned int>* >::iterator GroupMapIt;
	typedef std::map<std::string*, std::vector<unsigned int>* >::const_iterator ConstGroupMapIt;

	//!	Model name
	std::string m_ModelName;
	//!	List ob assigned objects
	std::vector<Object*> m_Objects;
	//!	Pointer to current object
	ObjFile::Object *m_pCurrent;
	//!	Pointer to current material
	ObjFile::Material *m_pCurrentMaterial;
	//!	Pointer to default material
	ObjFile::Material *m_pDefaultMaterial;
	//!	Vector with all generated materials
	std::vector<std::string> m_MaterialLib;
	//!	Vector with all generated group
	std::vector<std::string> m_GroupLib;
	//!	Vector with all generated vertices
	std::vector<aiVector3D_t*> m_Vertices;
	//!	vector with all generated normals
	std::vector<aiVector3D_t*> m_Normals;
	//!	Groupmap
	GroupMap m_Groups;
	std::vector<unsigned int> *m_pGroupFaceIDs;
	//!	Active group
	std::string m_strActiveGroup;
	//!	Vector with generated texture coordinates
	std::vector<aiVector2D_t*> m_TextureCoord;
	//!	Material map
	std::map<std::string, Material*> m_MaterialMap;

	//!	\brief	Default constructor
	Model() :
		m_ModelName(""),
		m_pCurrent(NULL),
		m_pCurrentMaterial(NULL),
		m_pDefaultMaterial(NULL),
		m_strActiveGroup("")
	{
		// empty
	}
	
	//!	\brief	DEstructor
	~Model()
	{
		for (std::vector<Object*>::iterator it = m_Objects.begin();
		it != m_Objects.end(); ++it)
		{
			delete *it;
		}
		m_Objects.clear();
	}
};

} // Namespace ObjFile
} // Namespace Assimp

#endif
