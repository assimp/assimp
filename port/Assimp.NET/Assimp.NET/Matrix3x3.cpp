
#include "Matrix3x3.h"

namespace AssimpNET
{

Matrix3x3::Matrix3x3(void)
{
	this->p_native = new aiMatrix3x3;
}

Matrix3x3::Matrix3x3(Matrix4x4^ matrix)
{
	this->p_native = new aiMatrix3x3(*(matrix->getNative()));
}

Matrix3x3::Matrix3x3(	float _a1, float _a2, float _a3,
			float _b1, float _b2, float _b3,
			float _c1, float _c2, float _c3)
{
	this->p_native = new aiMatrix3x3(_a1, _a2, _a3, _b1, _b2, _b3, _c1, _c2, _c3);
}

Matrix3x3::Matrix3x3(aiMatrix3x3* native)
{
	this->p_native = native;
}

Matrix3x3::~Matrix3x3(void)
{
	if(this->p_native)
		delete this->p_native;
}

float Matrix3x3::Determinant()
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3^ Matrix3x3::Inverse()
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3^ Matrix3x3::Transpose()
{
	throw gcnew System::NotImplementedException();
}

bool Matrix3x3::operator != (const Matrix3x3^ m)
{
	throw gcnew System::NotImplementedException();
}

bool Matrix3x3::operator == (const Matrix3x3^ m)
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3^ Matrix3x3::operator* (const Matrix3x3^ m)
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3^ Matrix3x3::operator*= (const Matrix3x3^ m)
{
	throw gcnew System::NotImplementedException();
}

float Matrix3x3::operator[](unsigned int i)
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3^ Matrix3x3::FromToMatrix(Vector3D^ from, Vector3D^ to, Matrix3x3^ out)
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3^ Matrix3x3::Rotation(float a, const Vector3D^ axis, Matrix3x3^ out)
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3^ Matrix3x3::RotationZ(float a, Matrix3x3^ out)
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3^ Matrix3x3::Translation(const Vector2D^ v, Matrix3x3^ out)
{
	throw gcnew System::NotImplementedException();
}

aiMatrix3x3* Matrix3x3::getNative()
{
	return this->p_native;
}


}//namespace