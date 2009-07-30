
#include "Matrix4x4.h"

namespace AssimpNET
{

Matrix4x4::Matrix4x4(void)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4::Matrix4x4(const Matrix3x3 other)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4::Matrix4x4(	float _a1, float _a2, float _a3, float _a4,
						float _b1, float _b2, float _b3, float _b4,
						float _c1, float _c2, float _c3, float _c4,
						float _d1, float _d2, float _d3, float _d4)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4::~Matrix4x4(void)
{
	throw gcnew System::NotImplementedException();
}

void Matrix4x4::Decompose(Vector3D^ scaling, Quaternion^ rotation, Vector3D^ position)
{
	throw gcnew System::NotImplementedException();
}

void Matrix4x4::DecomposeNoScaling(Quaternion^ rotation, Vector3D^ position)
{
	throw gcnew System::NotImplementedException();
}

float Matrix4x4::Determinant()
{
	throw gcnew System::NotImplementedException();
}

void Matrix4x4::FromEulerAnglesXYZ(const Vector3D^ euler)
{
	throw gcnew System::NotImplementedException();
}

void Matrix4x4::FromEulerAnglesXYZ(float x, float y, float z)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::Inverse()
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::Transpose()
{
	throw gcnew System::NotImplementedException();
}

bool Matrix4x4::IsIdentity()
{
	throw gcnew System::NotImplementedException();
}

bool Matrix4x4::operator != (const Matrix4x4^ m)
{
	throw gcnew System::NotImplementedException();
}

bool Matrix4x4::operator == (const Matrix4x4^ m)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::operator* (const Matrix4x4^ m)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::operator*= (const Matrix4x4^ m)
{
	throw gcnew System::NotImplementedException();
}

float Matrix4x4::operator[](unsigned int i)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::FromToMatrix (const Vector3D^ from, const Vector3D^ to, Matrix4x4^ out)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::Rotation (float a, const Vector3D^ axis, Matrix4x4^ out)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::RotationX (float a, Matrix4x4^ out)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::RotationY (float a, Matrix4x4^ out)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::RotationZ (float a, Matrix4x4^ out)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::Scaling (const Vector3D^ v, Matrix4x4^ out)
{
	throw gcnew System::NotImplementedException();
}

Matrix4x4^ Matrix4x4::Translation (const Vector3D^ v, Matrix4x4^ out)
{
	throw gcnew System::NotImplementedException();
}

}//namespace