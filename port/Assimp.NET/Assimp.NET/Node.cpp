
#include "Node.h"

namespace AssimpNET
{

Node::Node(void)
{
	this->p_native = new aiNode();
}

Node::Node(aiNode* native)
{
	this->p_native = native;
}

Node::~Node(void)
{
	if(this->p_native)
		delete this->p_native;
}

Node^ Node::findNode(array<char>^ name)
{
	throw gcnew System::NotImplementedException();
}

Node^ Node::findNode(const String^ name)
{
	throw gcnew System::NotImplementedException();
}

aiNode* Node::getNative()
{
	return this->p_native;
}

}//namespace