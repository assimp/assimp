#ifndef OBJ_FILEPARSER_H_INC
#define OBJ_FILEPARSER_H_INC

#include <vector>
#include <string>
#include "aiTypes.h"

/*struct aiVector2D_t;
struct aiVector3D_t;*/

namespace Assimp
{

namespace ObjFile
{
struct Model;
struct Object;
struct Material;
struct Point3;
struct Point2;
}
class ObjFileImporter;

class ObjFileParser
{
public:
	static const size_t BUFFERSIZE = 1024;
	typedef std::vector<char> DataArray;
	typedef std::vector<char>::iterator DataArrayIt;
	typedef std::vector<char>::const_iterator ConstDataArrayIt;

public:
	ObjFileParser(std::vector<char> &Data, const std::string &strAbsPath, const std::string &strModelName);
	~ObjFileParser();
	ObjFile::Model *GetModel() const;

private:
	void parseFile();
	void copyNextWord(char *pBuffer, size_t length);
	void copyNextLine(char *pBuffer, size_t length);
	void getVector3(std::vector<aiVector3D_t*> &point3d_array);
	void getVector2(std::vector<aiVector2D_t*> &point2d_array);
	void skipLine();
	void getFace();
	void getMaterialDesc();
	void getComment();
	void getMaterialLib();
	void getNewMaterial();
	void getGroupName();
	void getGroupNumber();
	void getObjectName();
	void createObject(const std::string &strObjectName);
	void reportErrorTokenInFace();
	void extractExtension(const std::string strFile, std::string &strExt);

private:
	std::string m_strAbsPath;
	DataArrayIt m_DataIt;
	DataArrayIt m_DataItEnd;
	ObjFile::Model *m_pModel;
	unsigned int m_uiLine;
	char m_buffer[BUFFERSIZE];

};

}	// Namespace Assimp

#endif
