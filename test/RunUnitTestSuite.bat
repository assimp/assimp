rem Alexander Gessler, 12:30:08

set errorlevel=0
color 4e
cls

@echo off

rem 
SET ARCHEXT=x64
IF %PROCESSOR_ARCHITECTURE% == x86 SET ARCHEXT=win32


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
call RunSingleUnitTestSuite unittest_release_%ARCHEXT% release.txt


echo ======================================================================
echo Config: Release -st (Single-threaded, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unittest_release-st_%ARCHEXT% release-st.txt


echo ======================================================================
echo Config: Release -noboost (NoBoost workaround, implicit -st)
echo ======================================================================
call RunSingleUnitTestSuite unittest_release-noboost_%ARCHEXT% release-st-noboost.txt


echo ======================================================================
echo Config: Release -DLL (Multi-threaded DLL, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unittest_release-dll_%ARCHEXT% release-dll.txt


echo ======================================================================
echo Config: Debug (Multi-threaded, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unittest_debug_%ARCHEXT% debug.txt



echo ======================================================================
echo Config: Debug -st (Single-threaded, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unittest_debug_st_%ARCHEXT% debug-st.txt


echo ======================================================================
echo Config: Debug -noboost (NoBoost workaround, implicit -st)
echo ======================================================================
call RunSingleUnitTestSuite unittest_debug-noboost_%ARCHEXT% debug-st-noboost.txt


echo ======================================================================
echo Config: Debug -DLL (Multi-threaded, using boost)
echo ======================================================================
call RunSingleUnitTestSuite unittest_debug-dll_%ARCHEXT% debug-dll.txt




echo.
echo ----------------------------------------------------------------------

IF FIRSTUTNA==none goto end2
echo One or more test configs are not available.

:end2
IF FIRSTUTFAILURE==none goto end
echo One or more tests failed.

echo ----------------------------------------------------------------------
echo.

:end
pause