
#include "DefaultLogger.h"

namespace AssimpNET
{

DefaultLogger::DefaultLogger(void)
{
	throw gcnew System::NotImplementedException();
}

DefaultLogger::~DefaultLogger(void)
{
	throw gcnew System::NotImplementedException();
}

bool DefaultLogger::attachStream(LogStream^ stream, unsigned int severity)
{
	throw gcnew System::NotImplementedException();
}

bool DefaultLogger::detachStream(LogStream^ stream, unsigned int severity)
{
	throw gcnew System::NotImplementedException();
}

Logger^ DefaultLogger::create(const String^ name, LogSeverity severity, unsigned int defStream, IOSystem^ io)
{
	throw gcnew System::NotImplementedException();
}

Logger^ DefaultLogger::get()
{
	throw gcnew System::NotImplementedException();
}

bool DefaultLogger::isNullLogger()
{
	throw gcnew System::NotImplementedException();
}

void DefaultLogger::kill()
{
	throw gcnew System::NotImplementedException();
}

void DefaultLogger::set(Logger^ logger)
{
	throw gcnew System::NotImplementedException();
}

}//namespace