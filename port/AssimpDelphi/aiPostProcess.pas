unit aiPostProcess;

interface

const
  // The aiPostProcessSteps flags
  aiProcess_CalcTangentSpace         = $1;
  aiProcess_JoinIdenticalVertices    = $2;
  aiProcess_MakeLeftHanded           = $4;
  aiProcess_Triangulate              = $8;
  aiProcess_RemoveComponent          = $10;
  aiProcess_GenNormals               = $20;
  aiProcess_GenSmoothNormals         = $40;
  aiProcess_SplitLargeMeshes         = $80;
  aiProcess_PreTransformVertices     = $100;
  aiProcess_LimitBoneWeights         = $200;
  aiProcess_ValidateDataStructure    = $400;
  aiProcess_ImproveCacheLocality     = $800;
  aiProcess_RemoveRedundantMaterials = $1000;
  aiProcess_FixInfacingNormals       = $2000;
  aiProcess_PopulateArmatureData     = $4000;
  aiProcess_SortByPType              = $8000;
  aiProcess_FindDegenerates          = $10000;
  aiProcess_FindInvalidData          = $20000;
  aiProcess_GenUVCoords              = $40000;
  aiProcess_TransformUVCoords        = $80000;
  aiProcess_FindInstances            = $100000;
  aiProcess_OptimizeMeshes           = $200000;
  aiProcess_OptimizeGraph            = $400000;
  aiProcess_FlipUVs                  = $800000;
  aiProcess_FlipWindingOrder         = $1000000;
  aiProcess_SplitByBoneCount         = $2000000;
  aiProcess_Debone                   = $4000000;
  aiProcess_GlobalScale              = $8000000;
  aiProcess_EmbedTextures            = $10000000;
  aiProcess_ForceGenNormals          = $20000000;
  aiProcess_DropNormals              = $40000000;
  aiProcess_GenBoundingBoxes         = $80000000;

  aiProcess_ConvertToLeftHanded =
    aiProcess_MakeLeftHanded or
    aiProcess_FlipUVs or
    aiProcess_FlipWindingOrder;

  aiProcessPreset_TargetRealtime_Fast =
    aiProcess_CalcTangentSpace or
    aiProcess_GenNormals or
    aiProcess_JoinIdenticalVertices or
    aiProcess_Triangulate or
    aiProcess_GenUVCoords or
    aiProcess_SortByPType;

  aiProcessPreset_TargetRealtime_Quality =
    aiProcess_CalcTangentSpace or
    aiProcess_GenSmoothNormals or
    aiProcess_JoinIdenticalVertices or
    aiProcess_ImproveCacheLocality or
    aiProcess_LimitBoneWeights or
    aiProcess_RemoveRedundantMaterials or
    aiProcess_SplitLargeMeshes or
    aiProcess_Triangulate or
    aiProcess_GenUVCoords or
    aiProcess_SortByPType or
    aiProcess_FindDegenerates or
    aiProcess_FindInvalidData;

  aiProcessPreset_TargetRealtime_MaxQuality =
    aiProcessPreset_TargetRealtime_Quality or
    aiProcess_FindInstances or
    aiProcess_ValidateDataStructure or
    aiProcess_OptimizeMeshes;

implementation

end.
