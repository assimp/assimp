
#include "mIOSystem.h"

namespace AssimpNET
{

IOSystem::IOSystem(void)
{
	throw gcnew System::NotImplementedException();
}

IOSystem::IOSystem(Assimp::IOSystem* native)
{
	this->p_native = native;
}

IOSystem::~IOSystem(void)
{
	if(this->p_native)
		delete this->p_native;
}

bool IOSystem::ComparePaths (const String^ one, const String^ second)
{
	throw gcnew System::NotImplementedException();
}

bool IOSystem::ComparePaths (array<char>^ one, array<char>^ second)
{
	throw gcnew System::NotImplementedException();
}

bool IOSystem::Exists(const String^ pFile)
{
	throw gcnew System::NotImplementedException();
}

IOStream^ IOSystem::Open(const String^ pFile, const String^ pMode)
{
	throw gcnew System::NotImplementedException();
}

Assimp::IOSystem* IOSystem::getNative()
{
	return this->p_native;
}

}//namespace