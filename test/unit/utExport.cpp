 
#include "UnitTestPCH.h"
#include "utExport.h"

#ifndef ASSIMP_BUILD_NO_EXPORT

CPPUNIT_TEST_SUITE_REGISTRATION (ExporterTest);

void ExporterTest :: setUp (void)
{
	
	ex = new Assimp::Exporter();
	im = new Assimp::Importer();

	pTest = im->ReadFile("../../test/models/X/test.x",0);
}


void ExporterTest :: tearDown (void)
{
	delete ex;
	delete im;
}


void  ExporterTest :: testExportToFile (void)
{
	const char* file = "unittest_output.dae";
	CPPUNIT_ASSERT_EQUAL(AI_SUCCESS,ex->Export(pTest,"collada",file));

	// check if we can read it again
	CPPUNIT_ASSERT(im->ReadFile(file,0));
}


void  ExporterTest :: testExportToBlob (void)
{
	const aiExportDataBlob* blob = ex->ExportToBlob(pTest,"collada");
	CPPUNIT_ASSERT(blob);
	CPPUNIT_ASSERT(blob->data);
	CPPUNIT_ASSERT(blob->size > 0);
	CPPUNIT_ASSERT(!blob->name.length); 

	// XXX test chained blobs (i.e. obj file with accompanying mtl script)

	// check if we can read it again
	CPPUNIT_ASSERT(im->ReadFileFromMemory(blob->data,blob->size,0,"dae"));
}


void  ExporterTest :: testCppExportInterface (void)
{
	CPPUNIT_ASSERT(ex->GetExportFormatCount() > 0);
	for(size_t i = 0; i < ex->GetExportFormatCount(); ++i) {
		const aiExportFormatDesc* const desc = ex->GetExportFormatDescription(i);
		CPPUNIT_ASSERT(desc);
		CPPUNIT_ASSERT(desc->description && strlen(desc->description));
		CPPUNIT_ASSERT(desc->fileExtension && strlen(desc->fileExtension));
		CPPUNIT_ASSERT(desc->id && strlen(desc->id));
	}

	CPPUNIT_ASSERT(ex->IsDefaultIOHandler());
}


void  ExporterTest :: testCExportInterface (void)
{
	CPPUNIT_ASSERT(aiGetExportFormatCount() > 0);
	for(size_t i = 0; i < aiGetExportFormatCount(); ++i) {
		const aiExportFormatDesc* const desc = aiGetExportFormatDescription(i);
		CPPUNIT_ASSERT(desc);
		// rest has aleady been validated by testCppExportInterface
	}
}

#endif
