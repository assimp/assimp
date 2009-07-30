
#include "Quaternion.h"

namespace AssimpNET
{

Quaternion::Quaternion(void)
{
	throw gcnew System::NotImplementedException();
}

Quaternion::Quaternion(Vector3D^ normalized)
{
	throw gcnew System::NotImplementedException();
}

Quaternion::Quaternion(Vector3D^ axis, float angle)
{
	throw gcnew System::NotImplementedException();
}

Quaternion::Quaternion(float rotx, float roty, float rotz)
{
	throw gcnew System::NotImplementedException();
}

Quaternion::Quaternion(const Matrix3x3^ rotMatrix)
{
	throw gcnew System::NotImplementedException();
}

Quaternion::Quaternion(float _w, float _x, float _y, float _z)
{
	throw gcnew System::NotImplementedException();
}

Quaternion::~Quaternion(void)
{
	throw gcnew System::NotImplementedException();
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

}//namespace