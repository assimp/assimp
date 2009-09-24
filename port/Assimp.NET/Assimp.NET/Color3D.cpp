
#include "Color3D.h"

namespace AssimpNET
{

Color3D::Color3D(void)
{
	this->p_native = new aiColor3D();
}

Color3D::Color3D(Color3D% other)
{
	this->p_native = other.getNative();
}

Color3D::Color3D(float _r, float _g, float _b)
{
	this->p_native = new aiColor3D(_r, _g, _b);
}

Color3D::Color3D(aiColor3D* native)
{
	this->p_native = native;
}

bool Color3D::IsBlack()
{
	throw gcnew System::NotImplementedException();
}

bool Color3D::operator != (const Color3D^ other)
{
	throw gcnew System::NotImplementedException();
}

Color3D^ Color3D::operator*(float f)
{
	throw gcnew System::NotImplementedException();
}

Color3D^ Color3D::operator*(const Color3D^ c)
{
	throw gcnew System::NotImplementedException();
}

Color3D^ Color3D::operator+ (const Color3D^ c)
{
	throw gcnew System::NotImplementedException();
}

Color3D^ Color3D::operator- (const Color3D^ c)
{
	throw gcnew System::NotImplementedException();
}

bool Color3D::operator== (const Color3D^ other)
{
	throw gcnew System::NotImplementedException();
}

float^ Color3D::operator[] (unsigned int i)
{
	throw gcnew System::NotImplementedException();
}

aiColor3D* Color3D::getNative()
{
	return this->p_native;
}

}
