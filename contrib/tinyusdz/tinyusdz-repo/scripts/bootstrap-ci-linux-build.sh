curdir=`pwd`

builddir=${curdir}/build

rm -rf ${builddir}
mkdir ${builddir}

# Use local installation of Python
# (Pleae build it using ci-build-python-lib.sh)

#  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
cd ${builddir} && CXX=clang++ CC=clang cmake \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  -DTINYUSDZ_WITH_OPENSUBDIV=1 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DTINYUSDZ_WITH_EXR=1 \
  -DTINYUSDZ_PREFER_LOCAL_PYTHON_INSTALLATION=1 \
  -DPython3_EXECUTABLE=${curdir}/ci/dist/python/bin/python \
  -DTINYUSDZ_WITH_PYTHON=1 \
  -DTINYUSDZ_BUILD_BENCHMARKS=1 \
  -DSANITIZE_ADDRESS=0 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  ..

