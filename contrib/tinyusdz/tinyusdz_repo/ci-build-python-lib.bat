rem Build local Python lib
rem Assume running this batch file at the toplevel directory of TinyUSDZ.

git clone https://github.com/lighttransport/python-cmake-buildsystem ci/python-cmake-buildsystem
cd ci/python-cmake-buildsystem
git pull origin master
cd %~dp0

rmdir /s /q %~dp0/ci/buil_python

rem change to "Visual Studio 17 2022" if you use VS2022
cmake.exe -G "Visual Studio 16 2019" -A x64 ^
   -DCMAKE_INSTALL_PREFIX:PATH=%~dp0/ci/dist/python ^
     -DPYTHON_VERSION="3.10.6" ^
     -DUSE_SYSTEM_TCL=OFF ^
     -DUSE_SYSTEM_ZLIB=OFF ^
     -DUSE_SYSTEM_DB=OFF ^
     -DUSE_SYSTEM_GDBM=OFF ^
     -DUSE_SYSTEM_LZMA=OFF ^
     -DUSE_SYSTEM_READLINE=OFF ^
     -DUSE_SYSTEM_SQLITE3=OFF ^
     -DENABLE_SSL=OFF ^
     -DENABLE_HASHLIB=OFF ^
     -DENABLE_MD5=OFF ^
     -DENABLE_SHA=OFF ^
     -DENABLE_SHA256=OFF ^
     -DENABLE_SHA512=OFF ^
     -B %~dp0/ci/build_python ^
     -S %~dp0/ci/python-cmake-buildsystem && ^
cmake.exe --build %~dp0/ci/build_python --config Release --clean-first -- /m && ^
cmake.exe --build %~dp0/ci/build_python --config Release --target INSTALL
