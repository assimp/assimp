
#include "Light.h"

namespace AssimpNET
{

Light::Light(void)
{
	this->p_native = new aiLight();
}

Light::Light(aiLight* native)
{
	this->p_native = native;
}

Light::~Light(void)
{
	if(this->p_native)
		delete this->p_native;
}

aiLight* Light::getNative()
{
	return this->p_native;
}

}//namespace