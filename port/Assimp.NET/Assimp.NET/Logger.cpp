
#include "mLogger.h"

namespace AssimpNET
{

Logger::Logger(void)
{
	throw gcnew System::NotImplementedException();
}

Logger::Logger(LogSeverity severity)
{
	throw gcnew System::NotImplementedException();
}

Logger::Logger(Assimp::Logger* native)
{
	this->p_native = native;
}

Logger::~Logger(void)
{
	if(this->p_native)
		delete this->p_native;
}

void Logger::debug (const String^ message)
{
	throw gcnew System::NotImplementedException();
}

void Logger::error(const String^ message)
{
	throw gcnew System::NotImplementedException();
}

LogSeverity Logger::getLogSeverity()
{
	throw gcnew System::NotImplementedException();
}

void Logger::info(const String^ message)
{
	throw gcnew System::NotImplementedException();
}

void Logger::setLogSverity(LogSeverity log_severity)
{
	throw gcnew System::NotImplementedException();
}

void Logger::warn(const String^ message)
{
	throw gcnew System::NotImplementedException();
}

Assimp::Logger* Logger::getNative()
{
	return this->p_native;
}

}//namespace