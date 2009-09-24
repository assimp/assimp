
#include "Importer.h"

namespace AssimpNET
{

Importer::Importer(void)
{
	p_native = new Assimp::Importer();
}

Importer::Importer(Importer% other)
{
	p_native = other.getNative();
}

Importer::Importer(Assimp::Importer* native)
{
	this->p_native = native;
}

Importer::~Importer(void)
{
	if(this->p_native)
		delete this->p_native;
}

void Importer::FreeScene()
{
	p_native->FreeScene();
}

System::String^ Importer::GetErrorString()
{
	return gcnew System::String(p_native->GetErrorString());
}

void Importer::GetExtensionsList(String^ extensions)
{
	throw gcnew System::NotImplementedException();
}

IOSystem^ Importer::GetIOHandler()
{
	throw gcnew System::NotImplementedException();
}

void Importer::GetMemoryRequrements(aiMemoryInfo^ in)
{
	throw gcnew System::NotImplementedException();
}

Scene^ Importer::getOrphanedScene( )
{
	throw gcnew System::NotImplementedException();
}

float Importer::GetPropertyFloat(String^ propName)
{
	throw gcnew System::NotImplementedException();
	//return p_native->GetPropertyFloat(propName->ToCharArray());
}

int Importer::GetPropertyInt(String^ propName)
{
	throw gcnew System::NotImplementedException();
	//return p_native->GetPropertyInteger((IntPtr)propName->ToCharArray());
}

String^ Importer::GetPrpertyString(String^ propName)
{
	throw gcnew System::NotImplementedException();
	//return System::String(p_native->GetPropertyString(propName->ToCharArray()));
}

Scene^ Importer::getScene()
{
	throw gcnew System::NotImplementedException();
}

bool Importer::IsDefaultIOHandler()
{
	return p_native->IsDefaultIOHandler();
}

bool Importer::IsExtensionSupported(String^ extension)
{
	throw gcnew System::NotImplementedException();
}

Scene^ Importer::ReadFile(String^ fileName, unsigned int flags)
{
	throw gcnew System::NotImplementedException();
}

void Importer::SetExtraVerbose(bool verbose)
{
	p_native->SetExtraVerbose(verbose);
}

void Importer::SetIOHanlder(IOSystem^ ioHandler)
{
	throw gcnew System::NotImplementedException();
}

void Importer::SetPropertyFloat(String^ propName, float value)
{
	throw gcnew System::NotImplementedException();
}

void Importer::SetPropertyInt(String^ propName, int value)
{
	throw gcnew System::NotImplementedException();
}

void Importer::SetPrpertyString(String^ propName, String^ value)
{
	throw gcnew System::NotImplementedException();
}

bool Importer::ValidateFlags(unsigned int flags)
{
	return p_native->ValidateFlags(flags);
}

Assimp::Importer* Importer::getNative()
{
	return this->p_native;
}



}//namespace