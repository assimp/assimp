curdir=`pwd`

builddir=${curdir}/build

rm -rf ${builddir}
mkdir ${builddir}

# Set path to blender's python
PYTHON_EXE=$HOME/local/blender-2.93.1-linux-x64/2.93/python/bin/python3.9

cd ${builddir} && CXX=clang++ CC=clang cmake \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  -DPYTHON_EXECUTABLE=${PYTHON_EXE} \
  -DTINYUSDZ_WITH_BLENDER_ADDON=1 \
  ..

