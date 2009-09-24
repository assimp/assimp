
#include "Animation.h"

namespace AssimpNET
{

Animation::Animation(void)
{
	this->p_native = new aiAnimation();
}

Animation::Animation(aiAnimation* native)
{
	this->p_native = native;
}

Animation::~Animation(void)
{
	if(this->p_native)
		delete this->p_native;
}

aiAnimation* Animation::getNative()
{
	return this->p_native;
}

}//namespace