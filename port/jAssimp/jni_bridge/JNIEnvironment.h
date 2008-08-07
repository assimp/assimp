/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

#if (!defined AI_JNIENVIRONMENT_H_INCLUDED)
#define AI_JNIENVIRONMENT_H_INCLUDED


#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>

#include <vector>
#include <jni.h>

#include "../../../include/aiAssert.h"
#include "../../../include/aiMesh.h"
#include "../../../include/aiScene.h"


namespace Assimp	{
namespace JNIBridge		{


#define AIJ_GET_HANDLE(__handle__) \
	(JNIEnvironment::Get()-> __handle__)

#define AIJ_GET_DEFAULT_CTOR_HANDLE (__handle__) \
	(JNIEnvironment::Get()-> __handle__ . DefaultCtor)

#define AIJ_GET_CLASS_HANDLE (__handle__) \
	(JNIEnvironment::Get()-> __handle__ . Class)

// ---------------------------------------------------------------------------
/**	@class	JNIThreadData
 *	@brief	Manages a list of JNI data structures that are
 *  private to a thread.
 */
struct JNIThreadData
{
	//! Default constructor
	JNIThreadData() : m_pcEnv(NULL), m_iNumRef(1) {}

	//! Construction from an existing JNIEnv
	JNIThreadData(JNIEnv* env) : m_pcEnv(env), m_iNumRef(1) {}

	//! JNI environment, is attached to the thread
	JNIEnv* m_pcEnv;

	//! Number of Importer instances that have been
	//! created by this thread
	unsigned int m_iNumRef;
};


// ---------------------------------------------------------------------------
/**	@class	JNIEnvironment
 *	@brief	Helper class to manage the JNI environment for multithreaded
 *  use of the library.
 */
class JNIEnvironment
{
private:

	JNIEnvironment() : m_iRefCnt(1) {}

public:

	//! Create the JNI environment class
	//! refcnt = 1
	static JNIEnvironment* Create()
	{
		if (NULL == s_pcEnv)
		{
			s_pcEnv = new JNIEnvironment();
		}
		else s_pcEnv->AddRef();
		return s_pcEnv;
	}

	//! static getter for the singleton instance
	//! doesn't hange the reference counter
	static JNIEnvironment* Get()
	{
		ai_assert(NULL != s_pcEnv);
		return s_pcEnv;
	}

	//! COM-style reference counting mechanism
	unsigned int AddRef()
	{
		return ++this->m_iRefCnt;
	}

	//! COM-style reference counting mechanism
	unsigned int Release()
	{
		unsigned int iNew = --this->m_iRefCnt;
		if (0 == iNew)delete this;
		return iNew;
	}

	//! Attach to the current thread
	bool AttachToCurrentThread (JNIEnv* pcEnv);

	//! Detach from the current thread
	bool DetachFromCurrentThread ();

	//! Get the thread local data of the current thread
	JNIThreadData* GetThread();

	//! Throw an NativeEror exception with the specified error message
	//! The error message is optional.
	void ThrowNativeError(const char* msg = NULL);

public:



	struct _java
	{
		inline void Initialize()
		{
			lang.Initialize();
		}

		struct _lang
		{
			inline void Initialize()
			{
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


			struct _Array
			{
				void Initialize();

				jclass FloatArray_Class;
				jclass IntArray_Class;

			} Array;

		} lang;
	} java;



	//! Represents the JNI interface to the package assimp
	struct _assimp 
	{
		//! Initializes the package assimp for use with jAssimp
		inline void Initialize()
		{
			// the NativeError class must be initialized first as it
			// is used by all other class initializers
			NativeException.Initialize();

			// now initialize all other classes, the rder doesn't care.
			Scene.Initialize();
			Importer.Initialize();
			Mesh.Initialize();
			Bone.Initialize();
			Animation.Initialize();
			BoneAnim.Initialize();
			Texture.Initialize();
			CompressedTexture.Initialize();
			Matrix3x3.Initialize();
			Matrix4x4.Initialize();
			Quaternion.Initialize();
			Node.Initialize();
			Material.Initialize();
		};

		//! Represents the JNI interface to class assimp.NativeException
		struct _NativeException
		{
			void Initialize();

			jclass Class;

		} NativeException;


		//! Represents the JNI interface to class assimp.Importer
		struct _Importer
		{
			void Initialize();

			jclass Class;

			jfieldID scene;

		} Importer;


		//! Represents the JNI interface to class assimp.Scene
		struct _Scene
		{
			void Initialize();

			jclass Class;

			jmethodID DefaultCtor;

			jfieldID m_vMeshes;
			jfieldID m_vTextures;
			jfieldID m_vMaterials;
			jfieldID m_vAnimations;
			jfieldID m_rootNode;
			jfieldID flags;

		} Scene;


		//! Represents the JNI interface to class assimp.Mesh
		struct _Mesh
		{
			void Initialize();
			void Fill(jobject obj,const aiMesh* pcSrc);

