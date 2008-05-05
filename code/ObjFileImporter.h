#ifndef OBJ_FILE_IMPORTER_H_INC
#define OBJ_FILE_IMPORTER_H_INC

#include "BaseImporter.h"
#include <vector>

struct aiMesh;
struct aiNode;

namespace Assimp
{

namespace ObjFile
{
struct Object;
struct Model;
}

///	\class	ObjFileImporter
///	\brief	IMports a waveform obj file
class ObjFileImporter :
	BaseImporter
{	
	friend class Importer;

	//!	OB file extention
	static const std::string OBJ_EXT;

protected:
	///	\brief	Default constructor
	ObjFileImporter();

	///	\brief	Destructor
	~ObjFileImporter();

public:
	/// \brief	Returns whether the class can handle the format of the given file. 
	/// \remark	See BaseImporter::CanRead() for details.
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler) const;

private:
	//!	\brief
	void InternReadFile(const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler);
	
	//!	\brief
	void CreateDataFromImport(const ObjFile::Model* pModel, aiScene* pScene);
	
	//!	\brief
	aiNode *createNodes(const ObjFile::Model* pModel, const ObjFile::Object* pData, 
		aiNode *pParent, aiScene* pScene, std::vector<aiMesh*> &MeshArray);

	//!	\brief
	void createTopology(const ObjFile::Model* pModel, const ObjFile::Object* pData,
		aiMesh* pMesh);	
	
	//!	\brief
	void createVertexArray(const ObjFile::Model* pModel, 
		const ObjFile::Object* pCurrentObject, aiMesh* pMesh);

	//!	\brief
	void countObjects(const std::vector<ObjFile::Object*> &rObjects, int &iNumMeshes);

	//!	\brief
	void createMaterial(const ObjFile::Model* pModel, const ObjFile::Object* pData, 
		aiScene* pScene);

	//!	\brief
	void appendChildToParentNode(aiNode *pParent, aiNode *pChild);

private:
	std::vector<char> m_Buffer;
	ObjFile::Object *m_pRootObject;
	std::string m_strAbsPath;
};

} // Namespace Assimp

#endif
