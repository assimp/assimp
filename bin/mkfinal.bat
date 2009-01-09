cls
@echo off

mkdir final
if exist assimp_release-dll_win32\Assimp32.dll goto do

echo Failed to find the Assimp DLL
pause
goto end

:do
copy assimp_release-dll_win32\assimp32.dll final\assimp32.dll

copy assimpview_release-dll_win32\assimp_view.exe final\assimp_view.exe
copy assimpdumb_release-dll_win32\assimp_dumb.exe final\assimp_dumb.exe

:end