
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

echo #=====================================================================
echo # Open Asset Import Library - Unit & Regression test suite                         
echo #=====================================================================
echo #                                                                     
echo # Executing the Assimp unit & regression test suites for the                                     
echo # following build configurations.                                                           
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


call RunUnitTestSuite.bat