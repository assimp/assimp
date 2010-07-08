%{
#include "aiMaterial.h"
%}

ASSIMP_ARRAY(aiMaterial, aiMaterialProperty*, mProperties, $self->mNumProperties)

%include <typemaps.i>
%apply enum SWIGTYPE *OUTPUT { aiTextureMapping* mapping };
%apply unsigned int *OUTPUT { unsigned int* uvindex };
%apply float *OUTPUT { float* blend };
%apply enum SWIGTYPE *OUTPUT { aiTextureOp* op };

%include "aiMaterial.h"

%clear aiTextureOp* op;
%clear float *blend;
%clear unsigned int* uvindex;
%clear aiTextureMapping* mapping;


%apply int &OUTPUT { int &pOut };
%apply float &OUTPUT { float &pOut };

%template(GetInteger) aiMaterial::Get<int>;
%template(GetFloat) aiMaterial::Get<float>;
%template(GetColor4D) aiMaterial::Get<aiColor4D>;
%template(GetColor3D) aiMaterial::Get<aiColor3D>;
%template(GetString) aiMaterial::Get<aiString>;

%clear int &pOut;
%clear float &pOut;
