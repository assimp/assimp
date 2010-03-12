%{
#include "aiMesh.h"
%}


ASSIMP_ARRAY(aiMesh, unsigned int, mNumUVComponents, AI_MAX_NUMBER_OF_TEXTURECOORDS);
ASSIMP_POINTER_ARRAY(aiMesh, aiVector3D, mVertices, $self->mNumVertices);
ASSIMP_POINTER_ARRAY(aiMesh, aiVector3D, mNormals, $self->mNumVertices);
ASSIMP_POINTER_ARRAY(aiMesh, aiFace, mFaces, $self->mNumFaces);
ASSIMP_POINTER_ARRAY_ARRAY(aiMesh, aiVector3D, mTextureCoords, AI_MAX_NUMBER_OF_TEXTURECOORDS, $self->mNumVertices);
ASSIMP_POINTER_ARRAY_ARRAY(aiMesh, aiColor4D, mColors, AI_MAX_NUMBER_OF_COLOR_SETS, $self->mNumVertices);

ASSIMP_ARRAY(aiFace, unsigned int, mIndices, $self->mNumIndices);


%include "aiMesh.h"
