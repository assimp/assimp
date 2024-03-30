rmdir /s /q build
mkdir build

cmake -G "Visual Studio 16 2019" -A x64 -DTINYUSDZ_WITH_OPENSUBDIV=On -Bbuild -H.
