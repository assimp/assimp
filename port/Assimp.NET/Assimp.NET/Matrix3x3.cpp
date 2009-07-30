
#include "Matrix3x3.h"

namespace AssimpNET
{

Matrix3x3::Matrix3x3(void)
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3::Matrix3x3(const Matrix4x4^ matrix)
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3::Matrix3x3(	float _a1, float _a2, float _a3,
			float _b1, float _b2, float _b3,
			float _c1, float _c2, float _c3)
{
	throw gcnew System::NotImplementedException();
}

Matrix3x3::~Matrix3x3(void)
{
	throw gcnew System::NotImplementedException();
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


}//namespace