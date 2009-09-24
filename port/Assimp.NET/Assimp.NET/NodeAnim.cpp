
#include "NodeAnim.h"

namespace AssimpNET
{

NodeAnim::NodeAnim(void)
{
	this->p_native = new aiNodeAnim();
}

NodeAnim::NodeAnim(aiNodeAnim* native)
{
	this->p_native = native;
}

NodeAnim::~NodeAnim(void)
{
	if(this->p_native)
		delete this->p_native;
}

aiNodeAnim* NodeAnim::getNative()
{
	return this->p_native;
}

}//namespace