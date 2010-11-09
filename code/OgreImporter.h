#include "BaseImporter.h"

#include <vector>

#include "OgreXmlHelper.h"
#include "irrXMLWrapper.h"

namespace Assimp
{
namespace Ogre
{


//Forward declarations:
struct SubMesh;
struct Face;
struct Weight;
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
	virtual void GetExtensionList(std::set<std::string>& extensions);
	virtual void SetupProperties(const Importer* pImp);
private:

	/// Helper Functions to read parts of the XML File
	void ReadSubMesh(SubMesh& theSubMesh, XmlReader* Reader);//the submesh reference is the result value

	/// writes the results in Bones and Animations, Filename is not const, because its call-by-value and the function will change it!
	void LoadSkeleton(std::string FileName, std::vector<Bone> &Bones, std::vector<Animation> &Animations) const;

	/// converts the animations in aiAnimations and puts them into the scene
	void PutAnimationsInScene(const std::vector<Bone> &Bones, const std::vector<Animation> &Animations);

	/// uses the bone data to convert a SubMesh into a aiMesh which will be created and returned
	aiMesh* CreateAssimpSubMesh(const SubMesh &theSubMesh, const std::vector<Bone>& Bones) const;

	//creates the aiskeleton in current scene
	void CreateAssimpSkeleton(const std::vector<Bone> &Bones, const std::vector<Animation> &Animations);

	aiMaterial* LoadMaterial(const std::string MaterialName) const;
	
	///Recursivly creates a filled aiNode from a given root bone
	aiNode* CreateAiNodeFromBone(int BoneId, const std::vector<Bone> &Bones, aiNode* ParentNode) const;

	//Now we don't have to give theses parameters to all functions
	std::string m_CurrentFilename;
	std::string m_MaterialLibFilename;
	IOSystem* m_CurrentIOHandler;
	aiScene *m_CurrentScene;
};

///A submesh from Ogre
struct SubMesh
{	
	std::string Name;
	std::string MaterialName;
	std::vector<Face> FaceList;
	std::vector<aiVector3D> Positions; bool HasPositions;
	std::vector<aiVector3D> Normals; bool HasNormals;
	std::vector<aiVector3D> Uvs; unsigned int NumUvs;//nearly always 2d, but assimp has always 3d texcoords
	std::vector< std::vector<Weight> > Weights;//a list of bones for each vertex
	int MaterialIndex;///< The Index in the Assimp Materialarray from the material witch is attached to this submesh
	unsigned int BonesUsed;//the highest index of a bone from a bone weight, this is needed to create the assimp bone structur (converting from Vertex-Bones to Bone-Vertices)

	SubMesh(): HasPositions(false), HasNormals(false), NumUvs(0), MaterialIndex(-1), BonesUsed(0) {}//initialize everything
};

///For the moment just triangles, no other polygon types!
struct Face
{
	unsigned int VertexIndices[3];
};

struct BoneAssignment
{
	unsigned int BoneId;//this is, what we get from ogre
	std::string BoneName;//this is, what we need for assimp
};

///for a vertex->bone structur
struct Weight
{
	unsigned int BoneId;
	float Value;
};


/// Helper Class to describe an ogre-bone for the skeleton:
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
	aiMatrix4x4 BoneToWorldSpace;

	///ctor
	Bone(): Id(-1), ParentId(-1), RotationAngle(0.0f) {}
	///this operator is needed to sort the bones after Id's
	bool operator<(const Bone& rval) const
		{return Id<rval.Id; }
	///this operator is needed to find a bone by its name in a vector<Bone>
	bool operator==(const std::string& rval) const
		{return Name==rval; }
	bool operator==(const aiString& rval) const
	{return Name==std::string(rval.data); }

	void CalculateBoneToWorldSpaceMatrix(std::vector<Bone>& Bones);
	
};



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
