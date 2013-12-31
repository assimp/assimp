/** @file  LDrawImporter.h
 *  @brief Declaration of the LDraw importer class
 */
#ifndef INCLUDED_AI_LDR_IMPORTER_H
#define INCLUDED_AI_LDR_IMPORTER_H

#include "AssimpPCH.h"
#include "ParsingUtils.h"
#include "fast_atof.h"
#include "TinyFormatter.h"
#include "Importer.h"
#include "GenericProperty.h"
#include "SceneCombiner.h"

namespace Assimp{

	class LDrawImporter : public BaseImporter
	{
	public:
		LDrawImporter();
		~LDrawImporter();

		// -------------------------------------------------------------------

		bool CanRead(const std::string& pFile, IOSystem* pIOHandler,
			bool checkSig) const;

		// -------------------------------------------------------------------

		const aiImporterDesc* GetInfo() const;

		// -------------------------------------------------------------------

		void SetupProperties(const Importer* pImp);

		// -------------------------------------------------------------------
	protected:
		void InternReadFile(const std::string& pFile, aiScene* pScene,
			IOSystem* pIOHandler);

		// -------------------------------------------------------------------

	private:

		// Read num floats from the buffer and store it in the out array
		bool ReadNumFloats(const char* line, float* &out, unsigned int num);

		// -------------------------------------------------------------------

		//throw an DeadlyImportError with the specified Message
		void ThrowException(const std::string &Message){
			throw DeadlyImportError("LDraw: " + Message);
		}

		std::string _libPath = "";

	}; //end of class LDrawImporter

} // end of namespace Assimp
#endif // !INCLUDED_AI_LDR_IMPORTER_H