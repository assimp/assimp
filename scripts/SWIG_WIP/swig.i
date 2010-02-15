

%module assimp
%{
%}

#define C_STRUCT
#define C_ENUM
#define ASSIMP_API
#define AI_FORCE_INLINE

%include "../../include/aiAnim.h"
%include "../../include/aiCamera.h"
%include "../../include/aiColor4D.h"
%include "../../include/aiLight.h"
//%include "../../include/aiMaterial.h"
%include "../../include/aiMatrix3x3.h"
%include "../../include/aiMatrix4x4.h"
%include "../../include/aiMesh.h"
%include "../../include/aiPostProcess.h"
%include "../../include/aiQuaternion.h"
%include "../../include/aiScene.h"
%include "../../include/aiTexture.h"
%include "../../include/aiTypes.h"
%include "../../include/aiVector2D.h"
%include "../../include/aiVector3D.h"
//%include "../../include/aiVersion.h"
%include "../../include/assimp.hpp"
%include "../../include/DefaultLogger.h"
%include "../../include/IOStream.h"
%include "../../include/IOSystem.h"
%include "../../include/Logger.h"
%include "../../include/LogStream.h"
%include "../../include/NullLogger.h"


