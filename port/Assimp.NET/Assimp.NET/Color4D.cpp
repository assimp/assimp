
#include "Color4D.h"

namespace AssimpNET
{

Color4D::Color4D(void)
{
	this->p_native = new aiColor4D();
}

Color4D::Color4D(Color4D% other)
{
	this->p_native = other.getNative();
}

Color4D::Color4D(float _r, float _g, float _b, float _a)
{
	this->p_native = new aiColor4D(_r, _g, _b, _a);
}

Color4D::Color4D(aiColor4D *native)
{
	this->p_native = native;
}

bool Color4D::IsBlack()
{
	throw gcnew System::NotImplementedException();
}

bool Color4D::operator!= (const Color4D^ other)
{
	throw gcnew System::NotImplementedException();
}

bool Color4D::operator== (const Color4D^ other)
{
	throw gcnew System::NotImplementedException();
}

float Color4D::operator[] (unsigned int i)
{
	throw gcnew System::NotImplementedException();
}

aiColor4D* Color4D::getNative()
{
	return this->p_native;
}

}//namespace