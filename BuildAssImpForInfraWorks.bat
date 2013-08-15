@echo off
Rem Builds required AssImp dlls for InfraWorks
Rem
Rem  VisualStudio-Version: VS2012
Rem  Solution-Configs:     debug-noboost-dll, release-noboost-dll
Rem  Project:              assimp (assimpview etc. are not required)

Rem Find VisualStudio 2012 devenv file
Set DevEnvDir=%VS110COMNTOOLS%\..\IDE

if not exist "%DevEnvDir%\devenv.com" GOTO :ERROR_DONE

"%DevEnvDir%\devenv.com" workspaces\vc11\assimp.sln /build "debug-noboost-dll|x64" /project assimp
"%DevEnvDir%\devenv.com" workspaces\vc11\assimp.sln /build "release-noboost-dll|x64" /project assimp

GOTO :EOF

:ERROR_DONE

echo Failed to build AssImp library, because devenv.com could not be located.
echo Make sure that the VS110COMNTOOLS env variable is properly set e.g. to 
echo C:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\Tools\
echo This should usually be set automatically when installing VisualStudio 2012