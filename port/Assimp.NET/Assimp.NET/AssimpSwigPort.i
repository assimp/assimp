/* File : example.i */
%module Assimp
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

//%rename(Node) aiNode;

%include "std_string.i"

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




