
/* --------------------------------------------------------------------------------
 *
 * Open Asset Import Library (ASSIMP) (http://assimp.sourceforge.net)
 * Assimp2Java bridge 
 *
 * Copyright (c) 2006-2009, ASSIMP Development Team
 * All rights reserved. See the LICENSE file for more information.
 *
 * --------------------------------------------------------------------------------
 */

#ifndef INCLUDED_JBRIDGE_ENVIRONMENT_H
#define INCLUDED_JBRIDGE_ENVIRONMENT_H

namespace Assimp	{
namespace jbridge		{

// -----------------------------------------------------------------------------------------
// Nasty macros. One day I'll probably switch to a generic and beautiful solution, but 
// this day is still far, far away
// -----------------------------------------------------------------------------------------

#define _AI_CONCAT(a,b)  a ## b
#define  AI_CONCAT(a,b)  _AI_CONCAT(a,b)

#define AIJ_GET_JNIENV() JNIEnvironment::Get()->GetThread()->m_pcEnv

#define AIJ_BEGIN_CLASS(name) \
	struct AI_CONCAT(_,name) : public _Base { \
		void Initialize(); \

#define AIJ_END_CLASS(name) \
	} name ; 

#define AIJ_SET_FIELD_TYPE(field,type) \
	const char* AI_CONCAT(QueryFieldString_,field) () { \
		return type; \
		} \
	jfieldID field;

#define AIJ_SET_CLASS_TYPE(ctype,jtype) \
	const char* QueryClassString() { \
			return jtype; \
		} \
	void Fill(jobject obj,const ctype* pcSrc); \
    \
	inline void Fill(jobject obj,const void* pcSrc) { \
		Fill(obj,(const ctype*)pcSrc); \
	}

#define AIJ_LOAD_CLASS() \
	JNIEnv* pc = AIJ_GET_JNIENV(); \
	\
	if(!(Class = pc->FindClass(QueryClassString()))) { \
		JNIEnvironment::Get()->ThrowNativeError(std::string("Failed to load class ") + \
			QueryClassString()); \
	} \
	DefaultCtor = pc->GetMethodID(Class,"<init>","");

#define AIJ_LOAD_FIELD(name) { \
	const char* t = AI_CONCAT(QueryFieldString_,name)(); \
	name = pc->GetFieldID(Class,# name,t); \
	if (!name) \
		JNIEnvironment::Get()->ThrowNativeError(std::string("Failed to load ") + \
			QueryClassString() + "#" + t); \
	}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

#define AIJ_GET_HANDLE(h)              (JNIEnvironment::Get()->h)
#define AIJ_GET_DEFAULT_CTOR_HANDLE(h) (JNIEnvironment::Get()->h.DefaultCtor)
#define AIJ_GET_CLASS_HANDLE(h)        (JNIEnvironment::Get()->h.Class)

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
/**	
 *	@brief	Manages a list of JNI data structures private to a thread.
 */
struct JNIThreadData
{
	//! Default constructor
	JNIThreadData() 
		: m_pcEnv(NULL), m_iNumRef(1) {}

	//! Construction from an existing JNIEnv
	JNIThreadData(JNIEnv* env) : m_pcEnv(env), m_iNumRef(1) {}

	//! JNI environment, is attached to the thread
	JNIEnv* m_pcEnv;

	//! Number of Importer instances that have been created by this thread
	unsigned int m_iNumRef;
};


// -----------------------------------------------------------------------------------------
/**	@brief	Helper class to manage the JNI environment for multithreaded calls.
 *
 *  It's not so simple. Java users have to create multiple instances of assimp.Importer
 *  to access Assimp from multiple threads, but we need to synchronize everything to a
 *  single Java logging implementation.
 */
class JNIEnvironment
{
private:

	JNIEnvironment()
		: m_iRefCnt(1) {}

public:

	/** Attach a new thread to the environment */
	static JNIEnvironment* Create()	{
		if (NULL == s_pcEnv)	{
			s_pcEnv = new JNIEnvironment();
		}
		else s_pcEnv->AddRef();
		return s_pcEnv;
	}

