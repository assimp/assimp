
#include "UnitTestPCH.h"
#include "utSharedPPData.h"


CPPUNIT_TEST_SUITE_REGISTRATION (SharedPPDataTest);

static bool destructed;

struct TestType
{
	~TestType()
	{
		destructed = true;
	}
};


// ------------------------------------------------------------------------------------------------
void SharedPPDataTest :: setUp (void)
{
	shared = new SharedPostProcessInfo();
	destructed = false;
}

// ------------------------------------------------------------------------------------------------
void SharedPPDataTest :: tearDown (void)
{
	
}

// ------------------------------------------------------------------------------------------------
void  SharedPPDataTest :: testPODProperty (void)
{
	int i = 5;
	shared->AddProperty("test",i);
	int o;
	CPPUNIT_ASSERT(shared->GetProperty("test",o) && 5 == o);
	CPPUNIT_ASSERT(!shared->GetProperty("test2",o) && 5 == o);

	float f = 12.f, m;
	shared->AddProperty("test",f);
	CPPUNIT_ASSERT(shared->GetProperty("test",m) && 12.f == m);
}

// ------------------------------------------------------------------------------------------------
void  SharedPPDataTest :: testPropertyPointer (void)
{
	int *i = new int[35];
	shared->AddProperty("test16",i);
	int* o;
	CPPUNIT_ASSERT(shared->GetProperty("test16",o) && o == i);
	shared->RemoveProperty("test16");
	CPPUNIT_ASSERT(!shared->GetProperty("test16",o));
}

// ------------------------------------------------------------------------------------------------
void  SharedPPDataTest :: testPropertyDeallocation (void)
{
	TestType *out, * pip = new TestType();
	shared->AddProperty("quak",pip);
	CPPUNIT_ASSERT(shared->GetProperty("quak",out) && out == pip);

	delete shared;
	CPPUNIT_ASSERT(destructed);
}
