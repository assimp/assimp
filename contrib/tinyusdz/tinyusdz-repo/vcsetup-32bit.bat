rmdir /s /q build_win32
mkdir build_win32

rem cmake -G "Visual Studio 16 2019" -A Win32 -DTINYUSDZ_WITH_OPENSUBDIV=On -Bbuild_win32 -S.

cmake -G "Visual Studio 17 2022" -A Win32 -DTINYUSDZ_WITH_OPENSUBDIV=On -Bbuild_win32 -S.