	/** Static getter  */
	static JNIEnvironment* Get()	{
		ai_assert(NULL != s_pcEnv);
		return s_pcEnv;
	}

	/** Add a reference to the environment */
	unsigned int AddRef()	{
		return ++m_iRefCnt;
	}

	/** Remove a reference from the environment */
	unsigned int Release()	{

		const unsigned int iNew = --m_iRefCnt;
		if (0 == iNew) {
			delete this;
		}
		return iNew;
	}

	/** Attach a JNIEnv to the current thread */
	bool AttachToCurrentThread (JNIEnv* pcEnv);

	/** Detach from the current thread */
	bool DetachFromCurrentThread ();

	/** Get the thread local data of the current thread */
	JNIThreadData* GetThread();

	/** Throw an NativeException exception with the specified error message
	 *  The error message itself is optional. */
	void ThrowNativeError(const std::string& msg = std::string("Unknown error"));

public:


	// ------------------------------------------------------------------------------------
	// Reflects the part of the Java class library which we need for our dealings 
	//
	struct _java
	{
		void Initialize()	{
			lang.Initialize();
		}

		struct _lang
		{
			void Initialize()	{
				String.Initialize();
			}

			struct _String
			{
				void Initialize();

				//! Handle to the java.lang.String class
				static jclass Class;

				//! Handle to the java.lang.String.getBytes() class
				static jmethodID getBytes;

				//! Handle to the java.lang.String.String(byte[]) c'tor
				static jmethodID constructor_ByteArray;

			} String;

			struct _Array	{
				void Initialize();

				jclass FloatArray_Class;
				jclass IntArray_Class;

			} Array;

		} lang;
	} java;

	struct _Base	{
		virtual void Fill(jobject obj,const void* pcSrc) {}

		//! Handle to class
		jclass Class;

		//! Handle to default c'tor
		jmethodID DefaultCtor;
	};

	// ------------------------------------------------------------------------------------
	// Reflects the classes of the assimp package.
	//
	struct _assimp 
	{
		//! Initializes the package assimp for use with our bridge module
		inline void Initialize()	{

			// The NativeException class must be initialized first as it
			// is used by all other class initializion routines
			NativeException.Initialize();
			Importer.Initialize();

			// now initialize all other classes, the order doesn't care.
			Face.Initialize();
			Scene.Initialize();
			Importer.Initialize();
			Mesh.Initialize();
			Bone.Initialize();
			Animation.Initialize();
			NodeAnim.Initialize();
			Texture.Initialize();
			CompressedTexture.Initialize();
			Matrix3x3.Initialize();
			Matrix4x4.Initialize();
			Quaternion.Initialize();
			Node.Initialize();
			Material.Initialize();
			Camera.Initialize();
			Light.Initialize();
		};

		//! Represents the JNI interface to class assimp.NativeException
		AIJ_BEGIN_CLASS(NativeException)	
		AIJ_END_CLASS(NativeException)

		//! Represents the JNI interface to class assimp.Importer
		AIJ_BEGIN_CLASS(Importer)
			AIJ_SET_CLASS_TYPE(Assimp::Importer,"assimp.Importer");
			AIJ_SET_FIELD_TYPE(scene,"Lassimp.Scene");

		AIJ_END_CLASS(Importer)

		//! Represents the JNI interface to class assimp.Scene
		AIJ_BEGIN_CLASS(Scene)
			AIJ_SET_CLASS_TYPE(aiScene,"assimp.Scene");

			AIJ_SET_FIELD_TYPE(m_vTextures,"[Lassimp.Texture");
			AIJ_SET_FIELD_TYPE(m_vCameras,"[Lassimp.Camera");
			AIJ_SET_FIELD_TYPE(m_vLights,"[Lassimp.Light");
			AIJ_SET_FIELD_TYPE(m_vMeshes,"[Lassimp.Mesh");
			AIJ_SET_FIELD_TYPE(m_vMaterials,"[Lassimp.Material");
			AIJ_SET_FIELD_TYPE(m_vAnimations,"[Lassimp.Animation");
			AIJ_SET_FIELD_TYPE(m_rootNode,"[Lassimp.Node");
			AIJ_SET_FIELD_TYPE(flags,"I");
	
