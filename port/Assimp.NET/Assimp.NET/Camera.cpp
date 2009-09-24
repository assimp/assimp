
#include "Camera.h"

namespace AssimpNET
{

Camera::Camera(void)
{
	this->p_native = new aiCamera();
}

Camera::Camera(aiCamera* native)
{
	this->p_native = native;
}

Camera::~Camera(void)
{
	if(this->p_native)
		delete this->p_native;
}

void Camera::GetCameraMatrix(Matrix4x4^ out)
{
	throw gcnew System::NotImplementedException();
}

aiCamera* Camera::getNative()
{
	return this->p_native;
}

}//namespace