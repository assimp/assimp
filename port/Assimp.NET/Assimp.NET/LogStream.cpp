
#include "mLogStream.h"

namespace AssimpNET
{

LogStream::LogStream(void)
{
	throw gcnew System::NotImplementedException();
}

LogStream::LogStream(Assimp::LogStream* native)
{
	this->p_native = native;
}

LogStream::~LogStream(void)
{
	if(this->p_native)
		delete this->p_native;
}

LogStream^ LogStream::createDefaultStream(DefaulLogStreams streams, array<char>^ name, IOSystem^ io)
{
	throw gcnew System::NotImplementedException();
}

Assimp::LogStream* LogStream::getNative()
{
	return this->p_native;
}

}//namespace