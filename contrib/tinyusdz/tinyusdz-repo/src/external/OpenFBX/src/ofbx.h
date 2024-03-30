#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif


namespace ofbx
{


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#ifdef _WIN32
	typedef long long i64;
	typedef unsigned long long u64;
#else
	typedef long i64;
	typedef unsigned long u64;
#endif

static_assert(sizeof(u8) == 1, "u8 is not 1 byte");
static_assert(sizeof(u32) == 4, "u32 is not 4 bytes");
static_assert(sizeof(u64) == 8, "u64 is not 8 bytes");
static_assert(sizeof(i64) == 8, "i64 is not 8 bytes");


using JobFunction = void (*)(void*);
using JobProcessor = void (*)(JobFunction, void*, void*, u32, u32);

enum class LoadFlags : u64 {
	TRIANGULATE = 1 << 0,
	IGNORE_GEOMETRY = 1 << 1,
	IGNORE_BLEND_SHAPES = 1 << 2,
};


struct Vec2
{
	double x, y;
};


struct Vec3
{
	double x, y, z;
};


struct Vec4
{
	double x, y, z, w;
};


struct Matrix
{
	double m[16]; // last 4 are translation
};


struct Quat
{
	double x, y, z, w;
};


struct Color
{
	float r, g, b;
};


struct DataView
{
	const u8* begin = nullptr;
	const u8* end = nullptr;
	bool is_binary = true;

	bool operator!=(const char* rhs) const { return !(*this == rhs); }
	bool operator==(const char* rhs) const;

	u64 toU64() const;
	i64 toI64() const;
	int toInt() const;
	u32 toU32() const;
	double toDouble() const;
	float toFloat() const;

	template <int N>
	void toString(char(&out)[N]) const
	{
		char* cout = out;
		const u8* cin = begin;
		while (cin != end && cout - out < N - 1)
		{
			*cout = (char)*cin;
			++cin;
			++cout;
		}
		*cout = '\0';
	}
};


struct IElementProperty
{
	enum Type : unsigned char
	{
		LONG = 'L',
		INTEGER = 'I',
		STRING = 'S',
		FLOAT = 'F',
		DOUBLE = 'D',
		ARRAY_DOUBLE = 'd',
		ARRAY_INT = 'i',
		ARRAY_LONG = 'l',
		ARRAY_FLOAT = 'f',
		BINARY = 'R'
	};
	virtual ~IElementProperty() {}
	virtual Type getType() const = 0;
	virtual IElementProperty* getNext() const = 0;
	virtual DataView getValue() const = 0;
	virtual int getCount() const = 0;
	virtual bool getValues(double* values, int max_size) const = 0;
	virtual bool getValues(int* values, int max_size) const = 0;
	virtual bool getValues(float* values, int max_size) const = 0;
	virtual bool getValues(u64* values, int max_size) const = 0;
	virtual bool getValues(i64* values, int max_size) const = 0;
};


struct IElement
{
    virtual ~IElement() = default;
	virtual IElement* getFirstChild() const = 0;
	virtual IElement* getSibling() const = 0;
	virtual DataView getID() const = 0;
	virtual IElementProperty* getFirstProperty() const = 0;
};


enum class RotationOrder
{
	EULER_XYZ,
	EULER_XZY,
	EULER_YZX,
	EULER_YXZ,
	EULER_ZXY,
	EULER_ZYX,
	SPHERIC_XYZ // Currently unsupported. Treated as EULER_XYZ.
};


struct AnimationCurveNode;
struct AnimationLayer;
struct Scene;
struct IScene;


struct Object
{
	enum class Type
	{
		ROOT,
		GEOMETRY,
		SHAPE,
		MATERIAL,
		MESH,
		TEXTURE,
		LIMB_NODE,
		NULL_NODE,
		NODE_ATTRIBUTE,
		CLUSTER,
		SKIN,
		BLEND_SHAPE,
		BLEND_SHAPE_CHANNEL,
		ANIMATION_STACK,
		ANIMATION_LAYER,
		ANIMATION_CURVE,
		ANIMATION_CURVE_NODE,
		POSE
	};

	Object(const Scene& _scene, const IElement& _element);

	virtual ~Object() {}
	virtual Type getType() const = 0;

	const IScene& getScene() const;
	Object* resolveObjectLink(int idx) const;
	Object* resolveObjectLink(Type type, const char* property, int idx) const;
	Object* resolveObjectLinkReverse(Type type) const;
	Object* getParent() const;

	RotationOrder getRotationOrder() const;
	Vec3 getRotationOffset() const;
	Vec3 getRotationPivot() const;
	Vec3 getPostRotation() const;
	Vec3 getScalingOffset() const;
	Vec3 getScalingPivot() const;
	Vec3 getPreRotation() const;
	Vec3 getLocalTranslation() const;
	Vec3 getLocalRotation() const;
	Vec3 getLocalScaling() const;
	Matrix getGlobalTransform() const;
	Matrix getLocalTransform() const;
	Matrix evalLocal(const Vec3& translation, const Vec3& rotation) const;
	Matrix evalLocal(const Vec3& translation, const Vec3& rotation, const Vec3& scaling) const;
	bool isNode() const { return is_node; }


