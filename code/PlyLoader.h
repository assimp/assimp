/** @file Definition of the .ply importer class. */
#ifndef AI_PLYLOADER_H_INCLUDED
#define AI_PLYLOADER_H_INCLUDED

#include "BaseImporter.h"
#include "../include/aiTypes.h"

struct aiNode;

#include "PlyParser.h"

namespace Assimp
{
	class MaterialHelper;


	using namespace PLY;

	// ---------------------------------------------------------------------------
	/** Used to load PLY files
	*/
	class PLYImporter : public BaseImporter
	{
		friend class Importer;

	protected:
		/** Constructor to be privately used by Importer */
		PLYImporter();

		/** Destructor, private as well */
		~PLYImporter();

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

	protected:


		// -------------------------------------------------------------------
		/** Extract vertices from the DOM
		*/
		void LoadVertices(std::vector<aiVector3D>* pvOut,bool p_bNormals = false);

		// -------------------------------------------------------------------
		/** Extract vertex color channels
		*/
		void LoadVertexColor(std::vector<aiColor4D>* pvOut);

		// -------------------------------------------------------------------
		/** Extract a face list from the DOM
		*/
		void LoadFaces(std::vector<PLY::Face>* pvOut);

		// -------------------------------------------------------------------
		/** Extract a material list from the DOM
		*/
		void LoadMaterial(std::vector<MaterialHelper*>* pvOut);


		// -------------------------------------------------------------------
		/** Validate material indices, replace default material identifiers
		*/
		void ReplaceDefaultMaterial(std::vector<PLY::Face>* avFaces,
			std::vector<MaterialHelper*>* avMaterials);


		// -------------------------------------------------------------------
		/** Convert all meshes into our ourer representation
		*/
		void ConvertMeshes(std::vector<PLY::Face>* avFaces,
			const std::vector<aiVector3D>* avPositions,
			const std::vector<aiVector3D>* avNormals,
			const std::vector<aiColor4D>* avColors,
			const std::vector<MaterialHelper*>* avMaterials,
			std::vector<aiMesh*>* avOut);


		/** Buffer to hold the loaded file */
		unsigned char* mBuffer;

		/** Document object model representation extracted from the file */
		PLY::DOM* pcDOM;
	};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC