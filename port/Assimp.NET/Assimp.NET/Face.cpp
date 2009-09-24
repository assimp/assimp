
#include "Face.h"

namespace AssimpNET
{

Face::Face(void)
{
	this->p_native = new aiFace();
}

Face::Face(Face% other)
{
	this->p_native = other.getNative();
}

Face::Face(aiFace* native)
{
	this->p_native = native;
}

Face::~Face(void)
{
	if(this->p_native)
		delete this->p_native;
}

bool Face::operator != (const Face^ other)
{
	throw gcnew System::NotImplementedException();
}

Face^ Face::operator = (const Face^ other)
{
	throw gcnew System::NotImplementedException();
}

bool Face::operator == (const Face^ other)
{
	throw gcnew System::NotImplementedException();
}

aiFace* Face::getNative()
{
	return this->p_native;
}


}//namespace