
#include "Vector2D.h"

namespace AssimpNET
{

Vector2D::Vector2D(void)
{
	this->p_native = new aiVector2D();
}

Vector2D::Vector2D(Vector2D% o)
{
	this->p_native = new aiVector2D(*(o.getNative()));
}

Vector2D::Vector2D(float _xy)
{
	this->p_native = new aiVector2D(_xy);
}

Vector2D::Vector2D(float _x, float _y)
{
	this->p_native = new aiVector2D(_x, _y);
}

Vector2D::Vector2D(aiVector2D* native)
{
	this->p_native = native;
}

Vector2D::~Vector2D(void)
{
	if(this->p_native)
		delete this->p_native;
}

float Vector2D::Length()
{
	throw gcnew System::NotImplementedException();
}

Vector2D^ Vector2D::Normalize()
{
	throw gcnew System::NotImplementedException();
}

bool Vector2D::operator!= (const Vector2D^ other)
{
	throw gcnew System::NotImplementedException();
}

Vector2D^ Vector2D::operator*= (float f)
{
	throw gcnew System::NotImplementedException();
}

Vector2D^ Vector2D::operator+= (const Vector2D^ o)
{
	throw gcnew System::NotImplementedException();
}

Vector2D^ Vector2D::operator-= (const Vector2D^ o)
{
	throw gcnew System::NotImplementedException();
}

Vector2D^ Vector2D::operator/= (float f)
{
	throw gcnew System::NotImplementedException();
}

Vector2D^ Vector2D::operator= (float f)
{
	throw gcnew System::NotImplementedException();
}

bool Vector2D::operator== (const Vector2D^ other)
{
	throw gcnew System::NotImplementedException();
}

float Vector2D::operator[] (unsigned int i)
{
	throw gcnew System::NotImplementedException();
}

void Vector2D::Set(float pX, float pY)
{
	throw gcnew System::NotImplementedException();
}

float Vector2D::SquareLength()
{
	throw gcnew System::NotImplementedException();
}

Vector2D^ Vector2D::SymMul(const Vector2D^ o)
{
	throw gcnew System::NotImplementedException();
}

aiVector2D* Vector2D::getNative()
{
	return this->p_native;
}


}