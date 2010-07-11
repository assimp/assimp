/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

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

/** @file  Main.cpp
 *  @brief main() function of assimp_cmd
 */

#include "Main.h"

const char* AICMD_MSG_ABOUT = 
"------------------------------------------------------ \n"
"Open Asset Import Library - Assimp \n"
"http://assimp.sourceforge.net \n"
"Command-line tools \n"
"------------------------------------------------------ \n\n"

"Version %i.%i-%s%s%s%s%s (SVNREV %i)\n\n";

const char* AICMD_MSG_HELP = 
"assimp <verb> <arguments>\n\n"
"\tverbs:\n"
"\t\tinfo    - Display statistics and structure of a 3D model\n"
"\t\tversion - Display Assimp version\n"
"\t\tlistext - List all known file extension\n"
"\t\tknowext - Check whether a file extension is recognized by Assimp\n"
"\t\textract - Extract an embedded texture from a model\n"
"\t\tdump    - Convert a model to binary or XML dumps (ASSBIN/ASSXML)\n"
"\t\tcmpdump - Compare two file dumps produced with \'assimp dump <file> -s ...\'\n"
"\n\n\tUse \'assimp <verb> --help\' to get detailed help for a command.\n"
;

/*extern*/ Assimp::Importer* globalImporter = NULL;

// ------------------------------------------------------------------------------
// Application entry point
int main (int argc, char* argv[])
{
	if (argc <= 1)	{
		printf("assimp: No command specified. Use \'assimp help\' for a detailed command list\n");
		return 0;
	}

	// assimp version
	// Display version information
	if (! strcmp(argv[1], "version")) {
		const unsigned int flags = aiGetCompileFlags();
		printf(AICMD_MSG_ABOUT,
			aiGetVersionMajor(),
			aiGetVersionMinor(),
			(flags & ASSIMP_CFLAGS_DEBUG ?			"-debug "   : ""),
			(flags & ASSIMP_CFLAGS_NOBOOST ?		"-noboost " : ""),
			(flags & ASSIMP_CFLAGS_SHARED ?			"-shared "  : ""),
			(flags & ASSIMP_CFLAGS_SINGLETHREADED ? "-st "      : ""),
			(flags & ASSIMP_CFLAGS_STLPORT ?		"-stlport " : ""),
			aiGetVersionRevision());

		return 0;
	}

	// assimp help
	// Display some basic help (--help and -h work as well 
	// because people could try them intuitively)
	if (!strcmp(argv[1], "help") || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		printf("%s",AICMD_MSG_HELP);
		return 0;
	}

	// assimp cmpdump
	// Compare two mini model dumps (regression suite) 
	if (! strcmp(argv[1], "cmpdump")) {
		return Assimp_CompareDump (&argv[2],argc-2);
	}

	// construct a global Assimp::Importer instance
	// because all further tools rely on it
	Assimp::Importer imp;
	imp.SetPropertyBool("GLOB_MEASURE_TIME",true);
	globalImporter = &imp;

	// assimp listext
	// List all file extensions supported by Assimp
	if (! strcmp(argv[1], "listext")) {
		aiString s;
		imp.GetExtensionList(s);

		printf("%s",s.data);
		return 0;
	}

	// assimp knowext
	// Check whether a particular file extension is known by us, return 0 on success
	if (! strcmp(argv[1], "knowext")) {
		if (argc<3) {
			printf("Expected a file extension to check for!");
			return -10;
		}
		const bool b = imp.IsExtensionSupported(argv[2]);
		printf("File extension %s is %sknown",argv[2],(b?"":"not "));
		return b?0:-1;
	}

	// assimp info
	// Print basic model statistics
	if (! strcmp(argv[1], "info")) {
		return Assimp_Info ((const char**)&argv[2],argc-2);
	}

	// assimp dump 
	// Dump a model to a file 
	if (! strcmp(argv[1], "dump")) {
		return Assimp_Dump (&argv[2],argc-2);
	}

	// assimp extract 
	// Extract an embedded texture from a file
	if (! strcmp(argv[1], "extract")) {
		return Assimp_Extract (&argv[2],argc-2);
	}

	// assimp testbatchload
	// Used by /test/other/streamload.py to load a list of files
	// using the same importer instance to check for incompatible
	// importers.
	if (! strcmp(argv[1], "testbatchload")) {
		return Assimp_TestBatchLoad (&argv[2],argc-2);
	}

	printf("Unrecognized command. Use \'assimp help\' for a detailed command list\n");
	return 1;
}

