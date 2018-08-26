#pragma once

#include <assimp/BaseImporter.h>

namespace Assimp {
namespace STEP {

class StepFileImporter : public BaseImporter {
public:
    StepFileImporter();
    ~StepFileImporter();
    bool CanRead(const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const override;
    const aiImporterDesc* GetInfo() const override;

protected:
    void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler ) override;

private:
};

}
}
