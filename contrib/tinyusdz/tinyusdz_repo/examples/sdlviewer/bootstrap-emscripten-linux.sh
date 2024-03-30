rm -rf build_emcc
mkdir build_emcc

cd build_emcc

# Assume emsdk env has been setup
emcmake cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
