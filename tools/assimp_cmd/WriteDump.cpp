/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

/** @file  WriteDump.cpp
 *  @brief Implementation of the 'assimp dump' utility
 */

#include "Main.h"
#include "PostProcessing/ProcessHelper.h"

const char *AICMD_MSG_DUMP_HELP =
        "assimp dump <model> [<out>] [-b] [-s] [-z] [common parameters]\n"
        "\t -b Binary output \n"
        "\t -s Shortened  \n"
        "\t -z Compressed  \n"
        "\t[See the assimp_cmd docs for a full list of all common parameters]  \n"
        "\t -cfast    Fast post processing preset, runs just a few important steps \n"
        "\t -cdefault Default post processing: runs all recommended steps\n"
        "\t -cfull    Fires almost all post processing steps \n";

#include "Common/assbin_chunks.h"
#include <assimp/DefaultIOSystem.h>
#include "AssetLib/Assbin/AssbinFileWriter.h"
#include "AssetLib/Assxml/AssxmlFileWriter.h"

#include <memory>

FILE *out = nullptr;
bool shortened = false;

#ifndef ASSIMP_BUILD_NO_EXPORT

// -----------------------------------------------------------------------------------
int Assimp_Dump(const char *const *params, unsigned int num) {
    const char *fail = "assimp dump: Invalid number of arguments. "
                       "See \'assimp dump --help\'\r\n";

    // --help
    if (!strcmp(params[0], "-h") || !strcmp(params[0], "--help") || !strcmp(params[0], "-?")) {
        printf("%s", AICMD_MSG_DUMP_HELP);
        return AssimpCmdError::Success;
    }

    // asssimp dump in out [options]
    if (num < 1) {
        printf("%s", fail);
        return AssimpCmdError::InvalidNumberOfArguments;
    }

    std::string in = std::string(params[0]);
    std::string cur_out = (num > 1 ? std::string(params[1]) : std::string("-"));

    // store full command line
    std::string cmd;
    for (unsigned int i = (cur_out[0] == '-' ? 1 : 2); i < num; ++i) {
        if (!params[i]) continue;
        cmd.append(params[i]);
        cmd.append(" ");
    }

    // get import flags
    ImportData import;
    ProcessStandardArguments(import, params + 1, num - 1);

    bool binary = false, cur_shortened = false, compressed = false;

    // process other flags
    for (unsigned int i = 1; i < num; ++i) {
        if (!params[i]) {
            continue;
        }
        if (!strcmp(params[i], "-b") || !strcmp(params[i], "--binary")) {
            binary = true;
        } else if (!strcmp(params[i], "-s") || !strcmp(params[i], "--short")) {
            cur_shortened = true;
        } else if (!strcmp(params[i], "-z") || !strcmp(params[i], "--compressed")) {
            compressed = true;
        }
#if 0
		else if (i > 2 || params[i][0] == '-') {
			::printf("Unknown parameter: %s\n",params[i]);
			return 10;
		}
#endif
    }

    if (cur_out[0] == '-') {
        // take file name from input file
        std::string::size_type pos = in.find_last_of('.');
        if (pos == std::string::npos) {
            pos = in.length();
        }

        cur_out = in.substr(0, pos);
        cur_out.append((binary ? ".assbin" : ".assxml"));
        if (cur_shortened && binary) {
            cur_out.append(".regress");
        }
    }

    // import the main model
    const aiScene *scene = ImportModel(import, in);
    if (!scene) {
        printf("assimp dump: Unable to load input file %s\n", in.c_str());
        return AssimpCmdError::FailedToLoadInputFile;
    }

    try {
        // Dump the main model, using the appropriate method.
        std::unique_ptr<IOSystem> pIOSystem(new DefaultIOSystem());
        if (binary) {
            DumpSceneToAssbin(cur_out.c_str(), cmd.c_str(), pIOSystem.get(),
                    scene, shortened, compressed);
        } else {
            DumpSceneToAssxml(cur_out.c_str(), cmd.c_str(), pIOSystem.get(),
                    scene, shortened);
        }
    } catch (const std::exception &e) {
        printf("%s", ("assimp dump: " + std::string(e.what())).c_str());
        return AssimpCmdError::ExceptionWasRaised;
    } catch (...) {
        printf("assimp dump: An unknown exception occurred.\n");
        return AssimpCmdError::ExceptionWasRaised;
    }

    printf("assimp dump: Wrote output dump %s\n", cur_out.c_str());
    return AssimpCmdError::Success;
}
#else
int Assimp_Dump(const char *const *, unsigned int ) {
    printf("assimp dump: Export disabled.\n");
    return AssimpCmdError::UnrecognizedCommand;
}
#endif