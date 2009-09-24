
#include "Texel.h"

namespace AssimpNET
{

Texel::Texel(void)
{
	this->p_native = new aiTexel();
}

Texel::Texel(aiTexel* native)
{
	this->p_native = native;
}

Texel::~Texel(void)
{
	if(this->p_native)
		delete this->p_native;
}

Texel::operator Color4D()
{
	throw gcnew System::NotImplementedException();
}

bool Texel::operator != (const Texel^ t)
{
	throw gcnew System::NotImplementedException();
}

bool Texel::operator == (const Texel^ t)
{
	throw gcnew System::NotImplementedException();
}

aiTexel* Texel::getNative()
{
	return this->p_native;
}

}
