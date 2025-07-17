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
    void ExecutePerMesh(aiMesh *mesh);

private:
    SMikkTSpaceInterface mIface;
    SMikkTSpaceContext mContext;
};

} // namespace Assimp
