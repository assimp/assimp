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
SET FIRSTUTFAILURE=none
SET FIRSTUTNA=none

echo #=====================================================================
echo # Open Asset Import Library - Unittests                               
echo #=====================================================================
echo #                                                                     
echo # Executing the Assimp unit test suite for the following                
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

echo ======================================================================
echo Config: Release (Multi-threaded, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unit_release_%ARCHEXT% release.txt


echo ======================================================================
echo Config: Release -st (Single-threaded, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unit_release-st_%ARCHEXT% release-st.txt


echo ======================================================================
echo Config: Release -noboost (NoBoost workaround, implicit -st)
echo ======================================================================
call RunSingleUnitTestSuite unit_release-noboost-st_%ARCHEXT% release-st-noboost.txt


echo ======================================================================
echo Config: Release -DLL (Multi-threaded DLL, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unit_release-dll_%ARCHEXT% release-dll.txt


echo ======================================================================
echo Config: Debug (Multi-threaded, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unit_debug_%ARCHEXT% debug.txt



echo ======================================================================
echo Config: Debug -st (Single-threaded, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unit_debug-st_%ARCHEXT% debug-st.txt


echo ======================================================================
echo Config: Debug -noboost (NoBoost workaround, implicit -st)
echo ======================================================================
call RunSingleUnitTestSuite unit_debug-noboost-st_%ARCHEXT% debug-st-noboost.txt


echo ======================================================================
echo Config: Debug -DLL (Multi-threaded, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unit_debug-dll_%ARCHEXT% debug-dll.txt




echo.
echo ----------------------------------------------------------------------

IF NOT FIRSTUTNA==none (
   echo One or more test configs are not available.
)

IF NOT FIRSTUTFAILURE==none (
   echo One or more tests failed.
)

echo ----------------------------------------------------------------------
echo.

pause