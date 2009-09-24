
#include "Material.h"

namespace AssimpNET
{

Material::Material(void)
{
	this->p_native = new aiMaterial();
}

Material::Material(aiMaterial* native)
{
	this->p_native = native;
}

Material::~Material(void)
{
	if(this->p_native)
		delete this->p_native;
}

generic<typename T>
aiReturn Material::Get(array<char>^ pKey, unsigned int type, unsigned int idx, T pOut)
{
	throw gcnew System::NotImplementedException();
}

generic<typename T>
aiReturn Material::Get(array<char>^ pKey, unsigned int type, unsigned int idx, T pOut, unsigned int^ pMax)
{
	throw gcnew System::NotImplementedException();
}

aiReturn Material::GetTexture(TextureType type, unsigned int index, String^ path, TextureMapping& mapping, unsigned int^ uvindex, float^ blend,
					TextureOP& op, TextureMapMode& mapMode)
{
	throw gcnew System::NotImplementedException();
}

aiMaterial* Material::getNative()
{
	return this->p_native;
}

}//namespace