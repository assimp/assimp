
#ifndef AI_OGREIMPORTER_H_INC
#define AI_OGREIMPORTER_H_INC

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include "BaseImporter.h"
#include "OgreParsingUtils.h"

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

/**	Importer for Ogre mesh, skeleton and material formats.
	@todo Support vertex colors
	@todo Support multiple TexCoords (this is already done??) */
class OgreImporter : public BaseImporter
{
public:
	/// BaseImporter override.
	virtual bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool checkSig) const;
	
	/// BaseImporter override.
	virtual void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler);
	
	/// BaseImporter override.
	virtual const aiImporterDesc *GetInfo() const;
	
	/// BaseImporter override.
	virtual void SetupProperties(const Importer *pImp);

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
	aiMesh *CreateAssimpSubMesh(aiScene *pScene, const SubMesh &submesh, const std::vector<Bone> &bones) const;

	//-------------------------------- OgreSkeleton.cpp -------------------------------

	/// Writes the results in Bones and Animations, Filename is not const, because its call-by-value and the function will change it!
	void ReadSkeleton(const std::string &pFile, Assimp::IOSystem *pIOHandler, const aiScene *pScene,
					  const std::string &skeletonFile, std::vector<Bone> &Bones, std::vector<Animation> &Animations) const;

	/// Converts the animations in aiAnimations and puts them into the scene.
	void PutAnimationsInScene(aiScene *pScene, const std::vector<Bone> &Bones, const std::vector<Animation> &Animations);

	/// Creates the aiSkeleton in current scene.
	void CreateAssimpSkeleton(aiScene *pScene, const std::vector<Bone> &bones, const std::vector<Animation> &animations);

	/// Recursively creates a filled aiNode from a given root bone.
	static aiNode* CreateNodeFromBone(int boneId, const std::vector<Bone> &bones, aiNode *parent);

	//-------------------------------- OgreMaterial.cpp -------------------------------

	/// Reads material
	aiMaterial* ReadMaterial(const std::string &pFile, Assimp::IOSystem *pIOHandler, const std::string MaterialName);

	// These functions parse blocks from a material file from @c ss. Starting parsing from "{" and ending it to "}".
	bool ReadTechnique(const std::string &techniqueName, std::stringstream &ss, aiMaterial *material);
	bool ReadPass(const std::string &passName, std::stringstream &ss, aiMaterial *material);	
	bool ReadTextureUnit(const std::string &textureUnitName, std::stringstream &ss, aiMaterial *material);

	std::string m_userDefinedMaterialLibFile;
	bool m_detectTextureTypeFromFilename;
	
	/// VertexBuffer for the sub meshes that use shader geometry.
	SubMesh m_SharedGeometry;
	
	std::map<aiTextureType, unsigned int> m_textures;
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
	/// Bone Id
	unsigned int Id;
	/// BoneWeight
	float Value;
};


/// Ogre Bone
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

	/// Returns if this bone is parented.
	bool IsParented() const { return (ParentId != -1); }

	/// This operator is needed to sort the bones by Id in a vector<Bone>.
	bool operator<(const Bone &other) const { return (Id < other.Id); }

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

} // Ogre
} // Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
#endif // AI_OGREIMPORTER_H_INC
