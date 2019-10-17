/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team

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

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

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
#include "UnitTestPCH.h"
#include "../../include/assimp/DefaultLogger.hpp"

#include <math.h>
#include <time.h>

int main(int argc, char* argv[]) {
    try {
        ::testing::InitGoogleTest(&argc, argv);
    } catch (...) {
        return 1;
    }

    // seed the randomizer with the current system time
    time_t t;time(&t);
    srand((unsigned int)t);

    // ............................................................................

    // create a logger from both CPP
    Assimp::DefaultLogger::create("AssimpLog_Cpp.txt",Assimp::Logger::VERBOSE,
        aiDefaultLogStream_DEBUGGER | aiDefaultLogStream_FILE);

    // .. and C. They should smoothly work together
    aiEnableVerboseLogging(AI_TRUE);
    aiLogStream logstream= aiGetPredefinedLogStream(aiDefaultLogStream_FILE, "AssimpLog_C.txt");
    aiAttachLogStream(&logstream);

    int result = RUN_ALL_TESTS();

    // ............................................................................
    // but shutdown must be done from C to ensure proper deallocation
    aiDetachAllLogStreams();

    return result;
}
