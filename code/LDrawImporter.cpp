
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
const aiImporterDesc* LDrawImporter::GetInfo() const 
{
	return &desc;
}
// -------------------------------------------------------------------------------
void LDrawImporter::SetupProperties(const Importer* pImp)
{
	_libPath = pImp->GetPropertyString(AI_CONFIG_IMPORT_LDRAW_LIB_PATH, "");
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

	char DS = pIOHandler->getOsSeparator();
	//ensure _libPath points to a valid path
	if (_libPath == ""){
		//use the models folder as root
		_libPath = GetFolderPath(pFile, DS);
	}
	else if (_libPath.find_last_of(DS) != (_libPath.size() - 1))
	{
		_libPath += DS;
	}

	//setup a batch loader, it's quite probable we will need it
	BatchLoader loader(pIOHandler);
	BatchLoader::PropertyMap loaderParams = BatchLoader::PropertyMap();
	SetGenericProperty(loaderParams.strings, AI_CONFIG_IMPORT_LDRAW_LIB_PATH, _libPath);

	//TODO estimate sizes
	std::vector<aiVector3D> vertices = std::vector<aiVector3D>();
	std::vector<aiFace> faces = std::vector<aiFace>();
	std::vector<std::pair<unsigned int, aiMatrix4x4*>> fileIds = std::vector<std::pair<unsigned int, aiMatrix4x4*>>();

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
				//it's a comment
				continue;
			}
			else if (command == 1){
				//it's a sub file reference, load it
				float * params = NULL;
				//read a colour constant and 12 floats of a 4x4 matrix
				if (!ReadNumFloats(lp, params, 13)){
					ThrowException(Formatter::format("could not read ") << 13 << " command parameter floats from the line '" << line << "'");
				}
				aiMatrix4x4 *mat = new aiMatrix4x4(
					params[4], params[5], params[6], params[1],
					params[7], params[8], params[9], params[2],
					params[10], params[11], params[12], params[3],
					0.0f, 0.0f, 0.0f, 1.0f
					);
				//we read 13 tokens from lp, so we can safely skip them
				for (unsigned int i = 0; i < 13; ++i){
					SkipToken(lp);
				}
				std::string subpath = GetNextToken(lp);
				if (subpath == ""){
					ThrowException("sub-file reference with empty path/filename");
				}

				//find the full path of the part
				std::string fullpath;
				if (pIOHandler->Exists(subpath)){
					//we are lucky, file is full path referenced
					fullpath = subpath;
				}
				else
				{
					//test the specified directories of the LDraw Library
					static std::vector<std::string> paths{ "parts", "p", "models" };
					for (unsigned int i = 0; i < paths.size(); ++i){
						if (pIOHandler->Exists(_libPath + paths[i] + DS + subpath)){
							fullpath = _libPath + paths[i] + DS + subpath;
							break;
						}
						else if (pIOHandler->Exists(_libPath + ".." + DS+ paths[i] + DS + subpath))
						{
							fullpath = _libPath + ".." + DS + paths[i] + DS + subpath;
							break;
						}
					}
				}
				if (fullpath == ""){
					//we can't find it
					ThrowException("Unable to find file '" + subpath + "'");
				}
				unsigned int id = loader.AddLoadRequest(fullpath, 0, &loaderParams);
				fileIds.push_back(std::pair<unsigned int, aiMatrix4x4*>(id, mat));
				continue;
			}
			else if (command == 2 || command == 3 || command == 4){
				//it's a line or a triangle or a quad
				float * params = NULL;
				//read a colour constant and 2 (line) or 3 (triangle) or 4 (quad) vertices
				if (!ReadNumFloats(lp, params, 1 + (command * 3))){
					ThrowException(Formatter::format("could not read ") << (1 + (command * 3)) <<" command parameter floats from the line '" << line<<"'");
				}
				unsigned int index = vertices.size();

				//TODO colour @params[0]

				vertices.push_back(aiVector3D(params[1], params[2], params[3]));
				vertices.push_back(aiVector3D(params[4], params[5], params[6]));

				aiFace f = aiFace();
				f.mNumIndices = command;
				f.mIndices = new unsigned int[command];
				f.mIndices[0] = index;
				f.mIndices[1] = index + 1;
				if (command == 3 || command == 4){

					vertices.push_back(aiVector3D(params[7], params[8], params[9]));
					f.mIndices[2] = index + 2;

					if (command == 3){
						//it's a triangle
						primitivesType = primitivesType | aiPrimitiveType_TRIANGLE;
					}
					else if (command == 4){
						//it's a quad
						vertices.push_back(aiVector3D(params[10], params[11], params[12]));
						f.mIndices[3] = index + 3;
						primitivesType = primitivesType | aiPrimitiveType_POLYGON;
					}
				}
				else
				{
					//it's a line
					primitivesType = primitivesType | aiPrimitiveType_LINE;
				}
				faces.push_back(f);
				continue;
			}
			else
			{
				//it's an optional 'line' or an unknown command, ignore them
				continue;
			}
		}
		else
		{
			//Line starts not with an identifier
			ThrowException("Line not starting with an Command Identifier");
		}
	}
	aiScene * master = new aiScene();

	//we did read the whole file, now build the scenegraph
	master->mRootNode = new aiNode(pFile);
	master->mRootNode->mTransformation = aiMatrix4x4();

	if (vertices.size() && faces.size())
	{
		master->mNumMeshes = 1;
		master->mMeshes = new aiMesh*[master->mNumMeshes];
		//master->mFlags = AI_SCENE_FLAGS_INCOMPLETE;

		aiMesh* mesh = new aiMesh();
		mesh->mNumFaces = faces.size();
		mesh->mFaces = new aiFace[mesh->mNumFaces];
		std::copy(faces.begin(), faces.end(), mesh->mFaces);
		mesh->mNumVertices = vertices.size();
		mesh->mVertices = new aiVector3D[mesh->mNumVertices];
		std::copy(vertices.begin(), vertices.end(), mesh->mVertices);
		mesh->mPrimitiveTypes = primitivesType;
		master->mMeshes[0] = mesh;

		master->mRootNode->mNumMeshes = 1;
		master->mRootNode->mMeshes = new unsigned int[master->mRootNode->mNumMeshes];
		master->mRootNode->mMeshes[0] = 0;
	}

	if (fileIds.size() != 0){
		//we did queue some sub files, get them
		loader.LoadAll();
		std::vector<AttachmentInfo> attatched(fileIds.size());
		for (unsigned int i = 0; i < fileIds.size(); ++i)
		{
			aiScene* sc = loader.GetImport(fileIds[i].first);
			//check for duplicates with diffrent transformation matrix
			int duplID = -1;
			for (unsigned int j = 0; j < fileIds.size(); j++){
				if (j == i) continue; //don't check ourself for duplicates
				if (fileIds[j].first == fileIds[i].first && *fileIds[j].second != *fileIds[i].second){
					//we found a scene with same id but diffrent transformation => true duplicate
					duplID = j;
					break;
				}
			}
			aiScene * scene;
			if (duplID != -1 && duplID < i){
				//duplicate was found and loaded scene was already used
				SceneCombiner::CopyScene(&scene, sc);
			}
			else
			{
				scene = sc;
			}

			scene->mRootNode->mTransformation = *fileIds[i].second;
			attatched[i] = AttachmentInfo(scene, master->mRootNode);
		}
		SceneCombiner::MergeScenes(&pScene, master, attatched, AI_INT_MERGE_SCENE_GEN_UNIQUE_NAMES_IF_NECESSARY);
	}
	else
	{
		SceneCombiner::CopySceneFlat(&pScene, master);
	}
}

bool LDrawImporter::ReadNumFloats(const char* line, float* & out, unsigned int num)
{
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