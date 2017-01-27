#include "UnitTestPCH.h"

#include <assimp/cexport.h>
#include <assimp/Exporter.hpp>


#ifndef ASSIMP_BUILD_NO_EXPORT

class ExporterTest : public ::testing::Test {
public:

    virtual void SetUp()
    {
        ex = new Assimp::Exporter();
        im = new Assimp::Importer();

        pTest = im->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/test.x",0);
    }

    virtual void TearDown()
    {
        delete ex;
        delete im;
    }

protected:

    const aiScene* pTest;
    Assimp::Exporter* ex;
    Assimp::Importer* im;
};

// ------------------------------------------------------------------------------------------------
TEST_F(ExporterTest, testExportToFile)
{
    const char* file = "unittest_output.dae";
    EXPECT_EQ(AI_SUCCESS,ex->Export(pTest,"collada",file));

    // check if we can read it again
    EXPECT_TRUE(im->ReadFile(file,0));
}

// ------------------------------------------------------------------------------------------------
TEST_F(ExporterTest, testExportToBlob)
{
    const aiExportDataBlob* blob = ex->ExportToBlob(pTest,"collada");
    ASSERT_TRUE(blob);
    EXPECT_TRUE(blob->data);
    EXPECT_GT(blob->size,  0U);
    EXPECT_EQ(0U, blob->name.length);

    // XXX test chained blobs (i.e. obj file with accompanying mtl script)

    // check if we can read it again
    EXPECT_TRUE(im->ReadFileFromMemory(blob->data,blob->size,0,"dae"));
}

// ------------------------------------------------------------------------------------------------
TEST_F(ExporterTest, testCppExportInterface)
{
    EXPECT_TRUE(ex->GetExportFormatCount() > 0);
    for(size_t i = 0; i < ex->GetExportFormatCount(); ++i) {
        const aiExportFormatDesc* const desc = ex->GetExportFormatDescription(i);
        ASSERT_TRUE(desc);
        EXPECT_TRUE(desc->description && strlen(desc->description));
        EXPECT_TRUE(desc->fileExtension && strlen(desc->fileExtension));
        EXPECT_TRUE(desc->id && strlen(desc->id));
    }

    EXPECT_TRUE(ex->IsDefaultIOHandler());
}

// ------------------------------------------------------------------------------------------------
TEST_F(ExporterTest, testCExportInterface)
{
    EXPECT_TRUE(aiGetExportFormatCount() > 0);
    for(size_t i = 0; i < aiGetExportFormatCount(); ++i) {
        const aiExportFormatDesc* const desc = aiGetExportFormatDescription(i);
        EXPECT_TRUE(desc);
        // rest has aleady been validated by testCppExportInterface
    }
}

#endif
