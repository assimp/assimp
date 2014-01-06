
/** @file  LDrawImporter.cpp
 *  @brief Implementation of the LDraw importer.
 */
#ifndef ASSIMP_BUILD_NO_LDR_IMPORTER
#include "LDrawImporter.h"

using namespace Assimp;
using namespace LDraw;

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
	this->pIOHandler = pIOHandler;
	std::string filepath = FindPath(pFile);
	// Check whether we can read from the file
	if (filepath == "") {
		ThrowException("Failed to open LDraw file " + pFile + ".");
	}
	
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

	//Load the materials from <LDrawLibRoot>/ldconfig.ldr
	std::string configPath = FindPath("ldconfig.ldr");
	if (configPath == ""){
		DefaultLogger::get()->info("LDraw: Could not find ldconfig.ldr, using assimp default material");
	}
	else
	{
		ReadMaterials(configPath);
	}

	//Load the scene structure into our intermediate LDrawNode structure
	LDrawNode *root = new LDrawNode();
	root->file.path = filepath;
	root->file.transformation = aiMatrix4x4();
	root->file.color = UINT_MAX;
	ProcessNode(filepath, root, UINT_MAX);

	//convert the LDrawNode structure into assimps scene structure
	pScene->mRootNode = new aiNode(pFile);
	pScene->mRootNode->mTransformation = aiMatrix4x4();
	pScene->mFlags = AI_SCENE_FLAGS_INCOMPLETE;

	std::vector<aiMesh*> aiMeshes;
	std::vector<aiMaterial*> aiMaterials;

	ConvertNode(pScene->mRootNode, root, &aiMeshes, &aiMaterials);

	if (aiMeshes.size())
	{
		//copy the collected meshes
		pScene->mNumMeshes = aiMeshes.size();
		pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
		std::copy(aiMeshes.begin(), aiMeshes.end(), pScene->mMeshes); 
	}

	if (aiMaterials.size())
	{
		//and the materials
		pScene->mNumMaterials = aiMaterials.size();
		pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials];
		std::copy(aiMaterials.begin(), aiMaterials.end(), pScene->mMaterials);
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

void LDrawImporter::ReadMaterials(std::string filename){
	if (!pIOHandler->Exists(filename))	return;
	boost::scoped_ptr<StreamReaderLE> stream(new StreamReaderLE(pIOHandler->Open(filename, "rb")));

	for (LineSplitter splitter(*stream.get()); splitter; ++splitter) {
		const char* cmd = splitter[0];
		//only read line type 0 (comments)
		if (IsNumeric(*cmd) && *cmd == '0'){
			cmd = splitter[1];
			if (*cmd != '!') continue;
			DefaultLogger::get()->debug(*splitter);
			if (TokenMatchI(cmd,"!colour",7)){
				//name of the color
				SkipToken(cmd);
				SkipSpaces(&cmd);
				if (TokenMatchI(cmd, "code", 4)){
					SkipSpaces(&cmd);
					ColorIndex code = strtoul10(cmd, &cmd);
					SkipSpaces(&cmd);
					if (TokenMatchI(cmd, "value", 5)){
						SkipSpaces(&cmd);
						//skip the # before the hex values
						++cmd;
						aiColor3D value;
						value.r = HexOctetToDecimal(cmd);
						cmd += 2;
						value.g = HexOctetToDecimal(cmd);
						cmd += 2;
						value.b = HexOctetToDecimal(cmd);
						cmd += 2;
						value = value * (1 / 255.0f);
						SkipSpaces(&cmd);
						if (TokenMatchI(cmd, "edge", 4)){
							SkipSpaces(&cmd);
							//skip the # before the hex values
							++cmd;
							aiColor3D edge;
							edge.r = HexOctetToDecimal(cmd);
							cmd += 2;
							edge.g = HexOctetToDecimal(cmd);
							cmd += 2;
							edge.b = HexOctetToDecimal(cmd);
							cmd += 2;
							edge = edge * (1 / 255.0f);
							//TODO ALPHA and LUMINANCE
							LDrawMaterial mat = LDrawMaterial(code, value, edge);
							materials.insert(std::pair<ColorIndex, LDrawMaterial>(code, mat));
						}
					}
				}
			}
		}
	}
}

