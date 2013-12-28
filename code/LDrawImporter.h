/** @file  LDrawImporter.h
 *  @brief Declaration of the LDraw importer class
 */
#ifndef INCLUDED_AI_LDR_IMPORTER_H
#define INCLUDED_AI_LDR_IMPORTER_H

#include "AssimpPCH.h"

namespace Assimp{

	class LDrawImporter : public BaseImporter
	{
	public:
		LDrawImporter();
		~LDrawImporter();

		// -------------------------------------------------------------------

		bool CanRead(const std::string& pFile, IOSystem* pIOHandler,
			bool checkSig) const;

	protected:
		// -------------------------------------------------------------------

		const aiImporterDesc* GetInfo() const;

		// -------------------------------------------------------------------

		void InternReadFile(const std::string& pFile, aiScene* pScene,
			IOSystem* pIOHandler);

		// -------------------------------------------------------------------

	}; //end of class LDrawImporter

} // end of namespace Assimp
#endif // !INCLUDED_AI_LDR_IMPORTER_H