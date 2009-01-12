
#include "UnitTestPCH.h"

//#include <cppunit/XMLOutputter.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>

#include <math.h>
#include <time.h>

int main (int argc, char* argv[])
{
	// seed the randomizer with the current system time
	time_t t;time(&t);
	srand((unsigned int)t);

	// create a logger
	Assimp::DefaultLogger::create("AssimpLog.txt",Assimp::Logger::VERBOSE,
		Assimp::DLS_DEBUGGER | Assimp::DLS_FILE);

    // Informiert Test-Listener ueber Testresultate
    CPPUNIT_NS :: TestResult testresult;

    // Listener zum Sammeln der Testergebnisse registrieren
    CPPUNIT_NS :: TestResultCollector collectedresults;
    testresult.addListener (&collectedresults);

    // Listener zur Ausgabe der Ergebnisse einzelner Tests
    CPPUNIT_NS :: BriefTestProgressListener progress;
    testresult.addListener (&progress);

    // Test-Suite ueber die Registry im Test-Runner einfuegen
    CPPUNIT_NS :: TestRunner testrunner;
    testrunner.addTest (CPPUNIT_NS :: TestFactoryRegistry :: getRegistry ().makeTest ());
    testrunner.run (testresult);

    // Resultate im Compiler-Format ausgeben
	CPPUNIT_NS :: CompilerOutputter compileroutputter (&collectedresults, std::cerr);
    compileroutputter.write ();

#if 0
	// Resultate im XML-Format ausgeben
	std::ofstream of("output.xml");
	CPPUNIT_NS :: XmlOutputter xml (&collectedresults, of);
    xml.write ();
#endif

	// kill the logger again
	Assimp::DefaultLogger::kill();

    // Rueckmeldung, ob Tests erfolgreich waren
    return collectedresults.wasSuccessful () ? 0 : 1;
}