std::string LDrawImporter::FindPath(std::string subpath){
	char DS = pIOHandler->getOsSeparator();
	//find the full path of the file
	std::string fullpath;
	if (pIOHandler->Exists(subpath)){
		//we are lucky, file is full path referenced
		fullpath = subpath;
	}
	else
	{
		//test the specified directories of the LDraw Library
		static std::vector<std::string> paths{ "parts", "p", "", "models"};
		for (unsigned int i = 0; i < paths.size(); ++i){
			if (pIOHandler->Exists(_libPath + paths[i] + DS + subpath)){
				fullpath = _libPath + paths[i] + DS + subpath;
				break;
			}
			else if (pIOHandler->Exists(_libPath + ".." + DS + paths[i] + DS + subpath))
			{
				fullpath = _libPath + ".." + DS + paths[i] + DS + subpath;
				break;
			}
		}
	}
	return fullpath;
}

void LDrawImporter::ProcessNode(std::string file, LDrawNode* current, ColorIndex colorindex)
{
	std::vector<SubFileReference> subfiles;
	std::map<ColorIndex, LDrawMesh> meshes;
	//iterate through file, collect subfilereferences and meshes

	boost::scoped_ptr<IOStream> fileStream(pIOHandler->Open(file, "rb"));
	std::vector<char> vecBuffer;
	TextFileToBuffer(fileStream.get(), vecBuffer);

	const char * buffer = &vecBuffer[0];

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
				SubFileReference ref;
				ref.transformation = *mat;
				ref.color = ColorIndex(params[0]);
				if (ref.color == 16)
					ref.variableColor = true;
				ref.path  = FindPath(subpath);
				if (ref.path == ""){
					//we can't find it
					ThrowException("Unable to find file '" + subpath + "'");
				}
				subfiles.push_back(ref);
				continue;
			}
			else if (command == 2 || command == 3 || command == 4){
				//it's a line or a triangle or a quad
				float * params = NULL;
				//read a colour constant and 2 (line) or 3 (triangle) or 4 (quad) vertices
				if (!ReadNumFloats(lp, params, 1 + (command * 3))){
					ThrowException(Formatter::format("could not read ") << (1 + (command * 3)) << " command parameter floats from the line '" << line << "'");
				}


				LDrawMesh * mesh = &meshes[ColorIndex(params[0])];
				std::vector<aiVector3D> *vertices = &mesh->vertices;
				std::vector<aiFace> *faces = &mesh->faces;

				unsigned int index = vertices->size();

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
						//it's a triangle
						mesh->primitivesType = mesh->primitivesType | aiPrimitiveType_TRIANGLE;
					}
					else if (command == 4){
						//it's a quad
						vertices->push_back(aiVector3D(params[10], params[11], params[12]));
						f.mIndices[3] = index + 3;
						mesh->primitivesType = mesh->primitivesType | aiPrimitiveType_POLYGON;
					}
				}
				else
				{
					//it's a line
					mesh->primitivesType = mesh->primitivesType | aiPrimitiveType_LINE;
				}
				faces->push_back(f);
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

	//load subfiles
	for (std::vector<SubFileReference>::iterator sb = subfiles.begin(); sb != subfiles.end(); ++sb)
	{
		LDrawNode* child = new LDrawNode;
		child->file = *sb;

		//check cache for already loaded files
		LDrawFile loadedFile;
		try
		{
			loadedFile = fileCache.at(sb->path);
			child->children.insert(child->children.end(), loadedFile.subtree.children.begin(), loadedFile.subtree.children.end());
		}
		catch (std::out_of_range ex){
			//not existing in cache, load it
			ProcessNode(sb->path, child, sb->color);
			loadedFile = fileCache.at(sb->path);
		}
		ColorNode(child, (child->file.variableColor)? colorindex : child->file.color);
		current->children.push_back(*child);

		////merge the childs meshes with ours
		//for (std::map<ColorIndex, LDrawMesh>::iterator m = loadedFile.meshes.begin(); m != loadedFile.meshes.end(); ++m)
		//{
		//	//merge vertices
		//	std::vector<aiVector3D>* v = &meshes[m->first].vertices;
		//	v->reserve(v->size() + m->second.vertices.size());
		//	unsigned int offset = v->size();
		//	v->insert(v->end(), m->second.vertices.begin(), m->second.vertices.end());

		//	std::vector<aiFace>* f = &meshes[m->first].faces;
		//	f->reserve(f->size() + m->second.faces.size());
		//	for (std::vector<aiFace>::iterator fnew = m->second.faces.begin(); fnew != m->second.faces.end(); ++fnew)
		//	{
		//		for (unsigned int  i = 0; i < fnew->mNumIndices; ++i)
		//		{
		//			fnew->mIndices[i] += offset;
		//		}
		//		f->push_back(*fnew);
		//	}

		//	meshes[m->first].primitivesType = meshes[m->first].primitivesType | m->second.primitivesType;
		//}
	}

	//cache file
	LDrawFile thisfile;
	thisfile.meshes = meshes;
	thisfile.subtree = *current;
	fileCache.insert(std::pair<std::string, LDrawFile>(file, thisfile));
}

