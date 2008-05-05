/** @file Definition of the .MD2 importer class. */
#ifndef AI_MD2LOADER_H_INCLUDED
#define AI_MD2LOADER_H_INCLUDED

#include "BaseImporter.h"
#include "../include/aiTypes.h"

struct aiNode;
#include "MD2FileData.h"

namespace Assimp
{
	class MaterialHelper;

	using namespace MD2;

	// ---------------------------------------------------------------------------
	/** Used to load MD2 files
	*/
	class MD2Importer : public BaseImporter
	{
		friend class Importer;

	protected:
		/** Constructor to be privately used by Importer */
		MD2Importer();

		/** Destructor, private as well */
		~MD2Importer();

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

		/** Header of the MD2 file */
		const MD2::Header* m_pcHeader;

		/** Buffer to hold the loaded file */
		const unsigned char* mBuffer;
	};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC