@echo off
set "initialdir=%cd%"
goto:main

:exitsucc
cd /d "%initialdir%"
set initialdir=

set MSBUILD_15="C:\Program Files (x86)\Microsoft Visual Studio\2018\Professional\MSBuild\15.0\Bin\msbuild.exe"
set MSBUILD_14="C:\Program Files (x86)\MSBuild\14.0\Bin\msbuild.exe"

if not "%VS150%"=="" set MSBUILD_15="%VS150%\MSBuild\15.0\Bin\msbuild.exe"

if /i %VS_VERSION%==2017 (
	set MS_BUILD_EXE=%MSBUILD_15%
	set PLATFORM_VER=v141
) else (
	set MS_BUILD_EXE=%MSBUILD_14%
	set PLATFORM_VER=v140
)

set MSBUILD=%MS_BUILD_EXE%

exit /b 0

:main
if not defined PLATFORM set "PLATFORM=x64"

::my work here is done?

set PATH_VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\
REM set PATH_STUDIO="C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\"

for /f "usebackq tokens=*" %%i in (`"%PATH_VSWHERE%vswhere" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
  set InstallDir=%%i
)

IF EXIST "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" set VS150=%InstallDir%\

set "CMAKE_GENERATOR=Visual Studio 15 2017 Win64"
if not "%VS150%"=="" call "%VS150%\VC\Auxiliary\Build\vcvarsall.bat" %PLATFORM% && echo found VS 2o17 && set PLATFORM_VER=v141 && set VS_VERSION=2017 && goto:exitsucc

set "CMAKE_GENERATOR=Visual Studio 14 2015 Win64"
if defined VS140COMNTOOLS call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" %PLATFORM% && echo found VS 2o15 && set PLATFORM_VER=v140 && set VS_VERSION=2015 && goto:exitsucc

if defined VS130COMNTOOLS echo call ghostbusters... found lost VS version

set "CMAKE_GENERATOR=Visual Studio 12 2013 Win64"
if defined VS120COMNTOOLS call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" %PLATFORM% && echo found VS 2o13 && set PLATFORM_VER=v120 && set VS_VERSION=2013 && goto:exitsucc

set "CMAKE_GENERATOR=Visual Studio 11 2012 Win64"
if defined VS110COMNTOOLS call "%VS110COMNTOOLS%..\..\VC\vcvarsall.bat" %PLATFORM% && echo found VS 2o12 && set PLATFORM_VER=v110 && set VS_VERSION=2012 && goto:exitsucc

set "CMAKE_GENERATOR=Visual Studio 10 2010 Win64"
if defined VS100COMNTOOLS call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" %PLATFORM% && echo found VS 2o1o && set PLATFORM_VER=v100 && set VS_VERSION=2010 && goto:exitsucc

goto:exitsucc