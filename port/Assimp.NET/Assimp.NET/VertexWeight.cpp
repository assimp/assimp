
#include "VertexWeight.h"

namespace AssimpNET
{

VertexWeight::VertexWeight(void)
{
	this->p_native = new aiVertexWeight();
}

VertexWeight::VertexWeight(unsigned int pID, float pWeight)
{
	this->p_native = new aiVertexWeight(pID, pWeight);
}

VertexWeight::VertexWeight(aiVertexWeight *native)
{
	this->p_native = native;
}

VertexWeight::~VertexWeight(void)
{
	if(this->p_native)
		delete this->p_native;
}

aiVertexWeight* VertexWeight::getNative()
{
	return this->p_native;
}

}//namespace