void LDrawImporter::ConvertNode(aiNode* node, LDrawNode* current, std::vector<aiMesh*>* aiMeshes, std::vector<aiMaterial*>* aiMaterials)
{
	node->mTransformation = current->file.transformation;
	node->mName = current->file.path;

	//check cache for loaded meshes
	LDrawFile loadedFile;
	try
	{
		loadedFile = fileCache.at(current->file.path);
	}
	catch (std::out_of_range ex){
		//not existing in cache, something went wrong
		ThrowException("could not find the file in the cache: " + current->file.path);
	}
	std::map<ColorIndex, LDrawMesh>* meshes = &loadedFile.meshes;
	if (!meshes->empty())
	{
		node->mNumMeshes = meshes->size();
		node->mMeshes = new unsigned int[node->mNumMeshes];

		unsigned int index = 0;
		for (std::map<ColorIndex, LDrawMesh>::iterator i = meshes->begin(); i != meshes->end(); ++i, ++index)
		{
			LDrawMesh * ldrMesh = &i->second;
			std::vector<aiVector3D>* vertices = &ldrMesh->vertices;
			std::vector<aiFace>* faces = &ldrMesh->faces;

			aiMesh* mesh = new aiMesh();
			mesh->mNumFaces = faces->size();
			mesh->mFaces = new aiFace[mesh->mNumFaces];
			std::copy(faces->begin(), faces->end(), mesh->mFaces);
			mesh->mNumVertices = vertices->size();
			mesh->mVertices = new aiVector3D[mesh->mNumVertices];
			std::copy(vertices->begin(), vertices->end(), mesh->mVertices);
			mesh->mPrimitiveTypes = ldrMesh->primitivesType;
			
			node->mMeshes[index] = aiMeshes->size();
			aiMeshes->push_back(mesh);

			LDrawMaterial * rawMaterial;
			ColorIndex color = (i->first == 16 || i->first == 24) ? current->file.color : i->first;
			try{
				rawMaterial = &materials.at(color);
			}
			catch (std::out_of_range ex){
				//we don't know that material
				continue;
			}
			aiMaterial* material = new aiMaterial();
			if (i->first == 24)
				material->AddProperty(&rawMaterial->edge, 1, AI_MATKEY_COLOR_DIFFUSE);
			else
				material->AddProperty(&rawMaterial->color, 1, AI_MATKEY_COLOR_DIFFUSE);
			if (rawMaterial->alpha != 1.0f)
				material->AddProperty(&rawMaterial->alpha, 1, AI_MATKEY_OPACITY);
			if (rawMaterial->luminance != 0.0f)
				material->AddProperty(&(rawMaterial->color * rawMaterial->luminance), 1, AI_MATKEY_COLOR_EMISSIVE);

			mesh->mMaterialIndex = aiMaterials->size();
			aiMaterials->push_back(material);
		}
	}

	node->mNumChildren = current->children.size();
	node->mChildren = new aiNode*[node->mNumChildren];
	unsigned int nodeIndex = 0;
	for (std::vector<LDrawNode>::iterator child = current->children.begin(); child != current->children.end(); ++child, ++nodeIndex)
	{
		aiNode * nodeChild = new aiNode();
		ConvertNode(nodeChild, &(*child), aiMeshes, aiMaterials);
		nodeChild->mParent = node;
		node->mChildren[nodeIndex] = nodeChild;
	}
}

void LDrawImporter::ColorNode(LDrawNode* current, ColorIndex color)
{
	if (current->file.color == 16 || current->file.variableColor)
		current->file.color = color;

	//recursive for the children
	for (std::vector<LDrawNode>::iterator child = current->children.begin(); child != current->children.end(); ++child)
	{
		ColorNode(&(*child), color);
	}
}

#endif // !ASSIMP_BUILD_NO_LDR_IMPORTER