			jclass Class;

			jmethodID DefaultCtor;

			jfieldID m_vVertices;
			jfieldID m_vTangents;
			jfieldID m_vBitangents;
			jfieldID m_vNormals;
			jfieldID m_avUVs;
			jfieldID m_vFaces;
			jfieldID m_avColors;
			jfieldID m_aiNumUVComponents;
			jfieldID m_vBones;
			jfieldID m_iMaterialIndex;

		} Mesh;

		//! Represents the JNI interface to class assimp.Bone
		struct _Bone
		{
			void Initialize();

			jclass Class;

			jmethodID DefaultCtor;

			jfieldID name;
			jfieldID weights;

		} Bone;

		//! Represents the JNI interface to class assimp.Animation
		struct _Animation
		{
			void Initialize();

			jclass Class;

			jmethodID DefaultCtor;

			jfieldID name;
			jfieldID mDuration;
			jfieldID mTicksPerSecond;
			jfieldID boneAnims;

		} Animation;

		//! Represents the JNI interface to class assimp.BoneAnim
		struct _BoneAnim
		{
			void Initialize();


			//! Represents the JNI interface to class assimp.BoneAnim.KeyFrame<quak>
			struct _KeyFrame
			{
				jclass Class;

				jmethodID DefaultCtor;

				jfieldID time;
				jfieldID value;

			} KeyFrame;

			jclass Class;

			jmethodID DefaultCtor;

			jfieldID mName;
			jfieldID mQuatKeys;
			jfieldID mPosKeys;
			jfieldID mScalingKeys;

		} BoneAnim;

		//! Represents the JNI interface to class assimp.Texture
		struct _Texture
		{
			void Initialize();

			jclass Class;

			jmethodID DefaultCtor;

			jfieldID width;
			jfieldID height;
			jfieldID data;

		} Texture;

		//! Represents the JNI interface to class assimp.CompressedTexture
		struct _CompressedTexture
		{
			void Initialize();

			jclass Class;
			jmethodID DefaultCtor;
			jfieldID m_format;

		} CompressedTexture;

		//! Represents the JNI interface to class assimp.Material
		struct _Material
		{
			void Initialize();

			//! Represents the JNI interface to class assimp.Material.Property
			struct _Property
			{
				jclass Class;

				jfieldID key;
				jfieldID value;

			} Property;

			jclass Class;
			jmethodID DefaultCtor;
			jfieldID properties;

		} Material;

		//! Represents the JNI interface to class assimp.Matrix4x4
		struct _Matrix4x4
		{
			void Initialize();

			jclass Class;
			jmethodID DefaultCtor;
			jfieldID coeff;

		} Matrix4x4;

		//! Represents the JNI interface to class assimp.Matrix3x3
		struct _Matrix3x3
		{
			void Initialize();

			jclass Class;
			jmethodID DefaultCtor;
			jfieldID coeff;

		} Matrix3x3;

		//! Represents the JNI interface to class assimp.Quaternion
		struct _Quaternion
		{
			void Initialize();

			jclass Class;

			jmethodID DefaultCtor;

			jfieldID x;
			jfieldID y;
			jfieldID z;
			jfieldID w;

		} Quaternion;

		//! Represents the JNI interface to class assimp.Node
		struct _Node
		{
			void Initialize();

			jclass Class;

			jmethodID DefaultCtor;

			jfieldID meshIndices;
			jfieldID nodeTransform;
			jfieldID name;
			jfieldID children;
			jfieldID parent;

		} Node;

	} assimp;

private:

	//! Singleton instance
	static JNIEnvironment* s_pcEnv;

	//! TLS data 
	boost::thread_specific_ptr<JNIThreadData> ptr;

	//! Reference counter of the whole class
	unsigned int m_iRefCnt;
};

};};


// ---------------------------------------------------------------------------
/** @brief Helper function to copy data to a Java array
 *
 * @param jfl Java array
 * @param data Input data
 * @param size Size of data to be copied, in bytes
 */
JNU_CopyDataToArray(jarray jfl, void* data, unsigned int size);

// ---------------------------------------------------------------------------
/** @brief Helper function to create a java.lang.String from
 *  a native char*.
 *
 * This function has been taken from
 * http://java.sun.com/docs/books/jni/html/other.html#26021
 * I am not sure whether it is really necessary, but I trust the source
 */
jstring JNU_NewStringNative(JNIEnv *env, const char *str);


// ---------------------------------------------------------------------------
/** @brief Helper function to create a char* from
 *  a managed jstring
 *
 * This function has been taken from
 * http://java.sun.com/docs/books/jni/html/other.html#26021
 * I am not sure whether it is really necessary, but I trust the source
 */
char* JNU_GetStringNativeChars(JNIEnv *env, jstring jstr);

#endif //! AI_JNIENVIRONMENT_H_INCLUDED


