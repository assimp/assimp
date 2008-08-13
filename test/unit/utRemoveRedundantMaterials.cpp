#include "utRemoveRedundantMaterials.h"
#include "aiPostProcess.h"
#include <math.h>

CPPUNIT_TEST_SUITE_REGISTRATION (RemoveRedundantMatsTest);



aiMaterial* getUniqueMaterial1()
{
	// setup an unique name for each material - this shouldn't care
	aiString mTemp;
	mTemp.Set("UniqueMat1");

	MaterialHelper* pcMat = new MaterialHelper();
	pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
	float f = 2.0f;
	pcMat->AddProperty<float>(&f, 1, AI_MATKEY_BUMPSCALING);
	pcMat->AddProperty<float>(&f, 1, AI_MATKEY_SHININESS_STRENGTH);
	return pcMat;
}

aiMaterial* getUniqueMaterial2()
{
	// setup an unique name for each material - this shouldn't care
	aiString mTemp;
	mTemp.Set("UniqueMat2");

	MaterialHelper* pcMat = new MaterialHelper();
	pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
	float f = 4.0f;int i = 1;
	pcMat->AddProperty<float>(&f, 1, AI_MATKEY_BUMPSCALING);
	pcMat->AddProperty<int>(&i, 1, AI_MATKEY_ENABLE_WIREFRAME);
	return pcMat;
}

aiMaterial* getUniqueMaterial3()
{
	// setup an unique name for each material - this shouldn't care
	aiString mTemp;
	mTemp.Set("UniqueMat3");

	MaterialHelper* pcMat = new MaterialHelper();
	pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
	return pcMat;
}

void RemoveRedundantMatsTest :: setUp (void)
{
	// construct the process
	this->piProcess = new RemoveRedundantMatsProcess();

	// create a scene with 5 materials (2 is a duplicate of 0, 3 of 1)
	this->pcScene1 = new aiScene();
	this->pcScene1->mNumMaterials = 5;
	this->pcScene1->mMaterials = new aiMaterial*[5];

	this->pcScene1->mMaterials[0] = getUniqueMaterial1();
	this->pcScene1->mMaterials[1] = getUniqueMaterial2();
	this->pcScene1->mMaterials[4] = getUniqueMaterial3();

	// all materials must be referenced
	this->pcScene1->mNumMeshes = 5;
	this->pcScene1->mMeshes = new aiMesh*[5];
	for (unsigned int i = 0; i < 5;++i)
	{
		this->pcScene1->mMeshes[i] = new aiMesh();
		this->pcScene1->mMeshes[i]->mMaterialIndex = i;
	}

	// setup an unique name for each material - this shouldn't care
	aiString mTemp;
	mTemp.length = 1;
	mTemp.data[0] = 48;
	mTemp.data[1] = 0;

	MaterialHelper* pcMat;
	this->pcScene1->mMaterials[2] = pcMat = new MaterialHelper();
	MaterialHelper::CopyPropertyList(pcMat,(const MaterialHelper*)this->pcScene1->mMaterials[0]);
	pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
	mTemp.data[0]++;

	this->pcScene1->mMaterials[3] = pcMat = new MaterialHelper();
	MaterialHelper::CopyPropertyList(pcMat,(const MaterialHelper*)this->pcScene1->mMaterials[1]);
	pcMat->AddProperty(&mTemp,AI_MATKEY_NAME);
	mTemp.data[0]++;
}

void RemoveRedundantMatsTest :: tearDown (void)
{
	delete this->piProcess;
	delete this->pcScene1;
}

void  RemoveRedundantMatsTest :: testRedundantMaterials (void)
{
	this->piProcess->Execute(this->pcScene1);
	CPPUNIT_ASSERT_EQUAL(this->pcScene1->mNumMaterials,3u);
	CPPUNIT_ASSERT(0 != this->pcScene1->mMaterials && 
		0 != this->pcScene1->mMaterials[0] &&
		0 != this->pcScene1->mMaterials[1] &&
		0 != this->pcScene1->mMaterials[2]);

	aiString sName;
	CPPUNIT_ASSERT(AI_SUCCESS == aiGetMaterialString(this->pcScene1->mMaterials[2],
		AI_MATKEY_NAME,&sName));

	CPPUNIT_ASSERT(!::strcmp( sName.data, "UniqueMat3" ));

}