		AIJ_END_CLASS(Scene)

		//! Represents the JNI interface to class assimp.Mesh
		AIJ_BEGIN_CLASS(Mesh)
			AIJ_SET_CLASS_TYPE(aiMesh,"assimp.Mesh");
			
			AIJ_SET_FIELD_TYPE(m_iPrimitiveTypes,"I");
			AIJ_SET_FIELD_TYPE(m_vVertices,"[[F");
			AIJ_SET_FIELD_TYPE(m_vTangents,"[[F");
			AIJ_SET_FIELD_TYPE(m_vBitangents,"[[F");
			AIJ_SET_FIELD_TYPE(m_vNormals,"[[F");
			AIJ_SET_FIELD_TYPE(m_avUVs,"[[F");
			AIJ_SET_FIELD_TYPE(m_vFaces,"[[F");
			AIJ_SET_FIELD_TYPE(m_avColors,"[[F");
			AIJ_SET_FIELD_TYPE(m_aiNumUVComponents,"[I");
			AIJ_SET_FIELD_TYPE(m_vBones,"[Lassimp.Bone");
			AIJ_SET_FIELD_TYPE(m_iMaterialIndex,"I");
		AIJ_END_CLASS(Mesh)

		//! Represents the JNI interface to class assimp.Face
		AIJ_BEGIN_CLASS(Face)
			AIJ_SET_CLASS_TYPE(aiFace,"assimp.Face");

			AIJ_SET_FIELD_TYPE(indices,"LI");
		AIJ_END_CLASS(Face)

		//! Represents the JNI interface to class assimp.Bone
		AIJ_BEGIN_CLASS(Bone)
			AIJ_SET_CLASS_TYPE(aiBone,"assimp.Bone");

			AIJ_SET_FIELD_TYPE(name,"Ljava.lang.String");
			AIJ_SET_FIELD_TYPE(weights,"[Lassimp.Bone.Weight");

			//! Represents the JNI interface to class assimp.Bone.Weight
			AIJ_BEGIN_CLASS(Weight)
				AIJ_SET_CLASS_TYPE(aiVertexWeight,"assimp.Bone.Weight");

				AIJ_SET_FIELD_TYPE(index,"I");
				AIJ_SET_FIELD_TYPE(weight,"F");
			AIJ_END_CLASS(Weight)
		AIJ_END_CLASS(Bone)

		//! Represents the JNI interface to class assimp.Animation
		AIJ_BEGIN_CLASS(Animation)
			AIJ_SET_CLASS_TYPE(aiAnimation,"assimp.Animation");

			AIJ_SET_FIELD_TYPE(name,"Ljava.lang.String");
			AIJ_SET_FIELD_TYPE(mDuration,"D");
			AIJ_SET_FIELD_TYPE(mTicksPerSecond,"D");
			AIJ_SET_FIELD_TYPE(nodeAnims,"[Lassimp.NodeAnim");
		AIJ_END_CLASS(Animation)

		//! Represents the JNI interface to class assimp.NodeAnim
		AIJ_BEGIN_CLASS(NodeAnim)
			AIJ_SET_CLASS_TYPE(aiNodeAnim,"assimp.NodeAnim");

			//! Represents the JNI interface to class assimp.BoneAnim.KeyFrame<float[]>
			AIJ_BEGIN_CLASS(VectorKey)
				AIJ_SET_CLASS_TYPE(aiVectorKey,"Lassimp.NodeAnim.KeyFrame<[F>");
				
				AIJ_SET_FIELD_TYPE(time,"D");
				AIJ_SET_FIELD_TYPE(value,"[F");
			AIJ_END_CLASS(VectorKey)

			//! Represents the JNI interface to class assimp.BoneAnim.KeyFrame<assimp.Quaternion>
			AIJ_BEGIN_CLASS(QuatKey)
				AIJ_SET_CLASS_TYPE(aiQuatKey,"Lassimp.NodeAnim.KeyFrame<Lassimp.Quaternion>");
				
