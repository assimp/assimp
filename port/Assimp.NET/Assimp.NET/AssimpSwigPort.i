/* File : example.i */
%module Assimp_NET
%{
//#include "..\..\..\include\aiAssert.h"
#include "..\..\..\include\aiDefines.h"
#include "..\..\..\include\aiConfig.h"
#include "..\..\..\include\aiTypes.h"
//#include "..\..\..\include\aiVersion.h"
#include "..\..\..\include\aiPostProcess.h"
#include "..\..\..\include\aiVector2D.h"
#include "..\..\..\include\aiVector3D.h"
#include "..\..\..\include\aiMatrix3x3.h"
#include "..\..\..\include\aiMatrix4x4.h"
#include "..\..\..\include\aiCamera.h"
#include "..\..\..\include\aiLight.h"
#include "..\..\..\include\aiAnim.h"
#include "..\..\..\include\aiMesh.h"
#include "..\..\..\include\aiFileIO.h"
#include "..\..\..\include\aiMaterial.h"
#include "..\..\..\include\aiQuaternion.h"
#include "..\..\..\include\aiScene.h"
#include "..\..\..\include\aiTexture.h"
#include "..\..\..\include\assimp.hpp"
#include "..\..\..\include\IOSystem.h"
#include "..\..\..\include\IOStream.h"
#include "..\..\..\include\Logger.h"
#include "..\..\..\include\LogStream.h"
#include "..\..\..\include\NullLogger.h"
%}

#define C_STRUCT
#define C_ENUM
#define ASSIMP_API
#define PACK_STRUCT

%rename(__add__) operator+;
%rename(__addnset__) operator+=;
%rename(__sub__) operator-;
%rename(__subnset__) operator-=;
%rename(__mul__) operator*;
%rename(__mulnset__) operator*=;
%rename(__div__) operator/;
%rename(__divnset__) operator/=;
%rename(__equal__) operator==;
%rename(__nequal__) operator!=;
%rename(__idx__) operator[];
%rename(__set__) operator=;
%rename(__greater__) operator>;
%rename(__smaller__) operator<;


%rename(opNew) operator new;
%rename(opNewArray) operator new[];

%rename(opDelete) operator delete;
%rename(opDeleteArray) operator delete[];

%rename(ReadFile_s) ReadFile(std::string const &,unsigned int);
//%rename(ReadFile_c) ReadFile(char const *,unsigned int);



//%rename(Node) aiNode;

%include "std_string.i"
%include "std_vector.i"


// PACK_STRUCT is a no-op for SWIG – it does not matter for the generated
// bindings how the underlying C++ code manages its memory.
#define PACK_STRUCT


// Helper macros for wrapping the pointer-and-length arrays used in the
// Assimp API.

%define ASSIMP_ARRAY(CLASS, TYPE, NAME, LENGTH)
%newobject CLASS::NAME;
%extend CLASS {
  std::vector<TYPE > *NAME() {
    std::vector<TYPE > *result = new std::vector<TYPE >;
    result->reserve(LENGTH);

    for (unsigned int i = 0; i < LENGTH; ++i) {
      result->push_back($self->NAME[i]);
    }

    return result;
  }
}
%ignore CLASS::NAME;
%enddef

%define ASSIMP_POINTER_ARRAY(CLASS, TYPE, NAME, LENGTH)
%newobject CLASS::NAME;
%extend CLASS {
  std::vector<TYPE *> *NAME() {
    std::vector<TYPE *> *result = new std::vector<TYPE *>;
    result->reserve(LENGTH);

    TYPE *currentValue = $self->NAME;
    TYPE *valueLimit = $self->NAME + LENGTH;
    while (currentValue < valueLimit) {
      result->push_back(currentValue);
      ++currentValue;
    }

    return result;
  }
}
%ignore CLASS::NAME;
%enddef

%define ASSIMP_POINTER_ARRAY_ARRAY(CLASS, TYPE, NAME, OUTER_LENGTH, INNER_LENGTH)
%newobject CLASS::NAME;
%extend CLASS {
  std::vector<std::vector<TYPE *> > *NAME() {
    std::vector<std::vector<TYPE *> > *result = new std::vector<std::vector<TYPE *> >;
    result->reserve(OUTER_LENGTH);

    for (unsigned int i = 0; i < OUTER_LENGTH; ++i) {
      std::vector<TYPE *> currentElements;

      if ($self->NAME[i] != 0) {
        currentElements.reserve(INNER_LENGTH);

        TYPE *currentValue = $self->NAME[i];
        TYPE *valueLimit = $self->NAME[i] + INNER_LENGTH;
        while (currentValue < valueLimit) {
          currentElements.push_back(currentValue);
          ++currentValue;
        }
      }

      result->push_back(currentElements);
    }

    return result;
  }
}
%ignore CLASS::NAME;
%enddef


//%include "..\..\..\include\aiAssert.h"
%include "..\..\..\include\aiDefines.h"
%include "..\..\..\include\aiConfig.h"
%include "..\..\..\include\aiTypes.h"
//%include "..\..\..\include\aiVersion.h"
%include "..\..\..\include\aiPostProcess.h"
%include "..\..\..\include\aiVector2D.h"
%include "..\..\..\include\aiVector3D.h"
%include "..\..\..\include\aiMatrix3x3.h"
%include "..\..\..\include\aiMatrix4x4.h"
%include "..\..\..\include\aiCamera.h"
%include "..\..\..\include\aiLight.h"
%include "..\..\..\include\aiAnim.h"
%include "..\..\..\include\aiMesh.h"
%include "..\..\..\include\aiFileIO.h"
%include "..\..\..\include\aiMaterial.h"
%include "..\..\..\include\aiQuaternion.h"
%include "..\..\..\include\aiScene.h"
%include "..\..\..\include\aiTexture.h"
%include "..\..\..\include\assimp.hpp"
%include "..\..\..\include\IOSystem.h"
%include "..\..\..\include\IOStream.h"
%include "..\..\..\include\Logger.h"
%include "..\..\..\include\LogStream.h"
%include "..\..\..\include\NullLogger.h"

%template(UintVector) std::vector<unsigned int>;
%template(aiAnimationVector) std::vector<aiAnimation *>;
%template(aiCameraVector) std::vector<aiCamera *>;
%template(aiColor4DVector) std::vector<aiColor4D *>;
%template(aiColor4DVectorVector) std::vector<std::vector<aiColor4D *> >;
%template(aiFaceVector) std::vector<aiFace *>;
%template(aiLightVector) std::vector<aiLight *>;
%template(aiMaterialVector) std::vector<aiMaterial *>;
%template(aiMeshVector) std::vector<aiMesh *>;
%template(aiNodeVector) std::vector<aiNode *>;
%template(aiNodeAnimVector) std::vector<aiNodeAnim *>;
%template(aiTextureVector) std::vector<aiTexture *>;
%template(aiVector3DVector) std::vector<aiVector3D *>;
%template(aiVector3DVectorVector) std::vector<std::vector<aiVector3D *> >;


