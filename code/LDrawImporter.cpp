
/** @file  LDrawImporter.cpp
 *  @brief Implementation of the LDraw importer.
 */
#ifndef ASSIMP_BUILD_NO_LDR_IMPORTER
#include "LDrawImporter.h"

using namespace Assimp;

static const aiImporterDesc desc = {
	"LDraw Importer",
	"Tobias 'diiigle' Rittig",
	"",
	"ignoring Linetype 5 'optional lines'",
	0,
	0,
	0,
	0,
	0,
	"ldr dat mpd"
};

LDrawImporter::LDrawImporter()
{
}

LDrawImporter::~LDrawImporter()
{
}

// -------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool LDrawImporter::CanRead(const std::string& pFile, IOSystem* pIOHandler,
	bool checkSig) const
{
	const std::string extension = GetExtension(pFile);
	if (extension == "ldr" || extension == "dat" || extension == "mpd") {
		return true;
	}
	if (!extension.length() || checkSig) {
		const char* tokens[] = { "0 !LDRAW_ORG", "0 !LICENSE" };
		return SearchFileHeaderForToken(pIOHandler, pFile, tokens, 2);
	}
	return false;
}
// -------------------------------------------------------------------------------
const aiImporterDesc* LDrawImporter::GetInfo() const {
	return &desc;
}
// -------------------------------------------------------------------------------
void LDrawImporter::InternReadFile(const std::string& pFile,
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file(pIOHandler->Open(pFile, "rb"));
	// Check whether we can read from the file
	if (file.get() == NULL) {
		ThrowException("Failed to open LDraw file " + pFile + ".");
	}

	// Your task: fill pScene
	// Throw a ImportErrorException with a meaningful (!) error message if 
	// something goes wrong.
	std::vector<char> vecBuffer;
	TextFileToBuffer(file.get(), vecBuffer);

	const char * buffer = &vecBuffer[0];

	//TODO estimate sizes
	std::vector<aiVector3D> * vertices = new std::vector<aiVector3D>();
	std::vector<aiFace> * faces = new std::vector<aiFace>();

	unsigned int primitivesType = 0;

	//iterate line by line
	char line[4096];
	while (GetNextLine(buffer, line))
	{
		//line pointer, used as iterator through the line
		const char* lp = line;

		std::string wholeline = std::string(line);

		SkipSpaces(&lp);
		if (IsLineEnd(*lp))continue;
		if (IsNumeric(*lp)){
			int command = ::atoi(lp);
			++lp;
			if (command == 0){
				//its a comment
				continue;
			}
			else if (command == 1){
				//its a sub file reference, load it
			}
			else if (command == 2 || command == 3 || command == 4){
				//its a line or a triangle or a quad
				float * params = NULL;
				//read a colour constant and 2 (line) or 3 (triangle) or 4 (quad) vertices
				if (!ReadNumFloats(lp, params, 1 + (command * 3))){
					ThrowException(Formatter::format("could not read ") << (1 + (command * 3)) <<" command parameter floats from the line '" << line<<"'");
				}
				unsigned int index = vertices->size();

				//TODO colour @params[0]

				vertices->push_back(aiVector3D(params[1], params[2], params[3]));
				vertices->push_back(aiVector3D(params[4], params[5], params[6]));

				aiFace f = aiFace();
				f.mNumIndices = command;
				f.mIndices = new unsigned int[command];
				f.mIndices[0] = index;
				f.mIndices[1] = index + 1;
				if (command == 3 || command == 4){

					vertices->push_back(aiVector3D(params[7], params[8], params[9]));
					f.mIndices[2] = index + 2;

					if (command == 3){
						//its a triangle
						primitivesType = primitivesType | aiPrimitiveType_TRIANGLE;
					}
					else if (command == 4){
						//its a quad
						vertices->push_back(aiVector3D(params[10], params[11], params[12]));
						f.mIndices[3] = index + 3;
						primitivesType = primitivesType | aiPrimitiveType_POLYGON;
					}
				}
				else
				{
					//its a line
					primitivesType = primitivesType | aiPrimitiveType_LINE;
						
				}
				faces->push_back(f);
			}
			else
			{
				//its an optional 'line' or an unknown command, ignore them
				continue;
			}
		}
		else
		{
			//Line starts not with an identifier
			ThrowException("Line not starting with an Command Identifier");
		}
	}

	//we did read the whole file, now build the scenegraph
	pScene->mRootNode = new aiNode("<LDrawRoot>");
	pScene->mRootNode->mTransformation = aiMatrix4x4();

	pScene->mNumMeshes = 1;
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
	pScene->mFlags = AI_SCENE_FLAGS_INCOMPLETE;

	aiMesh* mesh = new aiMesh();
	mesh->mNumFaces = faces->size();
	mesh->mFaces = new aiFace[mesh->mNumFaces];
	memcpy(mesh->mFaces, faces->data(), sizeof(aiFace) * mesh->mNumFaces);
	mesh->mNumVertices = vertices->size();
	mesh->mVertices = new aiVector3D[mesh->mNumVertices];
	::memcpy(mesh->mVertices, vertices->data(), sizeof(aiVector3D)* mesh->mNumVertices);
	mesh->mPrimitiveTypes = primitivesType;
	pScene->mMeshes[0] = mesh;

	pScene->mRootNode->mNumMeshes = 1;
	pScene->mRootNode->mMeshes = new unsigned int[pScene->mRootNode->mNumMeshes];
	pScene->mRootNode->mMeshes[0] = 0;
	
}

bool LDrawImporter::ReadNumFloats(const char* line, float* & out, unsigned int num){
	out = new float[num];
	for (unsigned int i = 0; i < num; ++i){
		std::string token = GetNextToken(line);
		if (token == ""){
			//unexpected end of line
			return false;
		}
		out[i] = fast_atof(token.c_str());
	}
	return true;
}

#endif // !ASSIMP_BUILD_NO_LDR_IMPORTER