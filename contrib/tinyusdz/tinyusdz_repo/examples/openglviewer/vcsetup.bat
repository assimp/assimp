rem You can use --fresh flag in cmake for cmake 3.24+ without removing build folder to clean-up build folder.
rmdir /s /q build
mkdir build

rem cmake -G "Visual Studio 16 2019" -A x64 -DTINYUSDZ_WITH_OPENSUBDIV=On -Bbuild -S.

cmake -G "Visual Studio 17 2022" -A x64 -Bbuild -S.
