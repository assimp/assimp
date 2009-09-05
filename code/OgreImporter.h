#include "BaseImporter.h"

#include <vector>

#include "irrXMLWrapper.h"
#include "fast_atof.h"

namespace Assimp
{
namespace Ogre
{

typedef irr::io::IrrXMLReader XmlReader;

//Forward declarations:
struct Face;
struct SubMesh;

///The Main Ogre Importer Class
class OgreImporter : public BaseImporter
{
public:
	virtual bool CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const;
	virtual void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler);
	virtual void GetExtensionList(std::string& append);
	virtual void SetupProperties(const Importer* pImp);
private:

	///Helper Functions to read parts of the XML File
	/** @param Filename We need this to check for a material File with the same name.*/
	void ReadSubMesh(SubMesh& theSubMesh, XmlReader* Reader);
	aiMaterial* LoadMaterial(std::string MaterialName);

	//Now we don't have to give theses parameters to all functions
	std::string m_CurrentFilename;
	std::string m_MaterialLibFilename;
	IOSystem* m_CurrentIOHandler;
	aiScene *m_CurrentScene;
};



//------------Helper Funktion to Get a Attribute Save---------------
template<typename t> inline t GetAttribute(XmlReader* Reader, std::string Name)
{
	throw std::exception("unimplemented Funtcion used!");
	return t();
}

template<> inline int GetAttribute<int>(XmlReader* Reader, std::string Name)
{
	const char* Value=Reader->getAttributeValue(Name.c_str());
	if(Value)
		return atoi(Value);
	else
		throw ImportErrorException(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
}

template<> inline float GetAttribute<float>(XmlReader* Reader, std::string Name)
{
	const char* Value=Reader->getAttributeValue(Name.c_str());
	if(Value)
		return fast_atof(Value);
	else
		throw ImportErrorException(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
}

template<> inline std::string GetAttribute<std::string>(XmlReader* Reader, std::string Name)
{
	const char* Value=Reader->getAttributeValue(Name.c_str());
	if(Value)
		return std::string(Value);
	else
		throw new ImportErrorException(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
}

template<> inline bool GetAttribute<bool>(XmlReader* Reader, std::string Name)
{
	const char* Value=Reader->getAttributeValue(Name.c_str());
	if(Value)
	{
		if(Value==std::string("true"))
			return true;
		else if(Value==std::string("false"))
			return false;
		else
			throw new ImportErrorException(std::string("Bool value has invalid value: "+Name+" / "+Value+" / "+Reader->getNodeName()));
	}
	else
		throw new ImportErrorException(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
}
//__________________________________________________________________

inline bool XmlRead(XmlReader* Reader)
{
	do
	{
		if(!Reader->read())
			return false;
	}
	while(Reader->getNodeType()!=irr::io::EXN_ELEMENT);
	return true;
}



///For the moment just triangles, no other polygon types!
struct Face
{
	unsigned int VertexIndices[3];
};

/// Helper Class to describe a complete SubMesh
struct SubMesh
{
	std::string Name;
	std::string MaterialName;
	std::vector<Face> Faces;
};

}//namespace Ogre
}//namespace Assimp