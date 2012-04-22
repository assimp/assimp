#ifndef VTADJ_H
#define VTADJ_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "assimp/types.h"
#include "assimp/mesh.h"
#include <VertexTriangleAdjacency.h>


using namespace std;
using namespace Assimp;

class VTAdjacency : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (VTAdjacency);
    CPPUNIT_TEST (largeRandomDataSet);
	CPPUNIT_TEST (smallDataSet);
	CPPUNIT_TEST (unreferencedVerticesSet);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp (void);
        void tearDown (void);

    protected:

        void largeRandomDataSet (void);
		void smallDataSet (void);
		void unreferencedVerticesSet (void);

		void checkMesh(aiMesh* pMesh);
   
	private:

		VertexTriangleAdjacency* pAdj;
		aiMesh* pMesh, *pMesh2, *pMesh3;
};

#endif 
