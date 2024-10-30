#include "../../include/assimp/DefaultLogger.hpp"
#include "UnitTestPCH.h"
#include <math.h>
#include <time.h>

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);

    // seed the randomizer with the current system time
    time_t t;
    time(&t);
    srand((unsigned int)t);

    // ............................................................................

    // create a logger from both CPP
    Assimp::DefaultLogger::create("AssimpLog_Cpp.log", Assimp::Logger::VERBOSE,
            aiDefaultLogStream_STDOUT | aiDefaultLogStream_DEBUGGER | aiDefaultLogStream_FILE);

    // .. and C. They should smoothly work together
    aiEnableVerboseLogging(AI_TRUE);
    aiLogStream logstream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE, "AssimpLog_C.log");
    aiAttachLogStream(&logstream);

    int result = RUN_ALL_TESTS();

    // ............................................................................
    // but shutdown must be done from C to ensure proper deallocation
    aiDetachAllLogStreams();

    return result;
}