// ------------------------------------------------------------------------------
// Import a specific file
const aiScene* ImportModel(
	const ImportData& imp, 
	const std::string& path)
{
	// Attach log streams
	if (imp.log) {
		printf("\nAttaching log stream   ...           OK\n");
		
		unsigned int flags = 0;
		if (imp.logFile.length()) {
			flags |= aiDefaultLogStream_FILE;
		}
		if (imp.showLog) {
			flags |= aiDefaultLogStream_STDERR;
		}
		DefaultLogger::create(imp.logFile.c_str(),imp.verbose ? Logger::VERBOSE : Logger::NORMAL,flags);
	}
	printf("Launching model import ...           OK\n");

	// Now validate this flag combination
	if(!globalImporter->ValidateFlags(imp.ppFlags)) {
		printf("ERROR: Unsupported post-processing flags \n");
		return NULL;
	}
	printf("Validating postprocessing flags ...  OK\n");
	if (imp.showLog) 
		printf("-----------------------------------------------------------------\n");

	// do the actual import, measure time
	const clock_t first = clock();
	const aiScene* scene = globalImporter->ReadFile(path,imp.ppFlags);

	if (imp.showLog) {
		printf("-----------------------------------------------------------------\n");
	}
	if (!scene) {
		printf("ERROR: Failed to load file\n");	
		return NULL;
	}

	const clock_t second = ::clock();
	const float seconds = (float)(second-first) / CLOCKS_PER_SEC;

	printf("Importing file ...                   OK \n   import took approx. %.5f seconds\n"
		"\n",seconds);

	if (imp.log) { 
		DefaultLogger::kill();
	}
	return scene;
}

