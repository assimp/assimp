
#include "Texture.h"

namespace AssimpNET
{

Texture::Texture(void)
{
	this->p_native = new aiTexture();
}

Texture::Texture(aiTexture* native)
{
	this->p_native = native;
}

Texture::~Texture(void)
{
	if(this->p_native)
		delete this->p_native;
}

bool Texture::CheckFormat(array<char>^ s)
{
	throw gcnew System::NotImplementedException();
}

aiTexture* Texture::getNative()
{
	return this->p_native;
}

}//namespace