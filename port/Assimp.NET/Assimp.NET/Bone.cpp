
#include "Bone.h"


namespace AssimpNET
{

Bone::Bone(void)
{
	this->p_native = new aiBone();
}

Bone::Bone(Bone% other)
{
	this->p_native = other.getNative();
}

Bone::Bone(aiBone* native)
{
	this->p_native = native;
}

Bone::~Bone(void)
{
	if(this->p_native)
		delete this->p_native;
}

aiBone* Bone::getNative()
{
	return this->p_native;
}

}//namespace