@echo off

:: Now we declare a scope
Setlocal EnableDelayedExpansion EnableExtensions

if not defined Configuration set Configuration=2015

if "%Configuration%"=="MinGW" ( goto :mingw )

set arch=x86

if "%platform%" EQU "x64" ( set arch=x86_amd64 )

if "%Configuration%"=="2015" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
)

if "%Configuration%"=="2013" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
)

if "%Configuration%"=="2012" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
)

if "%Configuration%"=="2010" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
)

if "%Configuration%"=="2008" (
	set SET_VS_ENV="C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
)

:: Visual Studio detected
endlocal & call %SET_VS_ENV% %arch%
goto :eof

:: MinGW detected
:mingw
endlocal & set PATH=c:\mingw\bin;%PATH%
