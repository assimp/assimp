
#include "UnitTestPCH.h"
#include "utImporter.h"


CPPUNIT_TEST_SUITE_REGISTRATION (ImporterTest);

#define AIUT_DEF_ERROR_TEXT "sorry, this is a test"


bool TestPlugin :: CanRead( const std::string& pFile, 
	IOSystem* pIOHandler, bool test) const
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

void TestPlugin :: GetExtensionList(std::string& append)
{
	append.append("*.apple;*.mac;*.linux;*.windows");
}

void TestPlugin :: InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	throw new ImportErrorException(AIUT_DEF_ERROR_TEXT);
}


void ImporterTest :: setUp (void)
{
	pImp = new Importer();
}

void ImporterTest :: tearDown (void)
{
	delete pImp;
}

void ImporterTest :: testIntProperty (void)
{
	bool b;
	pImp->SetPropertyInteger("quakquak",1503,&b);
	CPPUNIT_ASSERT(!b);
	CPPUNIT_ASSERT(1503 == pImp->GetPropertyInteger("quakquak",0));
	CPPUNIT_ASSERT(314159 == pImp->GetPropertyInteger("not_there",314159));

	pImp->SetPropertyInteger("quakquak",1504,&b);
	CPPUNIT_ASSERT(b);
}

void ImporterTest :: testFloatProperty (void)
{
	bool b;
	pImp->SetPropertyFloat("quakquak",1503.f,&b);
	CPPUNIT_ASSERT(!b);
	CPPUNIT_ASSERT(1503.f == pImp->GetPropertyFloat("quakquak",0.f));
	CPPUNIT_ASSERT(314159.f == pImp->GetPropertyFloat("not_there",314159.f));
}

void ImporterTest :: testStringProperty (void)
{
	bool b;
	pImp->SetPropertyString("quakquak","test",&b);
	CPPUNIT_ASSERT(!b);
	CPPUNIT_ASSERT("test" == pImp->GetPropertyString("quakquak","weghwekg"));
	CPPUNIT_ASSERT("ILoveYou" == pImp->GetPropertyString("not_there","ILoveYou"));
}

void ImporterTest :: testPluginInterface (void)
{
	pImp->RegisterLoader(new TestPlugin());
	CPPUNIT_ASSERT(pImp->IsExtensionSupported(".apple"));
	CPPUNIT_ASSERT(pImp->IsExtensionSupported(".mac"));
	CPPUNIT_ASSERT(pImp->IsExtensionSupported(".linux"));
	CPPUNIT_ASSERT(pImp->IsExtensionSupported(".windows"));
	CPPUNIT_ASSERT(pImp->IsExtensionSupported(".x")); /* x and 3ds must be available of course */
	CPPUNIT_ASSERT(pImp->IsExtensionSupported(".3ds"));
	CPPUNIT_ASSERT(!pImp->IsExtensionSupported("."));

	TestPlugin* p = (TestPlugin*) pImp->FindLoader(".windows");
	CPPUNIT_ASSERT(NULL != p);

	try {
	p->InternReadFile("",0,NULL);
	}
	catch ( ImportErrorException* ex)
	{
		CPPUNIT_ASSERT(ex->GetErrorText() == AIUT_DEF_ERROR_TEXT);

		// unregister the plugin and delete it
		pImp->UnregisterLoader(p);
		delete p;

		return;
	}
	CPPUNIT_ASSERT(false); // control shouldn't reach this point
}

void ImporterTest :: testExtensionCheck (void)
{
	std::string s;
	pImp->GetExtensionList(s);

	// todo ..
}
