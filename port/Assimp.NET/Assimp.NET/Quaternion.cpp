
#include "Quaternion.h"
#include "Matrix3x3.h"

namespace AssimpNET
{

Quaternion::Quaternion(void)
{
	this->p_native = new aiQuaternion();
}

Quaternion::Quaternion(Vector3D^ normalized)
{
	this->p_native = new aiQuaternion(*(normalized->getNative()));
}

Quaternion::Quaternion(Vector3D^ axis, float angle)
{
	this->p_native = new aiQuaternion(*(axis->getNative()), angle);
}

Quaternion::Quaternion(float rotx, float roty, float rotz)
{
	this->p_native = new aiQuaternion(rotx, roty, rotz);
}

Quaternion::Quaternion(Matrix3x3^ rotMatrix)
{
	this->p_native = new aiQuaternion((*rotMatrix->getNative()));
}

Quaternion::Quaternion(float _w, float _x, float _y, float _z)
{
	this->p_native = new aiQuaternion(_w, _x, _y, _z);
}

Quaternion::Quaternion(aiQuaternion* native)
{
	this->p_native = native;
}

Quaternion::~Quaternion(void)
{
	if(this->p_native)
		delete this->p_native;
}

Quaternion^ Quaternion::Conjugate()
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3^ Quaternion::GetMatrix()
{
	throw gcnew System::NotImplementedException();
}

Quaternion^ Quaternion::Nomalize()
{
	throw gcnew System::NotImplementedException();
}

bool Quaternion::operator != (const Quaternion^ q)
{
	throw gcnew System::NotImplementedException();
}

bool Quaternion::operator == (const Quaternion^ q)
{
	throw gcnew System::NotImplementedException();
}

Quaternion^ Quaternion::operator* (const Quaternion^ q)
{
	throw gcnew System::NotImplementedException();
}

Vector3D^ Quaternion::Rotate(const Vector3D^ in)
{
	throw gcnew System::NotImplementedException();
}

void Quaternion::Interpolate(Quaternion^ pOut, const Quaternion^ pStart, const Quaternion^ pEnd, float factor)
{
	throw gcnew System::NotImplementedException();
}

aiQuaternion* Quaternion::getNative()
{
	return this->p_native;
}

}//namespace