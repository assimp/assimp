rem @echo off
setlocal
call build_env_win32.bat

set BUILD_CONFIG=release
set PLATFORM_CONFIG=x64
set MAX_CPU_CONFIG=4

set CONFIG_PARAMETER=/p:Configuration="%BUILD_CONFIG%"
set PLATFORM_PARAMETER=/p:Platform="%PLATFORM_CONFIG%"
set CPU_PARAMETER=/maxcpucount:%MAX_CPU_CONFIG%
set PLATFORM_TOOLSET=/p:PlatformToolset=%PLATFORM_VER%

pushd ..\..\
cmake CMakeLists.txt -G "Visual Studio 15 2017 Win64"
%MSBUILD% assimp.sln %CONFIG_PARAMETER% %PLATFORM_PARAMETER% %CPU_PARAMETER% %PLATFORM_TOOLSET%
popd
endlocal
