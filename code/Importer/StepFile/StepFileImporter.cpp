#include "StepFileImporter.h"

namespace Assimp {
namespace STEP {

static const aiImporterDesc desc = { "StepFile Importer",
                                "",
                                "",
                                "",
                                0,
                                0,
                                0,
                                0,
                                0,
                                "stp" };

StepFileImporter::StepFileImporter()
: BaseImporter() {

}

StepFileImporter::~StepFileImporter() {

}

bool StepFileImporter::CanRead(const std::string& file, IOSystem* pIOHandler, bool checkSig) const {
    const std::string &extension = GetExtension(file);
    if ( extension == "stp" ) {
        return true;
    } else if ((!extension.length() || checkSig) && pIOHandler) {
        const char* tokens[] = { "ISO-10303-21" };
        const bool found(SearchFileHeaderForToken(pIOHandler, file, tokens, 1));
        return found;
    }

    return false;
}

const aiImporterDesc *StepFileImporter::GetInfo() const {
    return &desc;
}

void StepFileImporter::InternReadFile(const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler) {

}

}
}
