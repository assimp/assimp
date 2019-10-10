#include "JTImporter.h"

namespace Assimp {

static const aiImporterDesc desc = {
    "JT File Format from Siemens",
    "",
    "",
    "",
    aiImporterFlags_SupportTextFlavour,
    0,
    0,
    0,
    0,
    "jt"
};

JTImporter::JTImporter()
: BaseImporter() {
    // empty
}

JTImporter::~JTImporter() {
    // empty
}

bool JTImporter::CanRead(const std::string &file, IOSystem *pIOHandler, bool checkSig) const {
    if (!checkSig) {
        //Check File Extension
        return SimpleExtensionCheck(file, "jt");
    }

    // Check file Header
    static const char *pTokens[] = { "mtllib", "usemtl", "v ", "vt ", "vn ", "o ", "g ", "s ", "f " };

    return BaseImporter::SearchFileHeaderForToken(pIOHandler, file, pTokens, 9, 200, false, true);
}

const aiImporterDesc *JTImporter::GetInfo() const {
    return &desc;
}

void JTImporter::InternReadFile(const std::string &file, aiScene *pScene, IOSystem *pIOHandler) {
    ai_assert(nullptr != pScene);
    ai_assert(nullptr != pIOHandler);


}

} //! namespace Assimp
