rm -rf build
mkdir build

cd build

CXX=clang++ CC=clang cmake \
   -DCMAKE_BUILD_TYPE=Debug \
   -DEXAMPLE_USD_INC_DIR="~/work/USD/dist-mindep/include" \
   -DEXAMPLE_USD_LIB_DIR="~/work/USD/dist-mindep/lib" \
   -DEXAMPLE_USD_MONOLITHIC=OFF \
   -DEXAMPLE_LINK_BOOST_PYTHON=Off \
   -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
   ..