// ------------------------------------------------------------------------------
// Process standard arguments
int ProcessStandardArguments(
	ImportData& fill, 
	const char* const * params,
	unsigned int num)
{
	// -ptv    --pretransform-vertices
	// -gsn    --gen-smooth-normals
	// -gn     --gen-normals
	// -cts    --calc-tangent-space
	// -jiv    --join-identical-vertices
	// -rrm    --remove-redundant-materials
	// -fd     --find-degenerates
	// -slm    --split-large-meshes
	// -lbw    --limit-bone-weights
	// -vds    --validate-data-structure
	// -icl    --improve-cache-locality
	// -sbpt   --sort-by-ptype
	// -lh     --convert-to-lh
	// -fuv    --flip-uv
	// -fwo    --flip-winding-order
	// -tuv    --transform-uv-coords
	// -guv    --gen-uvcoords
	// -fid    --find-invalid-data
	// -fixn   --fix normals
	// -tri    --triangulate
	// -fi     --find-instances
	// -fi     --find-instances
	// -og     --optimize-graph
	// -om     --optimize-meshes
	//
	// -c<file> --config-file=<file>

	for (unsigned int i = 0; i < num;++i) 
	{
		//if (!params[i]) { // could happen if some args have already been processed
		//	continue;
		//}

		// bool has = true;
		if (! strcmp(params[i], "-ptv") || ! strcmp(params[i], "--pretransform-vertices")) {
			fill.ppFlags |= aiProcess_PreTransformVertices;
		}
		else if (! strcmp(params[i], "-gsn") || ! strcmp(params[i], "--gen-smooth-normals")) {
			fill.ppFlags |= aiProcess_GenSmoothNormals;
		}
		else if (! strcmp(params[i], "-gn") || ! strcmp(params[i], "--gen-normals")) {
			fill.ppFlags |= aiProcess_GenNormals;
		}
		else if (! strcmp(params[i], "-jiv") || ! strcmp(params[i], "--join-identical-vertices")) {
			fill.ppFlags |= aiProcess_JoinIdenticalVertices;
		}
		else if (! strcmp(params[i], "-rrm") || ! strcmp(params[i], "--remove-redundant-materials")) {
			fill.ppFlags |= aiProcess_RemoveRedundantMaterials;
		}
		else if (! strcmp(params[i], "-fd") || ! strcmp(params[i], "--find-degenerates")) {
			fill.ppFlags |= aiProcess_FindDegenerates;
		}
		else if (! strcmp(params[i], "-slm") || ! strcmp(params[i], "--split-large-meshes")) {
			fill.ppFlags |= aiProcess_SplitLargeMeshes;
		}
		else if (! strcmp(params[i], "-lbw") || ! strcmp(params[i], "--limit-bone-weights")) {
			fill.ppFlags |= aiProcess_LimitBoneWeights;
		}
		else if (! strcmp(params[i], "-vds") || ! strcmp(params[i], "--validate-data-structure")) {
			fill.ppFlags |= aiProcess_ValidateDataStructure;
		}
		else if (! strcmp(params[i], "-icl") || ! strcmp(params[i], "--improve-cache-locality")) {
			fill.ppFlags |= aiProcess_ImproveCacheLocality;
		}
		else if (! strcmp(params[i], "-sbpt") || ! strcmp(params[i], "--sort-by-ptype")) {
			fill.ppFlags |= aiProcess_SortByPType;
		}
		else if (! strcmp(params[i], "-lh") || ! strcmp(params[i], "--left-handed")) {
			fill.ppFlags |= aiProcess_ConvertToLeftHanded;
		}
		else if (! strcmp(params[i], "-fuv") || ! strcmp(params[i], "--flip-uv")) {
			fill.ppFlags |= aiProcess_FlipUVs;
		}
		else if (! strcmp(params[i], "-fwo") || ! strcmp(params[i], "--flip-winding-order")) {
			fill.ppFlags |= aiProcess_FlipWindingOrder;
		}
		else if (! strcmp(params[i], "-tuv") || ! strcmp(params[i], "--transform-uv-coords")) {
			fill.ppFlags |= aiProcess_TransformUVCoords;
		}
		else if (! strcmp(params[i], "-guv") || ! strcmp(params[i], "--gen-uvcoords")) {
			fill.ppFlags |= aiProcess_GenUVCoords;
		}
		else if (! strcmp(params[i], "-fid") || ! strcmp(params[i], "--find-invalid-data")) {
			fill.ppFlags |= aiProcess_FindInvalidData;
		}
		else if (! strcmp(params[i], "-fixn") || ! strcmp(params[i], "--fix-normals")) {
			fill.ppFlags |= aiProcess_FixInfacingNormals;
		}
		else if (! strcmp(params[i], "-tri") || ! strcmp(params[i], "--triangulate")) {
			fill.ppFlags |= aiProcess_Triangulate;
		}
		else if (! strcmp(params[i], "-cts") || ! strcmp(params[i], "--calc-tangent-space")) {
			fill.ppFlags |= aiProcess_CalcTangentSpace;
		}
		else if (! strcmp(params[i], "-fi") || ! strcmp(params[i], "--find-instances")) {
			fill.ppFlags |= aiProcess_FindInstances;
		}
		else if (! strcmp(params[i], "-og") || ! strcmp(params[i], "--optimize-graph")) {
			fill.ppFlags |= aiProcess_OptimizeGraph;
		}
		else if (! strcmp(params[i], "-om") || ! strcmp(params[i], "--optimize-meshes")) {
			fill.ppFlags |= aiProcess_OptimizeMeshes;
		}

#if 0
		else if (! strcmp(params[i], "-oa") || ! strcmp(params[i], "--optimize-anims")) {
			fill.ppFlags |= aiProcess_OptimizeAnims;
		}
		else if (! strcmp(params[i], "-gem") || ! strcmp(params[i], "--gen-entity-meshes")) {
			fill.ppFlags |= aiProcess_GenEntityMeshes;
		}
		else if (! strcmp(params[i], "-ftp") || ! strcmp(params[i], "--fix-texture-paths")) {
			fill.ppFlags |= aiProcess_FixTexturePaths;
		}
#endif

		else if (! strncmp(params[i], "-c",2) || ! strncmp(params[i], "--config=",9)) {
			
			const unsigned int ofs = (params[i][1] == '-' ? 9 : 2);

			// use default configurations
			if (! strncmp(params[i]+ofs,"full",4))
				fill.ppFlags |= aiProcessPreset_TargetRealtime_MaxQuality;

			else if (! strncmp(params[i]+ofs,"default",7))
				fill.ppFlags |= aiProcessPreset_TargetRealtime_Quality;

			else if (! strncmp(params[i]+ofs,"fast",4))
				fill.ppFlags |= aiProcessPreset_TargetRealtime_Fast;
		}
		else if (! strcmp(params[i], "-l") || ! strcmp(params[i], "--show-log")) { 
			fill.showLog = true;
		}
		else if (! strcmp(params[i], "-v") || ! strcmp(params[i], "--verbose")) { 
			fill.verbose = true;
		}
		else if (! strncmp(params[i], "--log-out=",10) || ! strncmp(params[i], "-lo",3)) { 
			fill.logFile = std::string(params[i]+(params[i][1] == '-' ? 10 : 3));
			if (!fill.logFile.length())
				fill.logFile = "assimp-log.txt";
		}

		//else has = false;
		//if (has) {
		//	params[i] = NULL;
		//}
	}

	if (fill.logFile.length() || fill.showLog || fill.verbose)
		fill.log = true;

	return 0;
}

// ------------------------------------------------------------------------------
int Assimp_TestBatchLoad (
	const char* const* params, 
	unsigned int num)
{
	for(unsigned int i = 0; i < num; ++i) {
		globalImporter->ReadFile(params[i],aiProcessPreset_TargetRealtime_MaxQuality);
		// we're totally silent. scene destructs automatically.
	}
	return 0;
}
