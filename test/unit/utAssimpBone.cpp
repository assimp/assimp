#include "UnitTestPCH.h"

#include "../../include/assimp/postprocess.h"
#include "../../include/assimp/scene.h"
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <BaseImporter.h>

#ifndef ASSIMP_BUILD_NO_EXPORT


using namespace std;
using namespace Assimp;

class AssimpBoneTest : public ::testing::Test
{
public:

	virtual void SetUp() { 
		pImp = new Importer();
		pExp = new Exporter();
 }
	virtual void TearDown() { 
		delete pImp;
		delete pExp;
 }

protected:
	Importer* pImp;
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
TEST_F(AssimpBoneTest, exportShouldKeepObjectBones)
{

	const aiScene* sc = pImp->ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/simpleBone.dae",0);

	ASSERT_TRUE(sc != NULL);
	ASSERT_TRUE(sc->mMeshes[0]->HasBones());
	EXPECT_EQ(2, sc->mMeshes[0]->mNumBones);

	pExp->Export(sc,"collada","simpleBoneExp.dae");

	const aiScene* sc2 = pImp->ReadFile("simpleBoneExp.dae",0);

	ASSERT_TRUE(sc2 != NULL);
	ASSERT_TRUE(sc2->mMeshes[0]->HasBones());
	EXPECT_EQ(2, sc2->mMeshes[0]->mNumBones);

}

#endif // ASSIMP_BUILD_NO_EXPORT
