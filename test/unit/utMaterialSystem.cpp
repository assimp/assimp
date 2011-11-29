
#include "UnitTestPCH.h"
#include "utMaterialSystem.h"


CPPUNIT_TEST_SUITE_REGISTRATION (MaterialSystemTest);

// ------------------------------------------------------------------------------------------------
void MaterialSystemTest :: setUp (void)
{
	this->pcMat = new aiMaterial();
}

// ------------------------------------------------------------------------------------------------
void MaterialSystemTest :: tearDown (void)
{
	delete this->pcMat;
}

// ------------------------------------------------------------------------------------------------
void  MaterialSystemTest :: testFloatProperty (void)
{
	float pf = 150392.63f;
	this->pcMat->AddProperty(&pf,1,"testKey1");
	pf = 0.0f;

	CPPUNIT_ASSERT(AI_SUCCESS == pcMat->Get("testKey1",0,0,pf));
	CPPUNIT_ASSERT(pf == 150392.63f);
}

// ------------------------------------------------------------------------------------------------
void  MaterialSystemTest :: testFloatArrayProperty (void)
{
	float pf[] = {0.0f,1.0f,2.0f,3.0f};
	unsigned int pMax = sizeof(pf) / sizeof(float);
	this->pcMat->AddProperty(&pf,pMax,"testKey2");
	pf[0] = pf[1] = pf[2] = pf[3] = 12.0f;

	CPPUNIT_ASSERT(AI_SUCCESS == pcMat->Get("testKey2",0,0,pf,&pMax));
	CPPUNIT_ASSERT(pMax == sizeof(pf) / sizeof(float));
	CPPUNIT_ASSERT(!pf[0] && 1.0f == pf[1] && 2.0f == pf[2] && 3.0f == pf[3] );
}

// ------------------------------------------------------------------------------------------------
void  MaterialSystemTest :: testIntProperty (void)
{
	int pf = 15039263;
	this->pcMat->AddProperty(&pf,1,"testKey3");
	pf = 12;

	CPPUNIT_ASSERT(AI_SUCCESS == pcMat->Get("testKey3",0,0,pf));
	CPPUNIT_ASSERT(pf == 15039263);
}

// ------------------------------------------------------------------------------------------------
void  MaterialSystemTest :: testIntArrayProperty (void)
{
	int pf[] = {0,1,2,3};
	unsigned int pMax = sizeof(pf) / sizeof(int);
	this->pcMat->AddProperty(&pf,pMax,"testKey4");
	pf[0] = pf[1] = pf[2] = pf[3] = 12;

	CPPUNIT_ASSERT(AI_SUCCESS == pcMat->Get("testKey4",0,0,pf,&pMax));
	CPPUNIT_ASSERT(pMax == sizeof(pf) / sizeof(int));
	CPPUNIT_ASSERT(!pf[0] && 1 == pf[1] && 2 == pf[2] && 3 == pf[3] );
}

// ------------------------------------------------------------------------------------------------
void  MaterialSystemTest :: testColorProperty (void)
{
	aiColor4D clr;
	clr.r = 2.0f;clr.g = 3.0f;clr.b = 4.0f;clr.a = 5.0f;
	this->pcMat->AddProperty(&clr,1,"testKey5");
	clr.b = 1.0f;
	clr.a = clr.g = clr.r = 0.0f;

	CPPUNIT_ASSERT(AI_SUCCESS == pcMat->Get("testKey5",0,0,clr));
	CPPUNIT_ASSERT(clr.r == 2.0f && clr.g == 3.0f && clr.b == 4.0f && clr.a == 5.0f);
}

// ------------------------------------------------------------------------------------------------
void  MaterialSystemTest :: testStringProperty (void)
{
	aiString s;
	s.Set("Hello, this is a small test");
	this->pcMat->AddProperty(&s,"testKey6");
	s.Set("358358");
	CPPUNIT_ASSERT(AI_SUCCESS == pcMat->Get("testKey6",0,0,s));
	CPPUNIT_ASSERT(!::strcmp(s.data,"Hello, this is a small test"));
}
