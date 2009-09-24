
#include "VectorKey.h"

namespace AssimpNET
{

VectorKey::VectorKey(void)
{
	this->p_native = new aiVectorKey();
}

VectorKey::VectorKey(double time, Vector3D^ value)
{
	this->p_native = new aiVectorKey(time, *(value->getNative()));
}

VectorKey::VectorKey(aiVectorKey* native)
{
	this->p_native = native;
}

VectorKey::~VectorKey(void)
{
	if(this->p_native)
		delete this->p_native;
}

bool VectorKey::operator != (const VectorKey^ o)
{
	throw gcnew System::NotImplementedException();
}

bool VectorKey::operator < (const VectorKey^ o)
{
	throw gcnew System::NotImplementedException();
}

bool VectorKey::operator == (const VectorKey^ o)
{
	throw gcnew System::NotImplementedException();
}

bool VectorKey::operator > (const VectorKey^ o)
{
	throw gcnew System::NotImplementedException();
}

aiVectorKey* VectorKey::getNative()
{
	return this->p_native;
}

}
