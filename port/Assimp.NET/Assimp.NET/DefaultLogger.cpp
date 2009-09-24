
#include "mDefaultLogger.h"

namespace AssimpNET
{

AssimpNET::DefaultLogger::DefaultLogger(void)
{
	throw gcnew System::NotImplementedException();
}

AssimpNET::DefaultLogger::DefaultLogger(Assimp::DefaultLogger* native)
{
	this->p_native = native;
}

AssimpNET::DefaultLogger::~DefaultLogger(void)
{
	throw gcnew System::NotImplementedException();
}

bool AssimpNET::DefaultLogger::attachStream(LogStream^ stream, unsigned int severity)
{
	throw gcnew System::NotImplementedException();
}

bool AssimpNET::DefaultLogger::detachStream(LogStream^ stream, unsigned int severity)
{
	throw gcnew System::NotImplementedException();
}

Logger^ AssimpNET::DefaultLogger::create(const String^ name, LogSeverity severity, unsigned int defStream, IOSystem^ io)
{
	throw gcnew System::NotImplementedException();
}

Logger^ AssimpNET::DefaultLogger::get()
{
	throw gcnew System::NotImplementedException();
}

bool AssimpNET::DefaultLogger::isNullLogger()
{
	throw gcnew System::NotImplementedException();
}

void AssimpNET::DefaultLogger::kill()
{
	throw gcnew System::NotImplementedException();
}

void AssimpNET::DefaultLogger::set(Logger^ logger)
{
	throw gcnew System::NotImplementedException();
}

Assimp::DefaultLogger* AssimpNET::DefaultLogger::getNative()
{
	return this->p_native;
}

}//namespace