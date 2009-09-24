#include "QuatKey.h"

namespace AssimpNET
{

QuatKey::QuatKey(void)
{
	this->p_native = new aiQuatKey();
}

QuatKey::QuatKey(double time, Quaternion% value)
{
	this->p_native = new aiQuatKey(time, *(value.getNative()));
}

QuatKey::QuatKey(aiQuatKey* native)
{
	this->p_native = native;
}

QuatKey::~QuatKey()
{
	if(this->p_native)
		delete this->p_native;
}

bool QuatKey::operator != (const QuatKey^ o)
{
	throw gcnew System::NotImplementedException();
}

bool QuatKey::operator < (const QuatKey^ o)
{
	throw gcnew System::NotImplementedException();
}

bool QuatKey::operator == (const QuatKey^ o)
{
	throw gcnew System::NotImplementedException();
}

bool QuatKey::operator > (const QuatKey^ o)
{
	throw gcnew System::NotImplementedException();
}

aiQuatKey* QuatKey::getNative()
{
	return this->p_native;
}

}