				AIJ_SET_FIELD_TYPE(time,"D");
				AIJ_SET_FIELD_TYPE(value,"Lassimp.Quaternion");
			AIJ_END_CLASS(QuatKey)

			AIJ_SET_FIELD_TYPE(mName,"Ljava.lang.String");
			AIJ_SET_FIELD_TYPE(mQuatKeys,"[Lassimp.NodeAnim.KeyFrame<Lassimp.Quaternion>");
			AIJ_SET_FIELD_TYPE(mPosKeys, "[Lassimp.NodeAnim.KeyFrame<[F>");
			AIJ_SET_FIELD_TYPE(mScalingKeys,"[Lassimp.NodeAnim.KeyFrame<[F>");
		AIJ_END_CLASS(NodeAnim)

		//! Represents the JNI interface to class assimp.Texture
		AIJ_BEGIN_CLASS(Texture)
			AIJ_SET_CLASS_TYPE(aiTexture,"assimp.Texture");

			AIJ_SET_FIELD_TYPE(width,"I");
			AIJ_SET_FIELD_TYPE(height,"I");
			AIJ_SET_FIELD_TYPE(data,"[b");
		AIJ_END_CLASS(Texture)

		//! Represents the JNI interface to class assimp.CompressedTexture
		AIJ_BEGIN_CLASS(CompressedTexture)
			AIJ_SET_CLASS_TYPE(aiTexture,"assimp.CompressedTexture");
			AIJ_SET_FIELD_TYPE(m_format,"Ljava.lang.String");
		AIJ_END_CLASS(CompressedTexture)

		//! Represents the JNI interface to class assimp.Material
		AIJ_BEGIN_CLASS(Material)
			AIJ_SET_CLASS_TYPE(aiMaterial,"assimp.Material");

			//! Represents the JNI interface to class assimp.Material.Property
			AIJ_BEGIN_CLASS(Property)
				AIJ_SET_CLASS_TYPE(aiMaterialProperty,"assimp.Material.Property");

				AIJ_SET_FIELD_TYPE(key,"L.java.lang.String");
				AIJ_SET_FIELD_TYPE(value,"Ljava.lang.Object");
			AIJ_END_CLASS(Property)
			
			AIJ_SET_FIELD_TYPE(properties,"[Lassimp.Material.Property");
		AIJ_END_CLASS(Material)

		//! Represents the JNI interface to class assimp.Matrix4x4
		AIJ_BEGIN_CLASS(Matrix4x4)
			AIJ_SET_CLASS_TYPE(aiMatrix4x4,"assimp.Matrix4x4");
			AIJ_SET_FIELD_TYPE(coeff,"[F");
		AIJ_END_CLASS(Matrix4x4)

		//! Represents the JNI interface to class assimp.Matrix3x3
		AIJ_BEGIN_CLASS(Matrix3x3)
			AIJ_SET_CLASS_TYPE(aiMatrix3x3,"assimp.Matrix3x3");
			AIJ_SET_FIELD_TYPE(coeff,"[F");
		AIJ_END_CLASS(Matrix3x3)

		//! Represents the JNI interface to class assimp.Quaternion
		AIJ_BEGIN_CLASS(Quaternion)
			AIJ_SET_CLASS_TYPE(aiQuaternion,"assimp.Quaternion");

			AIJ_SET_FIELD_TYPE(x,"F");
			AIJ_SET_FIELD_TYPE(y,"F");
			AIJ_SET_FIELD_TYPE(z,"F");
			AIJ_SET_FIELD_TYPE(w,"F");
		AIJ_END_CLASS(Quaternion)

		//! Represents the JNI interface to class assimp.Node
		AIJ_BEGIN_CLASS(Node)
			AIJ_SET_CLASS_TYPE(aiNode,"assimp.Node");

			AIJ_SET_FIELD_TYPE(meshIndices,"[I");
			AIJ_SET_FIELD_TYPE(name,"Ljava.lang.String");
			AIJ_SET_FIELD_TYPE(children,"[Lassimp.Node");
			AIJ_SET_FIELD_TYPE(nodeTransform,"Lassimp.Matrix4x4");
			AIJ_SET_FIELD_TYPE(parent,"Lassimp.Node");
		AIJ_END_CLASS(Node)

