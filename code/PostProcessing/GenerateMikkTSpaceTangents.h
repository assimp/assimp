#pragma once

#include "Common/BaseProcess.h"
#include "contrib/MikkTSpace/mikktspace.h"

namespace Assimp {

class GenerateMikkTSpaceTangents final : public BaseProcess {
public:
    GenerateMikkTSpaceTangents() = default;
    ~GenerateMikkTSpaceTangents() override = default;
    bool IsActive(unsigned int pFlags) const override;
    void Execute(aiScene* pScene) override;
    void SetupProperties(const Importer *pImp) override;
    void ExecutePerMesh(aiMesh *mesh);

private:
    bool mActive = false;
    SMikkTSpaceInterface mIface;
    SMikkTSpaceContext mContext;
};

} // namespace Assimp
