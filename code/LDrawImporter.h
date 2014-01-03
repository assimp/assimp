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
#include "LineSplitter.h"

namespace Assimp{

	namespace LDraw{
		struct LDrawMaterial
		{
			LDrawMaterial(unsigned int code, aiColor3D color, aiColor3D edge) :
			code(code), color(color), edge(edge)
			{}
			//identification of the color in LDraw files
			unsigned int code;
			//the main color of the material
			aiColor3D color;
			//the contrast color of the material
			aiColor3D edge;
			//opacity
			float alpha = 1.0f;
			//factor of light emmision
			float luminance = 0.0f;
		};
	}

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

		// -------------------------------------------------------------------

		//try to read the LDraw materials from _libPath/ldconfig.ldr
		void ReadMaterials(std::string filename, IOSystem* pIOHandler);

		// -------------------------------------------------------------------

		//try to find the full path of file in the LDrawLibrary
		//returns "" if unsuccessful
		std::string FindPath(std::string subpath, IOSystem* pIOHandler);

		//path to the root folder of the library
		std::string _libPath = "";

		//container for the LDraw color definitions
		std::map<unsigned int, LDraw::LDrawMaterial> materials = std::map<unsigned int,LDraw::LDrawMaterial>();

	}; //end of class LDrawImporter

} // end of namespace Assimp
#endif // !INCLUDED_AI_LDR_IMPORTER_H