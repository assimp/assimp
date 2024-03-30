rm -rf build
mkdir build

WASI_VERSION=16
WASI_VERSION_FULL=${WASI_VERSION}.0

WASI_SDK_PATH=$HOME/local/wasi-sdk-${WASI_VERSION_FULL}

CC=${WASI_SDK_PATH}/bin/clang CXX=${WASI_SDK_PATH}/bin/clang++ cmake -DWASI_SDK_PATH=${WASI_SDK_PATH} -Bbuild -DCMAKE_BUILD_TYPE=MinSizeRel -S.
