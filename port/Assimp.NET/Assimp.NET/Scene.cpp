
#include "Scene.h"

namespace AssimpNET
{

Scene::Scene(void)
{
	this->p_native = new aiScene();
}

Scene::Scene(aiScene* native)
{
	this->p_native = native;
}

Scene::~Scene(void)
{
	if(this->p_native)
		delete this->p_native;
}

bool Scene::HasAnimations()
{
	throw gcnew System::NotImplementedException();
}

bool Scene::HasCameras()
{
	throw gcnew System::NotImplementedException();
}

bool Scene::HasLights()
{
	throw gcnew System::NotImplementedException();
}

bool Scene::HasMaterials()
{
	throw gcnew System::NotImplementedException();
}

bool Scene::HasMeshes()
{
	throw gcnew System::NotImplementedException();
}

bool Scene::HasTextures()
{
	throw gcnew System::NotImplementedException();
}

aiScene* Scene::getNative()
{
	return this->p_native;
}

}//namespace