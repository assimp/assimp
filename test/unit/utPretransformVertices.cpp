
#include "UnitTestPCH.h"
#include "utPretransformVertices.h"

CPPUNIT_TEST_SUITE_REGISTRATION (PretransformVerticesTest);

// ------------------------------------------------------------------------------------------------
void AddNodes(unsigned int num, aiNode* father, unsigned int depth)
{
	father->mChildren = new aiNode*[father->mNumChildren = 5];
	for (unsigned int i = 0; i < 5; ++i) {
		aiNode* nd = father->mChildren[i] = new aiNode();

		nd->mName.length = sprintf(nd->mName.data,"%i%i",depth,i);

		// spawn two meshes
		nd->mMeshes = new unsigned int[nd->mNumMeshes = 2];
		nd->mMeshes[0] = num*5+i;
		nd->mMeshes[1] = 24-(num*5+i); // mesh 12 is special ... it references the same mesh twice

		// setup an unique transformation matrix
		nd->mTransformation.a1 = num*5.f+i + 1;
	}

	if (depth > 1) {
		for (unsigned int i = 0; i < 5; ++i)
			AddNodes(i, father->mChildren[i],depth-1);
	}
}

// ------------------------------------------------------------------------------------------------
void PretransformVerticesTest :: setUp (void)
{
	scene = new aiScene();

	// add 5 empty materials
	scene->mMaterials = new aiMaterial*[scene->mNumMaterials = 5];
	for (unsigned int i = 0; i < 5;++i)
		scene->mMaterials[i] = new aiMaterial();

	// add 25 test meshes
	scene->mMeshes = new aiMesh*[scene->mNumMeshes = 25];
	for (unsigned int i = 0; i < 25;++i) {
		aiMesh* mesh = scene->mMeshes[i] = new aiMesh();

		mesh->mPrimitiveTypes = aiPrimitiveType_POINT;
		mesh->mFaces = new aiFace[ mesh->mNumFaces = 10+i ];
		mesh->mVertices = new aiVector3D[mesh->mNumVertices = mesh->mNumFaces];
		for (unsigned int a = 0; a < mesh->mNumFaces; ++a ) {
			aiFace& f = mesh->mFaces[a];
			f.mIndices = new unsigned int [f.mNumIndices = 1];
			f.mIndices[0] = a*3;

			mesh->mVertices[a] = aiVector3D((float)i,(float)a,0.f);
		}
		mesh->mMaterialIndex = i%5;

		if (i % 2) 
			mesh->mNormals = new aiVector3D[mesh->mNumVertices];
	}

	// construct some nodes (1+25)
	scene->mRootNode = new aiNode();
	scene->mRootNode->mName.Set("Root");
	AddNodes(0,scene->mRootNode,2);

	process = new PretransformVertices();
}

// ------------------------------------------------------------------------------------------------
void PretransformVerticesTest :: tearDown (void)
{
	delete scene;
	delete process;
}

 // ------------------------------------------------------------------------------------------------
void PretransformVerticesTest :: testProcess_CollapseHierarchy (void)
{
	process->KeepHierarchy(false);
	process->Execute(scene);

	CPPUNIT_ASSERT(scene->mNumMaterials == 5);
	CPPUNIT_ASSERT(scene->mNumMeshes == 10); // every second mesh has normals
}

// ------------------------------------------------------------------------------------------------
void PretransformVerticesTest :: testProcess_KeepHierarchy (void)
{
	
	process->KeepHierarchy(true);
	process->Execute(scene);

	CPPUNIT_ASSERT(scene->mNumMaterials == 5);
	CPPUNIT_ASSERT(scene->mNumMeshes == 49); // see note on mesh 12 above
	
}