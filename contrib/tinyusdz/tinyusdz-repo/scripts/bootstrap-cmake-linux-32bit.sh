curdir=`pwd`

# For Ubuntu, install 32bit libc, libstdc++, etc libraries before the build.
# `gcc-multilib` and `g++-multilib` is the easier solution
# sudo apt install gcc-multilib g++-multilib


builddir=${curdir}/build_m32

rm -rf ${builddir}
mkdir ${builddir}


cd ${builddir} && CC=clang CXX=clang++ cmake \
  -DCMAKE_TOOLCHAIN_FILE=cmake/linux_i386.toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSANITIZE_ADDRESS=1 \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  ..
