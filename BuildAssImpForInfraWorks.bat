@echo off
Rem Builds required AssImp dlls for InfraWorks
Rem
Rem  VisualStudio-Version: VS2012
Rem  Solution-Configs:     debug-noboost-dll, release-noboost-dll
Rem  Project:              assimp (assimpview etc. are not required)

set solutionFile=workspaces\vc11\assimp.sln

call MSBuild.exe %solutionFile% /property:Configuration=debug-noboost-dll /property:Platform=x64 /target:assimp
if errorlevel 1 (
	exit /B errorlevel
)

call MSBuild.exe %solutionFile% /property:Configuration=release-noboost-dll /property:Platform=x64 /target:assimp
if errorlevel 1 (
	exit /B errorlevel
)