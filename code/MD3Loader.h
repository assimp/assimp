/** @file Definition of the .MD3 importer class. */
#ifndef AI_MD3LOADER_H_INCLUDED
#define AI_MD3LOADER_H_INCLUDED

#include <map>

#include "BaseImporter.h"

#include "../include/aiTypes.h"

struct aiNode;

#include "MD3FileData.h"
namespace Assimp
{
	class MaterialHelper;

	using namespace MD3;

	// ---------------------------------------------------------------------------
	/** Used to load MD3 files
	*/
	class MD3Importer : public BaseImporter
	{
		friend class Importer;

	protected:
		/** Constructor to be privately used by Importer */
		MD3Importer();

		/** Destructor, private as well */
		~MD3Importer();

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

		/** Header of the MD3 file */
		const MD3::Header* m_pcHeader;

		/** Buffer to hold the loaded file */
		const unsigned char* mBuffer;
	};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC