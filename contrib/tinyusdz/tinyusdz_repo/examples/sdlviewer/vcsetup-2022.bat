rmdir /s /q build
mkdir build

cmake -G "Visual Studio 17 2022" -A x64 -DTINYUSDZ_WITH_OPENSUBDIV=On -Bbuild -H.
