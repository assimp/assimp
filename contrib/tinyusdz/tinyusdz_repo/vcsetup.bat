rmdir /s /q build
mkdir build

rem cmake -G "Visual Studio 16 2019" -A x64 -DTINYUSDZ_WITH_OPENSUBDIV=On -Bbuild -H.

cmake -G "Visual Studio 17 2022" -A x64 -DTINYUSDZ_WITH_OPENSUBDIV=On -Bbuild -H.
