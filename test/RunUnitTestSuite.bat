rem ------------------------------------------------------------------------------
rem Tiny script to execute Assimp's fully unit test suite for all configurations
rem 
rem Usage: call RunUnitTestSuite
rem ------------------------------------------------------------------------------

rem Setup the console environment
set errorlevel=0
color 4e
cls

@echo off

rem Setup target architecture
SET ARCHEXT=x64
IF %PROCESSOR_ARCHITECTURE% == x86 (
   SET ARCHEXT=win32
)

rem Setup standard paths from here
SET OUTDIR=results\
SET BINDIR=..\bin\
SET FIRSTUTFAILURE=nil
SET FIRSTUTNA=nil

echo #=====================================================================
echo # Open Asset Import Library - Unittests                               
echo #=====================================================================
echo #                                                                     
echo # Executes the Assimp library unit test suite for the following                
echo # build configurations (if available):                                               
echo #                                                                     
echo #  Release                                                           
echo #  Release -st                                                        
echo #  Release -noboost                                                   
echo #  Release -dll                                                       
echo #                                                                     
echo #  Debug                                                              
echo #  Debug   -st                                                        
echo #  Debug   -noboost                                                   
echo #  Debug   -dll                                                                                                                        
echo ======================================================================
echo.
echo.


echo assimp-core release 
echo **********************************************************************
call RunSingleUnitTestSuite unit_release_%ARCHEXT% release.txt

echo assimp-core release -st 
echo **********************************************************************
call RunSingleUnitTestSuite unit_release-st_%ARCHEXT% release-st.txt

echo assimp-core release -noboost 
echo **********************************************************************
call RunSingleUnitTestSuite unit_release-noboost-st_%ARCHEXT% release-st-noboost.txt

echo assimp-core release -dll 
echo **********************************************************************
call RunSingleUnitTestSuite unit_release-dll_%ARCHEXT% release-dll.txt

echo assimp-core debug
echo **********************************************************************
call RunSingleUnitTestSuite unit_debug_%ARCHEXT% debug.txt

echo assimp-core debug -st 
echo **********************************************************************
call RunSingleUnitTestSuite unit_debug-st_%ARCHEXT% debug-st.txt

echo assimp-core debug -noboost 
echo **********************************************************************
call RunSingleUnitTestSuite unit_debug-noboost-st_%ARCHEXT% debug-st-noboost.txt

echo assimp-core debug -dll 
echo **********************************************************************
call RunSingleUnitTestSuite unit_debug-dll_%ARCHEXT% debug-dll.txt


echo ======================================================================
IF %FIRSTUTNA% == nil (
   echo All test configs have been found.
) ELSE (
   echo One or more test configs are not available.
)

IF %FIRSTUTFAILURE% == nil (
   echo All tests have been successful. 
) ELSE (
   echo One or more tests failed.
)
echo.

pause