	template <typename T> T* resolveObjectLink(int idx) const
	{
		return static_cast<T*>(resolveObjectLink(T::s_type, nullptr, idx));
	}

	u64 id;
	char name[128];
	const IElement& element;
	const Object* node_attribute;

protected:
	bool is_node;
	const Scene& scene;
};


struct Pose : Object {
	static const Type s_type = Type::POSE;
	Pose(const Scene& _scene, const IElement& _element);

	virtual Matrix getMatrix() const = 0;
	virtual const Object* getNode() const = 0;
};


struct Texture : Object
{
	enum TextureType
	{
		DIFFUSE,
		NORMAL,
		SPECULAR,
        SHININESS,
        AMBIENT,
        EMISSIVE,
        REFLECTION,
		COUNT
	};

	static const Type s_type = Type::TEXTURE;

	Texture(const Scene& _scene, const IElement& _element);
	virtual DataView getFileName() const = 0;
	virtual DataView getRelativeFileName() const = 0;
	virtual DataView getEmbeddedData() const = 0;
};


struct Material : Object
{
	static const Type s_type = Type::MATERIAL;

	Material(const Scene& _scene, const IElement& _element);

	virtual Color getDiffuseColor() const = 0;
	virtual Color getSpecularColor() const = 0;
    virtual Color getReflectionColor() const = 0;
    virtual Color getAmbientColor() const = 0;
    virtual Color getEmissiveColor() const = 0;

    virtual double getDiffuseFactor() const = 0;
    virtual double getSpecularFactor() const = 0;
    virtual double getReflectionFactor() const = 0;
    virtual double getShininess() const = 0;
    virtual double getShininessExponent() const = 0;
    virtual double getAmbientFactor() const = 0;
    virtual double getBumpFactor() const = 0;
    virtual double getEmissiveFactor() const = 0;

	virtual const Texture* getTexture(Texture::TextureType type) const = 0;
};


struct Cluster : Object
{
	static const Type s_type = Type::CLUSTER;

	Cluster(const Scene& _scene, const IElement& _element);

	virtual const int* getIndices() const = 0;
	virtual int getIndicesCount() const = 0;
	virtual const double* getWeights() const = 0;
	virtual int getWeightsCount() const = 0;
	virtual Matrix getTransformMatrix() const = 0;
	virtual Matrix getTransformLinkMatrix() const = 0;
	virtual const Object* getLink() const = 0;
};


struct Skin : Object
{
	static const Type s_type = Type::SKIN;

	Skin(const Scene& _scene, const IElement& _element);

	virtual int getClusterCount() const = 0;
	virtual const Cluster* getCluster(int idx) const = 0;
};


struct BlendShapeChannel : Object
{
	static const Type s_type = Type::BLEND_SHAPE_CHANNEL;

	BlendShapeChannel(const Scene& _scene, const IElement& _element);

	virtual double getDeformPercent() const = 0;
	virtual int getShapeCount() const = 0;
	virtual const struct Shape* getShape(int idx) const = 0;
};


struct BlendShape : Object
{
	static const Type s_type = Type::BLEND_SHAPE;

	BlendShape(const Scene& _scene, const IElement& _element);

	virtual int getBlendShapeChannelCount() const = 0;
	virtual const BlendShapeChannel* getBlendShapeChannel(int idx) const = 0;
};


struct NodeAttribute : Object
{
	static const Type s_type = Type::NODE_ATTRIBUTE;

	NodeAttribute(const Scene& _scene, const IElement& _element);

	virtual DataView getAttributeType() const = 0;
};


struct Geometry : Object
{
	static const Type s_type = Type::GEOMETRY;
	static const int s_uvs_max = 4;

	Geometry(const Scene& _scene, const IElement& _element);

	virtual const Vec3* getVertices() const = 0;
	virtual int getVertexCount() const = 0;

	virtual const int* getFaceIndices() const = 0;
	virtual int getIndexCount() const = 0;

	virtual const Vec3* getNormals() const = 0;
	virtual const Vec2* getUVs(int index = 0) const = 0;
	virtual const Vec4* getColors() const = 0;
	virtual const Vec3* getTangents() const = 0;
	virtual const Skin* getSkin() const = 0;
	virtual const BlendShape* getBlendShape() const = 0;
	virtual const int* getMaterials() const = 0;
};


struct Shape : Object
{
	static const Type s_type = Type::SHAPE;

	Shape(const Scene& _scene, const IElement& _element);

	virtual const Vec3* getVertices() const = 0;
	virtual int getVertexCount() const = 0;

	virtual const Vec3* getNormals() const = 0;
};


struct Mesh : Object
{
	static const Type s_type = Type::MESH;

	Mesh(const Scene& _scene, const IElement& _element);

	virtual const Pose* getPose() const = 0;
	virtual const Geometry* getGeometry() const = 0;
	virtual Matrix getGeometricMatrix() const = 0;
	virtual const Material* getMaterial(int idx) const = 0;
	virtual int getMaterialCount() const = 0;
};


struct AnimationStack : Object
{
	static const Type s_type = Type::ANIMATION_STACK;

