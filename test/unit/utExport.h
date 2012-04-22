#ifndef INCLUDED_UT_EXPORT_H
#define INCLUDED_UT_EXPORT_H

#ifndef ASSIMP_BUILD_NO_EXPORT

#include <assimp/cexport.h>
#include <assimp/Exporter.hpp>

using namespace Assimp;

class ExporterTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (ExporterTest);
	CPPUNIT_TEST (testExportToFile);
	CPPUNIT_TEST (testExportToBlob);
	CPPUNIT_TEST (testCppExportInterface);
	CPPUNIT_TEST (testCExportInterface);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testExportToFile (void);
		void  testExportToBlob (void);
		void  testCppExportInterface (void);
		void  testCExportInterface (void);
   
	private:

		
		const aiScene* pTest;
		Assimp::Exporter* ex;
		Assimp::Importer* im;
};

#endif

#endif 
