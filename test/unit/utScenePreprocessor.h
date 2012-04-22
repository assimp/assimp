


#ifndef TESTSCENEPREPROCESSOR_H
#define TESTSCENEPREPROCESSOR_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <ScenePreprocessor.h>

using namespace std;
using namespace Assimp;

class ScenePreprocessorTest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (ScenePreprocessorTest);
    CPPUNIT_TEST (testMeshPreprocessingPos);
	CPPUNIT_TEST (testMeshPreprocessingNeg);
	CPPUNIT_TEST (testAnimationPreprocessingPos);
    CPPUNIT_TEST_SUITE_END ();

    public:
		void setUp (void);
		void tearDown (void);

    protected:

        void  testMeshPreprocessingPos		    (void);
		void  testMeshPreprocessingNeg		    (void);
		void  testAnimationPreprocessingPos     (void);

	private:

		void CheckIfOnly(aiMesh* p,unsigned int num, unsigned flag);

		ScenePreprocessor* pp;
		aiScene* scene;
};

#endif