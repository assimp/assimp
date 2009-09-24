
#include "Vector3D.h"

namespace AssimpNET
{

Vector3D::Vector3D(void)
{
	this->p_native = new aiVector3D();
}

Vector3D::Vector3D(Vector3D% o)
{
	this->p_native = new aiVector3D(*(o.getNative()));
}

Vector3D::Vector3D(float _xyz)
{
	this->p_native = new aiVector3D(_xyz);
}

Vector3D::Vector3D(float _x, float _y, float _z)
{
	this->p_native = new aiVector3D(_x, _y, _z);
}

Vector3D::Vector3D(aiVector3D* native)
{
	this->p_native = native;
}

Vector3D::~Vector3D(void)
{
	if(this->p_native)
		delete this->p_native;
}

float Vector3D::Length()
{
	throw gcnew System::NotImplementedException();
}

Vector3D^ Vector3D::Normalize()
{
	throw gcnew System::NotImplementedException();
}

bool Vector3D::operator!= (const Vector3D^ other)
{
	throw gcnew System::NotImplementedException();
}

Vector3D^ Vector3D::operator*= (const Matrix4x4^ mat)
{
	throw gcnew System::NotImplementedException();
}

Vector3D^ Vector3D::operator*= (const Matrix3x3^ mat)
{
	throw gcnew System::NotImplementedException();
}

Vector3D^ Vector3D::operator*= (float f)
{
	throw gcnew System::NotImplementedException();
}

Vector3D^ Vector3D::operator+= (const Vector3D^ o)
{
	throw gcnew System::NotImplementedException();
}

Vector3D^ Vector3D::operator-= (const Vector3D^ o)
{
	throw gcnew System::NotImplementedException();
}

Vector3D^ Vector3D::operator/= (float f)
{
	throw gcnew System::NotImplementedException();
}

bool Vector3D::operator== (const Vector3D^ other)
{
	throw gcnew System::NotImplementedException();
}

float Vector3D::operator[] (unsigned int i)
{
	throw gcnew System::NotImplementedException();
}

void Vector3D::Set (float pX, float pY, float pZ)
{
	throw gcnew System::NotImplementedException();
}

float Vector3D::SquareLength()
{
	throw gcnew System::NotImplementedException();
}

Vector3D^ Vector3D::SymMul(const Vector3D^ o)
{
	throw gcnew System::NotImplementedException();
}

aiVector3D* Vector3D::getNative()
{
	return this->p_native;
}

}