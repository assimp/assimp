curdir=`pwd`

builddir=${curdir}/build-musl

rm -rf ${builddir}
mkdir ${builddir}

# Download musl native toolchain from
# https://musl.cc/
# ans set its path here.
MUSL_NATIVE_DIR=$HOME/local/x86_64-linux-musl-native/bin

cd ${builddir} && cmake \
  -DCMAKE_C_COMPILER=$MUSL_NATIVE_DIR/gcc \
  -DCMAKE_CXX_COMPILER=$MUSL_NATIVE_DIR/g++ \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  ..

