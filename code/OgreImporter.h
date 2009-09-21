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
struct Weight;
struct SubMesh;
struct Bone;
struct Animation;
struct Track;
struct Keyframe;

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
	void LoadSkeleton(std::string FileName);

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
		throw new ImportErrorException(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
}

template<> inline float GetAttribute<float>(XmlReader* Reader, std::string Name)
{
	const char* Value=Reader->getAttributeValue(Name.c_str());
	if(Value)
		return fast_atof(Value);
	else
		throw new ImportErrorException(std::string("Attribute "+Name+" does not exist in "+Reader->getNodeName()).c_str());
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

struct Weight
{
	unsigned int BoneId;
	float Value;
};

/// Helper Class to describe a complete SubMesh
struct SubMesh
{
	std::string Name;
	std::string MaterialName;
	std::vector<Face> Faces;
};


/// Helper Class to describe an ogre-bone
/** All Id's are signed ints, because than we have -1 as a simple INVALID_ID Value (we start from 0 so 0 is a valid bone ID!*/
struct Bone
{
	int Id;
	int ParentId;
	std::string Name;
	aiVector3D Position;
	float RotationAngle;
	aiVector3D RotationAxis;
	std::vector<int> Children;

	///ctor
	Bone(): Id(-1), ParentId(-1), RotationAngle(0.0f) {}
	///this operator is needed to sort the bones after Id's
	bool operator<(const Bone& rval) const
		{return Id<rval.Id; }
	///this operator is needed to find a bone by its name in a vector<Bone>
	bool operator==(const std::string& rval) const
		{return Name==rval; }
	
};

///Recursivly creates a filled aiNode from a given root bone
aiNode* CreateAiNodeFromBone(int BoneId, std::vector<Bone> Bones, aiNode* ParentNode);


///Describes an Ogre Animation
struct Animation
{
	std::string Name;
	float Length;
	std::vector<Track> Tracks;
};

///a track (keyframes for one bone) from an animation
struct Track
{
	std::string BoneName;
	std::vector<Keyframe> Keyframes;
};

/// keyframe (bone transformation) from a track from a animation
struct Keyframe
{
	float Time;
	aiVector3D Position;
	aiQuaternion Rotation;
	aiVector3D Scaling;
};

}//namespace Ogre
}//namespace Assimp