
#include "BaseImporter.h"
#include "OgreXmlHelper.hpp"
#include "irrXMLWrapper.h"

#include <vector>

/** @todo Read Vertex Colors
    @todo Read multiple TexCoords
*/

namespace Assimp
{
namespace Ogre
{

struct Face;
struct BoneWeight;
struct Bone;
struct Animation;

/// Ogre SubMesh
struct SubMesh
{
	bool UseSharedGeometry;
	bool Use32bitIndexes;

	std::string Name;
	std::string MaterialName;

	bool HasGeometry;
	bool HasPositions;
	bool HasNormals;
	bool HasTangents;

	std::vector<Face> Faces;
	std::vector<aiVector3D> Positions;
	std::vector<aiVector3D> Normals;
	std::vector<aiVector3D> Tangents;

	/// Arbitrary number of texcoords, they are nearly always 2d, but Assimp has always 3d texcoords, n vectors(outer) with texcoords for each vertex(inner).
	std::vector<std::vector<aiVector3D> > Uvs;

	/// A list(inner) of bones for each vertex(outer).
	std::vector<std::vector<BoneWeight> > Weights;

	/// The Index in the Assimp material array from the material witch is attached to this submesh.
	int MaterialIndex;

	// The highest index of a bone from a bone weight, this is needed to create the Assimp bone struct. Converting from vertex-bones to bone-vertices.
	unsigned int BonesUsed;

	SubMesh() :
		UseSharedGeometry(false),
		Use32bitIndexes(false),
		HasGeometry(false),
		HasPositions(false),
		HasNormals(false),
		HasTangents(false),
		MaterialIndex(-1),
		BonesUsed(0)
	{
	}
};

// ------------------------------------------------------------------------------------------------
///	\class	OgreImporter
///	\brief	Importer for Ogre mesh, skeleton and material formats.
// ------------------------------------------------------------------------------------------------
class OgreImporter : public BaseImporter
{
public:
	/// BaseImporter override.
	virtual bool CanRead(const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const;
	
	/// BaseImporter override.
	virtual void InternReadFile(const std::string &pFile, aiScene* pScene, IOSystem* pIOHandler);
	
	/// BaseImporter override.
	virtual const aiImporterDesc* GetInfo () const;
	
	/// BaseImporter override.
	virtual void SetupProperties(const Importer* pImp);

private:
	//-------------------------------- OgreMesh.cpp -------------------------------

	/// Helper Functions to read parts of the XML File.
	void ReadSubMesh(const unsigned int submeshIndex, SubMesh &submesh, XmlReader *reader);

	/// Reads a single Vertexbuffer and writes its data in the Submesh.
	static void ReadVertexBuffer(SubMesh &submesh, XmlReader *reader, const unsigned int numVertices);

	/// Reads bone weights are stores them into the given submesh.
	static void ReadBoneWeights(SubMesh &submesh, XmlReader *reader);

	/// After Loading a SubMehs some work needs to be done (make all Vertexes unique, normalize weights).
	static void ProcessSubMesh(SubMesh &submesh, SubMesh &sharedGeometry);

	/// Uses the bone data to convert a SubMesh into a aiMesh which will be created and returned.
	aiMesh* CreateAssimpSubMesh(const SubMesh &submesh, const std::vector<Bone>& bones) const;

	//-------------------------------- OgreSkeleton.cpp -------------------------------

	/// Writes the results in Bones and Animations, Filename is not const, because its call-by-value and the function will change it!
	void LoadSkeleton(std::string FileName, std::vector<Bone> &Bones, std::vector<Animation> &Animations) const;

	/// Converts the animations in aiAnimations and puts them into the scene.
	void PutAnimationsInScene(const std::vector<Bone> &Bones, const std::vector<Animation> &Animations);

	/// Creates the aiSkeleton in current scene.
	void CreateAssimpSkeleton(const std::vector<Bone> &Bones, const std::vector<Animation> &Animations);

	/// Recursivly creates a filled aiNode from a given root bone.
	static aiNode* CreateAiNodeFromBone(int BoneId, const std::vector<Bone> &Bones, aiNode* ParentNode);

	//-------------------------------- OgreMaterial.cpp -------------------------------

	aiMaterial* LoadMaterial(const std::string MaterialName) const;
	void ReadTechnique(std::stringstream &ss, aiMaterial* NewMaterial) const;

	// Now we don't have to give theses parameters to all functions
	/// @todo Remove this m_Current* bookkeeping.
	std::string m_CurrentFilename;
	std::string m_MaterialLibFilename;
	bool m_TextureTypeFromFilename;
	IOSystem* m_CurrentIOHandler;
	aiScene *m_CurrentScene;
	SubMesh m_SharedGeometry;///< we will just use the vertexbuffers of the submesh
};

/// Simplified face.
/** @todo Support other polygon types than just just triangles. Move to using aiFace. */
struct Face
{
	unsigned int VertexIndices[3];
};

/// Ogre Bone assignment
struct BoneAssignment
{
	/// Bone ID from Ogre.
	unsigned int BoneId;
	// Bone name for Assimp.
	std::string BoneName;
};

/// Ogre Bone weight
struct BoneWeight
{
	/// Bone ID
	unsigned int Id;
	/// BoneWeight
	float Value;
};


/// Helper Class to describe an ogre-bone for the skeleton:
/** All Id's are signed ints, because than we have -1 as a simple INVALID_ID Value (we start from 0 so 0 is a valid bone ID!
	@todo Cleanup if possible. Seems like this is overly complex for what we are reading. */
struct Bone
{
	std::string Name;

	int Id;
	int ParentId;
	
	aiVector3D Position;
	aiVector3D RotationAxis;
	float RotationAngle;
	
	aiMatrix4x4 BoneToWorldSpace;

	std::vector<int> Children;

	Bone() :
		Id(-1),
		ParentId(-1),
		RotationAngle(0.0f)
	{
	}

	/// This operator is needed to sort the bones after Id's
	bool operator<(const Bone &other) const { return Id < other.Id; }

	/// This operator is needed to find a bone by its name in a vector<Bone>
	bool operator==(const std::string& other) const { return Name == other; }
	bool operator==(const aiString& other) const { return Name == std::string(other.data); }

	/// @note Implemented in OgreSkeleton.cpp
	void CalculateBoneToWorldSpaceMatrix(std::vector<Bone>& Bones);
};

/// Ogre animation key frame
/** Transformations for a frame. */
struct KeyFrame
{
	float Time;
	aiVector3D Position;
	aiQuaternion Rotation;
	aiVector3D Scaling;
};

/// Ogre animation track
/** Keyframes for one bone. */
struct Track
{
	std::string BoneName;
	std::vector<KeyFrame> Keyframes;
};

/// Ogre animation
struct Animation
{
	/// Name
	std::string Name;
	/// Length
	float Length;
	/// Tracks
	std::vector<Track> Tracks;
};

}//namespace Ogre
}//namespace Assimp
