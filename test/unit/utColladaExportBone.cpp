#include "UnitTestPCH.h"

#include <assimp/cexport.h>
#include "../../include/assimp/postprocess.h"
#include "../../include/assimp/scene.h"
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <BaseImporter.h>

#ifndef ASSIMP_BUILD_NO_EXPORT


using namespace std;
using namespace Assimp;

#define EXPORT_FILE_NAME "simpleBoneExp.dae"

class utColladaExportBone : public ::testing::Test
{
public:

 virtual void SetUp() {
		pImp = new Importer();
		pExp = new Exporter();
		pImp2 = new Importer();
		origModel = pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/simpleBone.dae",0);

		ASSERT_TRUE(origModel != NULL);
		ASSERT_TRUE(origModel->mMeshes[0]->HasBones());

		pExp->Export(origModel,"collada",EXPORT_FILE_NAME);

		readModel = pImp2->ReadFile(EXPORT_FILE_NAME,0);

		ASSERT_TRUE(readModel != NULL);

 }

 virtual void TearDown() {
	 unlink(EXPORT_FILE_NAME);
		delete pImp;
		delete pImp2;
		delete pExp;
 }

protected:
 	const aiScene* origModel;
 	const aiScene* readModel;
	Importer* pImp;
	Importer* pImp2;
	Exporter* pExp;
};

#define AIUT_DEF_ERROR_TEXT "sorry, this is a test"


static const aiImporterDesc desc = {
	"UNIT TEST - IMPORTER",
	"",
	"",
	"",
	0,
	0,
	0,
	0,
	0,
	"apple mac linux windows"
};


class TestPlugin : public BaseImporter
{
public:

	virtual bool CanRead(
		const std::string& pFile, IOSystem* /*pIOHandler*/, bool /*test*/) const
	{
		std::string::size_type pos = pFile.find_last_of('.');
		// no file extension - can't read
		if( pos == std::string::npos)
			return false;
		std::string extension = pFile.substr( pos);

		// todo ... make case-insensitive
		return (extension == ".apple" || extension == ".mac" ||
			extension == ".linux" || extension == ".windows" );

	}

	virtual const aiImporterDesc* GetInfo () const
	{
		return & desc;
	}

	virtual void InternReadFile(
		const std::string& /*pFile*/, aiScene* /*pScene*/, IOSystem* /*pIOHandler*/)
	{
		throw DeadlyImportError(AIUT_DEF_ERROR_TEXT);
	}
};

// ------------------------------------------------------------------------------------------------
TEST_F(utColladaExportBone, exportShouldKeepObjectBones)
{

	ASSERT_TRUE(readModel != NULL);
	ASSERT_TRUE(readModel->mMeshes[0]->HasBones());
	EXPECT_EQ(origModel->mMeshes[0]->mNumBones, readModel->mMeshes[0]->mNumBones);

}

#define FLOAT_EUQAL_TH 1e-5
static void compareMatrix4x4(const aiMatrix4x4 &orig,const aiMatrix4x4 &read){
	EXPECT_NEAR(orig.a1,read.a1,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.a2,read.a2,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.a3,read.a3,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.a4,read.a4,FLOAT_EUQAL_TH);

	EXPECT_NEAR(orig.b1,read.b1,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.b2,read.b2,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.b3,read.b3,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.b4,read.b4,FLOAT_EUQAL_TH);

	EXPECT_NEAR(orig.c1,read.c1,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.c2,read.c2,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.c3,read.c3,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.c4,read.c4,FLOAT_EUQAL_TH);

	EXPECT_NEAR(orig.d1,read.d1,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.d2,read.d2,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.d3,read.d3,FLOAT_EUQAL_TH);
	EXPECT_NEAR(orig.d4,read.d4,FLOAT_EUQAL_TH);
}

TEST_F(utColladaExportBone, bonesHaveTheSamePosition)
{

	ASSERT_TRUE(readModel != NULL);
	EXPECT_EQ(origModel->mMeshes[0]->mNumBones, readModel->mMeshes[0]->mNumBones);

	aiBone **orig = origModel->mMeshes[0]->mBones;
	aiBone **read = readModel->mMeshes[0]->mBones;

	for(unsigned int i=0; i< origModel->mMeshes[0]->mNumBones; i++){
		compareMatrix4x4(orig[i]->mOffsetMatrix,read[i]->mOffsetMatrix);
	}
}

TEST_F(utColladaExportBone, boneHaveTheSameVertexAndWeigth)
{

	EXPECT_EQ(origModel->mMeshes[0]->mNumBones, readModel->mMeshes[0]->mNumBones);

	aiBone **orig = origModel->mMeshes[0]->mBones;
	aiBone **read = readModel->mMeshes[0]->mBones;

	for(unsigned int i=0; i< origModel->mMeshes[0]->mNumBones; i++){
		EXPECT_EQ(orig[i]->mNumWeights,read[i]->mNumWeights);
		aiVertexWeight *origVertex = orig[i]->mWeights;
		aiVertexWeight *readVertex = read[i]->mWeights;
		for(unsigned int j=0; j< orig[i]->mNumWeights; j++){
			EXPECT_EQ(origVertex[j].mVertexId,
					  readVertex[j].mVertexId);
			EXPECT_NEAR(origVertex[j].mWeight,
					readVertex[j].mWeight,FLOAT_EUQAL_TH);


		}
	}
}

#endif // ASSIMP_BUILD_NO_EXPORT