/** @file Definition of the .MD5 importer class. */
#ifndef AI_MD5LOADER_H_INCLUDED
#define AI_MD5LOADER_H_INCLUDED

#include "BaseImporter.h"
#include "../include/aiTypes.h"

struct aiNode;
#include "MD5FileData.h"

namespace Assimp
{
	class MaterialHelper;

	using namespace MD5;

	// ---------------------------------------------------------------------------
	/** Used to load MD5 files
	*/
	class MD5Importer : public BaseImporter
	{
		friend class Importer;

	protected:
		/** Constructor to be privately used by Importer */
		MD5Importer();

		/** Destructor, private as well */
		~MD5Importer();

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

		/** Header of the MD5 file */
		const MD5::Header* m_pcHeader;

		/** Buffer to hold the loaded file */
		const unsigned char* mBuffer;
	};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC