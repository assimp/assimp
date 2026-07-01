#pragma once

#include "Common/BaseProcess.h"
#include "contrib/MikkTSpace/mikktspace.h"

struct aiMesh;

namespace Assimp {

class ASSIMP_API GenerateMikkTSpaceTangents final : public BaseProcess {
public:
    GenerateMikkTSpaceTangents() = default;
    ~GenerateMikkTSpaceTangents() final = default;
    bool IsActive(unsigned int pFlags) const final;
    void Execute(aiScene* pScene) final;
    void SetupProperties(const Importer *pImp) final;
    void ExecutePerMesh(aiMesh *mesh);

private:
    bool mActive = false;
    SMikkTSpaceInterface mIface{};
    SMikkTSpaceContext mContext;
};

} // namespace Assimp
