/** @file Definition of the XFile importer class. */
#ifndef AI_XFILEIMPORTER_H_INC
#define AI_XFILEIMPORTER_H_INC

#include <map>

#include "XFileHelper.h"
#include "BaseImporter.h"

#include "../include/aiTypes.h"

struct aiNode;

namespace Assimp
{
struct XFile::Scene;
struct XFile::Node;

// ---------------------------------------------------------------------------
/** The XFileImporter is a worker class capable of importing a scene from a
 * DirectX file .x
 */
class XFileImporter : public BaseImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	XFileImporter();

	/** Destructor, private as well */
	~XFileImporter();

public:
	// -------------------------------------------------------------------
	/** Returns whether the class can handle the format of the given file. 
	 * See BaseImporter::CanRead() for details.	*/
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler) const;

protected:
	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	 * See BaseImporter::InternReadFile() for details
	 */
	void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler);

	// -------------------------------------------------------------------
	/** Constructs the return data structure out of the imported data.
	 * @param pScene The scene to construct the return data in.
	 * @param pData The imported data in the internal temporary representation.
	 */
	void CreateDataRepresentationFromImport( aiScene* pScene, const XFile::Scene* pData);

	// -------------------------------------------------------------------
	/** Recursively creates scene nodes from the imported hierarchy. The meshes and materials
	 * of the nodes will be extracted on the way.
	 * @param pScene The scene to construct the return data in.
	 * @param pParent The parent node where to create new child nodes
	 * @param pNode The temporary node to copy.
	 * @return The created node 
	 */
	aiNode* CreateNodes( aiScene* pScene, aiNode* pParent, const XFile::Node* pNode);

	// -------------------------------------------------------------------
	/** Converts all meshes in the given mesh array. Each mesh is splitted up per material,
	 * the indices of the generated meshes are stored in the node structure.
	 * @param pScene The scene to construct the return data in.
	 * @param pNode The target node structure that references the constructed meshes.
	 * @param pMeshes The array of meshes to convert
	 */
	void CreateMeshes( aiScene* pScene, aiNode* pNode, const std::vector<XFile::Mesh*>& pMeshes);

	// -------------------------------------------------------------------
	/** Converts the animations from the given imported data and creates them in the scene.
	 * @param pScene The scene to hold to converted animations
	 * @param pData The data to read the animations from
	 */
	void CreateAnimations( aiScene* pScene, const XFile::Scene* pData);

	// -------------------------------------------------------------------
	/** Converts all materials in the given array and stores them in the scene's material list.
	 * @param pScene The scene to hold the converted materials.
	 * @param pMaterials The material array to convert.
	 */
	void ConvertMaterials( aiScene* pScene, const std::vector<XFile::Material>& pMaterials);

protected:
	/** Buffer to hold the loaded file */
	std::vector<char> mBuffer;

	/** Imported materials: index in the scene's material list by name */
	std::map<std::string, unsigned int> mImportedMats;
};

} // end of namespace Assimp

#endif // AI_BASEIMPORTER_H_INC