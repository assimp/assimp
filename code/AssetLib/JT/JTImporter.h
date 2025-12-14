#pragma once

#include <assimp/BaseImporter.h>

namespace Assimp::JT {

    
class JTImporter : public BaseImporter {
public:
    JTImporter();
    ~JTImporter() override;
    bool CanRead(const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const override;
    void setupProperties(const Importer* pImp) override;

protected:
    void InternReadFile(const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler) override;
}
