
#include "MaterialProperty.h"

namespace AssimpNET
{

MaterialProperty::MaterialProperty(void)
{
	this->p_native = new aiMaterialProperty();
}

MaterialProperty::MaterialProperty(aiMaterialProperty* native)
{
	this->p_native = native;
}

MaterialProperty::~MaterialProperty(void)
{
	if(this->p_native)
		delete this->p_native;
}

aiMaterialProperty* MaterialProperty::getNative()
{
	return this->p_native;
}

}//namespace