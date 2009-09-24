/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

 * Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

 * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

 * Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
 */

#pragma once

//managed includes
#include "mLogStream.h"

//native includes
#include "Logger.h"

using namespace System;

namespace AssimpNET
{
	public enum LogSeverity
	{
		NORMAL,
		VERBOSE,
	};

	public ref class Logger	abstract
	{
	public:
		~Logger(void);

		virtual bool attachStream(LogStream^ stream, unsigned int severity) = 0;
		void debug (const String^ message);
		virtual bool detachStream(LogStream^ stream, unsigned int severity) = 0;
		void error(const String^ message);
		LogSeverity getLogSeverity();
		void info(const String^ message);		
		void setLogSverity(LogSeverity log_severity);
		void warn(const String^ message);

	protected:
		Logger(LogSeverity);
		Logger(Assimp::Logger* native);
		Logger();
		virtual void OnDebug(array<char>^ message) = 0;
		virtual void OnError(array<char>^ message) = 0;
		virtual void OnInfo(array<char>^ message) = 0;
		virtual void OnWarn(array<char>^ message) = 0;

		property LogSeverity m_Severity
		{
			LogSeverity get(){throw gcnew System::NotImplementedException();}
			void set(LogSeverity value){throw gcnew System::NotImplementedException();}
		}

		Assimp::Logger* getNative();
		private:
		Assimp::Logger *p_native;

	};
}//namespace