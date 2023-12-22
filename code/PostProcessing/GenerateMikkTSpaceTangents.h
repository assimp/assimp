#pragma once

#include "Common/BaseProcess.h"

namespace Assimp {

class GenerateMikkTSpaceTangents : public BaseProcess {
public:
    GenerateMikkTSpaceTangents() = default;
    ~GenerateMikkTSpaceTangents() override = default;
    bool IsActive(unsigned int pFlags) const override;
    void Execute(aiScene* pScene) override;

};

}