	AnimationStack(const Scene& _scene, const IElement& _element);
	virtual const AnimationLayer* getLayer(int index) const = 0;
};


struct AnimationLayer : Object
{
	static const Type s_type = Type::ANIMATION_LAYER;

	AnimationLayer(const Scene& _scene, const IElement& _element);

	virtual const AnimationCurveNode* getCurveNode(int index) const = 0;
	virtual const AnimationCurveNode* getCurveNode(const Object& bone, const char* property) const = 0;
};


struct AnimationCurve : Object
{
	static const Type s_type = Type::ANIMATION_CURVE;

	AnimationCurve(const Scene& _scene, const IElement& _element);

	virtual int getKeyCount() const = 0;
	virtual const i64* getKeyTime() const = 0;
	virtual const float* getKeyValue() const = 0;
};


struct AnimationCurveNode : Object
{
	static const Type s_type = Type::ANIMATION_CURVE_NODE;

	AnimationCurveNode(const Scene& _scene, const IElement& _element);

	virtual const AnimationCurve* getCurve(int idx) const = 0;
	virtual Vec3 getNodeLocalTransform(double time) const = 0;
	virtual const Object* getBone() const = 0;
};


struct TakeInfo
{
	DataView name;
	DataView filename;
	double local_time_from;
	double local_time_to;
	double reference_time_from;
	double reference_time_to;
};


// Specifies which canonical axis represents up in the system (typically Y or Z).
enum UpVector
{
	UpVector_AxisX = 0,
	UpVector_AxisY = 1,
	UpVector_AxisZ = 2
};


// Specifies the third vector of the system.
enum CoordSystem
{
	CoordSystem_RightHanded = 0,
	CoordSystem_LeftHanded = 1
};


// http://docs.autodesk.com/FBX/2014/ENU/FBX-SDK-Documentation/index.html?url=cpp_ref/class_fbx_time.html,topicNumber=cpp_ref_class_fbx_time_html29087af6-8c2c-4e9d-aede-7dc5a1c2436c,hash=a837590fd5310ff5df56ffcf7c394787e
enum FrameRate
{
	FrameRate_DEFAULT = 0,
	FrameRate_120 = 1,
	FrameRate_100 = 2,
	FrameRate_60 = 3,
	FrameRate_50 = 4,
	FrameRate_48 = 5,
	FrameRate_30 = 6,
	FrameRate_30_DROP = 7,
	FrameRate_NTSC_DROP_FRAME = 8,
	FrameRate_NTSC_FULL_FRAME = 9,
	FrameRate_PAL = 10,
	FrameRate_CINEMA = 11,
	FrameRate_1000 = 12,
	FrameRate_CINEMA_ND = 13,
	FrameRate_CUSTOM = 14,
};


struct GlobalSettings
{
	UpVector UpAxis = UpVector_AxisX;
	int UpAxisSign = 1;
	// this seems to be 1-2 in Autodesk (odd/even parity), and 0-2 in Blender (axis as in UpAxis)
	// I recommend to ignore FrontAxis and use just UpVector
	int FrontAxis = 1;
	int FrontAxisSign = 1;
	CoordSystem CoordAxis = CoordSystem_RightHanded;
	int CoordAxisSign = 1;
	int OriginalUpAxis = 0;
	int OriginalUpAxisSign = 1;
	float UnitScaleFactor = 1;
	float OriginalUnitScaleFactor = 1;
	double TimeSpanStart = 0L;
	double TimeSpanStop = 0L;
	FrameRate TimeMode = FrameRate_DEFAULT;
	float CustomFrameRate = -1.0f;
};


struct IScene
{
	virtual void destroy() = 0;
	virtual const IElement* getRootElement() const = 0;
	virtual const Object* getRoot() const = 0;
	virtual const TakeInfo* getTakeInfo(const char* name) const = 0;
	virtual int getGeometryCount() const = 0;
	virtual int getMeshCount() const = 0;
	virtual float getSceneFrameRate() const = 0;
	virtual const GlobalSettings* getGlobalSettings() const = 0;
	virtual const Mesh* getMesh(int index) const = 0;
	virtual const Geometry* getGeometry(int index) const = 0;
	virtual int getAnimationStackCount() const = 0;
	virtual const AnimationStack* getAnimationStack(int index) const = 0;
	virtual const Object* const* getAllObjects() const = 0;
	virtual int getAllObjectCount() const = 0;
	virtual int getEmbeddedDataCount() const = 0;
	virtual DataView getEmbeddedData(int index) const = 0;
	virtual DataView getEmbeddedFilename(int index) const = 0;

protected:
	virtual ~IScene() {}
};


IScene* load(const u8* data, int size, u64 flags, JobProcessor job_processor = nullptr, void* job_user_ptr = nullptr);
const char* getError();
double fbxTimeToSeconds(i64 value);
i64 secondsToFbxTime(double value);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // namespace ofbx