		//! Represents the JNI interface to class assimp.Camera
		AIJ_BEGIN_CLASS(Camera)
			AIJ_SET_CLASS_TYPE(aiCamera,"assimp.Camera");

			AIJ_SET_FIELD_TYPE(mName,"Ljava.lang.String");
			AIJ_SET_FIELD_TYPE(mPosition,"[F");
			AIJ_SET_FIELD_TYPE(mUp,"[F");
			AIJ_SET_FIELD_TYPE(mLookAt,"[F");
			AIJ_SET_FIELD_TYPE(mHorizontalFOV,"F");
			AIJ_SET_FIELD_TYPE(mClipPlaneNear,"F");
			AIJ_SET_FIELD_TYPE(mClipPlaneFar,"F");
			AIJ_SET_FIELD_TYPE(mAspect,"F");
		AIJ_END_CLASS(Camera)

		//! Represents the JNI interface to class assimp.Light
		AIJ_BEGIN_CLASS(Light)
			AIJ_SET_CLASS_TYPE(aiLight,"assimp.Light");

			AIJ_SET_FIELD_TYPE(mName,"[I");
			AIJ_SET_FIELD_TYPE(mType,"I");
			AIJ_SET_FIELD_TYPE(mPosition,"[F");
			AIJ_SET_FIELD_TYPE(mDirection,"[F");
			AIJ_SET_FIELD_TYPE(mAttenuationConstant,"F");
			AIJ_SET_FIELD_TYPE(mAttenuationLinear,"F");
			AIJ_SET_FIELD_TYPE(mAttenuationQuadratic,"F");
			AIJ_SET_FIELD_TYPE(mColorDiffuse,"[F");
			AIJ_SET_FIELD_TYPE(mColorSpecular,"[F");
			AIJ_SET_FIELD_TYPE(mColorAmbient,"[F");
			AIJ_SET_FIELD_TYPE(mAngleInnerCone,"F");
			AIJ_SET_FIELD_TYPE(mAngleOuterCone,"F");
		AIJ_END_CLASS(Light)
	} assimp;

	//! Master initialization of all stuff we need
	void Initialize()
	{
		assimp.Initialize();
		java.Initialize();
	}

private:

	//! Singleton instance
	static JNIEnvironment* s_pcEnv;

	//! TLS data 
	boost::thread_specific_ptr<JNIThreadData> ptr;

	//! Reference counter of the whole class
	unsigned int m_iRefCnt;
};

// ---------------------------------------------------------------------------
/** @brief Helper function to copy data to a Java array
 *
 * @param pc JNI env handle
 * @param jfl Java array
 * @param data Input data
 * @param size Size of data to be copied, in bytes
 */
void JNU_CopyDataToArray(JNIEnv* pc, jarray jfl, void* data, unsigned int size);

// ---------------------------------------------------------------------------
/** @brief Helper function to create a java.lang.String from a native char*.
 *
 * This function has been taken from
 * http://java.sun.com/docs/books/jni/html/other.html#26021
 */
jstring JNU_NewStringNative(JNIEnv *env, const char *str);

// ---------------------------------------------------------------------------
/** @brief Helper function to create a char* from a managed jstring
 *
 * This function has been taken from
 * http://java.sun.com/docs/books/jni/html/other.html#26021
 */
char* JNU_GetStringNativeChars(JNIEnv *env, jstring jstr);

// ---------------------------------------------------------------------------
/** @brief Helper function to copy a whole object array to the VM
 *
 *  @param pc JNI env handle
 *  @param in Input object array
 *  @param num Size of input array
 *  @param type Type of input
 *  @param out Output object
 */
// ---------------------------------------------------------------------------
void JNU_CopyObjectArrayToVM(JNIEnv* pc, const void** in, unsigned int num,
	JNIEnvironment::_Base& type, jobjectArray& out);

}} // end namespaces

using namespace Assimp;
using namespace Assimp::jbridge;

#include "jbridge_Logger.h"
#endif //! AI_JNIENVIRONMENT_H_INCLUDED

