#ifndef TESTIMPORTER_H
#define TESTIMPORTER_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/Importer.hpp>
#include <BaseImporter.h>

using namespace std;
using namespace Assimp;

class ImporterTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (ImporterTest);
    CPPUNIT_TEST (testIntProperty);
	CPPUNIT_TEST (testFloatProperty);
	CPPUNIT_TEST (testStringProperty);
	CPPUNIT_TEST (testPluginInterface);
	CPPUNIT_TEST (testExtensionCheck);
	CPPUNIT_TEST (testMemoryRead);
	CPPUNIT_TEST (testMultipleReads);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void  testIntProperty (void);
		void  testFloatProperty (void);
		void  testStringProperty (void);
		
		void  testPluginInterface (void);
		void  testExtensionCheck (void);
		void  testMemoryRead (void);

		void  testMultipleReads (void);

	private:

		Importer* pImp;
};

class TestPlugin : public BaseImporter
{
public:

	// overriden
	bool CanRead( const std::string& pFile, 
		IOSystem* pIOHandler, bool test) const;

	// overriden
	const aiImporterDesc* GetInfo () const;

	// overriden
	void InternReadFile( const std::string& pFile, 
		aiScene* pScene, IOSystem* pIOHandler);
};

#endif 
