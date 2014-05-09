@echo off
Rem Builds required AssImp dlls for InfraWorks
Rem
Rem  VisualStudio-Version: VS2013
Rem  Solution-Configs:     debug-noboost-dll, release-noboost-dll
Rem  Project:              assimp (assimpview etc. are not required)

Rem Find VisualStudio 2013 devenv file
Set DevEnvDir=%VS120COMNTOOLS%\..\IDE

if not exist "%DevEnvDir%\devenv.com" GOTO :ERROR_DONE

"%DevEnvDir%\devenv.com" workspaces\vc11\assimp.sln /upgrade
"%DevEnvDir%\devenv.com" workspaces\vc11\assimp.sln /project assimp /build "debug-noboost-dll|x64"
"%DevEnvDir%\devenv.com" workspaces\vc11\assimp.sln /project assimp /build "release-noboost-dll|x64"

GOTO :EOF

:ERROR_DONE

echo Failed to build AssImp library, because devenv.com could not be located.
echo Make sure that the VS120COMNTOOLS env variable is properly set e.g. to 
echo C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\
echo This should usually be set automatically when installing VisualStudio 2013
