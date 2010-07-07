%{
#include "aiMaterial.h"
%}

ASSIMP_ARRAY(aiMaterial, aiMaterialProperty*, mProperties, $self->mNumProperties)

%include "aiMaterial.h"

%include <typemaps.i>
%apply int &OUTPUT { int &pOut };
%apply float &OUTPUT { float &pOut };

%template(GetInteger) aiMaterial::Get<int>;
%template(GetFloat) aiMaterial::Get<float>;
%template(GetColor4D) aiMaterial::Get<aiColor4D>;
%template(GetColor3D) aiMaterial::Get<aiColor3D>;
%template(GetString) aiMaterial::Get<aiString>;

%clear int &pOut;
%clear float &pOut;
