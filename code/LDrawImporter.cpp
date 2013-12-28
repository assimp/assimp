
/** @file  LDrawImporter.cpp
 *  @brief Implementation of the LDraw importer.
 */
#ifndef ASSIMP_BUILD_NO_LDR_IMPORTER
#include "LDrawImporter.h"

using namespace Assimp;

static const aiImporterDesc desc = {
	"LDraw Importer",
	"Tobias 'diiigle' Rittig",
	"",
	"ignoring Linetype 5 'optional lines'",
	0,
	0,
	0,
	0,
	0,
	"ldr dat mpd"
};

LDrawImporter::LDrawImporter()
{
}

LDrawImporter::~LDrawImporter()
{
}

// -------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool LDrawImporter::CanRead(const std::string& pFile, IOSystem* pIOHandler,
	bool checkSig) const
{
	const std::string extension = GetExtension(pFile);
	if (extension == "ldr" || extension == "dat" || extension == "mpd") {
		return true;
	}
	if (!extension.length() || checkSig) {
		const char* tokens[] = { "0 !LDRAW_ORG", "0 !LICENSE" };
		return SearchFileHeaderForToken(pIOHandler, pFile, tokens, 2);
	}
	return false;
}
// -------------------------------------------------------------------------------
const aiImporterDesc* LDrawImporter::GetInfo() const {
	return &desc;
}
// -------------------------------------------------------------------------------
void LDrawImporter::InternReadFile(const std::string& pFile,
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file(pIOHandler->Open(pFile, "rb"));
	// Check whether we can read from the file
	if (file.get() == NULL) {
		throw DeadlyImportError("Failed to open LDraw file " + pFile + ".");
	}

	// Your task: fill pScene
	// Throw a ImportErrorException with a meaningful (!) error message if 
	// something goes wrong.
	std::vector<char> buffer;
	TextFileToBuffer(file.get(), buffer);
}

#endif // !ASSIMP_BUILD_NO_LDR_IMPORTER