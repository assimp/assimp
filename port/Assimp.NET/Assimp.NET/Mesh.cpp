
#include "Mesh.h"

namespace AssimpNET
{

Mesh::Mesh(void)
{
	this->p_native = new aiMesh();
}

Mesh::Mesh(aiMesh* native)
{
	this->p_native = native;
}

Mesh::~Mesh(void)
{
	if(this->p_native)
		delete this->p_native;
}

unsigned int Mesh::GetNumColorChannels()
{
	throw gcnew System::NotImplementedException();
}

unsigned int Mesh::GetNumUVChannels()
{
	throw gcnew System::NotImplementedException();
}

bool Mesh::HasBones()
{
	throw gcnew System::NotImplementedException();
}

bool Mesh::HasFaces()
{
	throw gcnew System::NotImplementedException();
}

bool Mesh::HasNormals()
{
	throw gcnew System::NotImplementedException();
}

bool Mesh::HasPositions()
{
	throw gcnew System::NotImplementedException();
}

bool Mesh::HasTangentsAndBitangents()
{
	throw gcnew System::NotImplementedException();
}

bool Mesh::HasTextureCoords()
{
	throw gcnew System::NotImplementedException();
}

bool Mesh::HasVertexColors()
{
	throw gcnew System::NotImplementedException();
}

aiMesh* Mesh::getNative()
{
	return this->p_native;
}

}//namespace