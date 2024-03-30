curdir=`pwd`

# Set path to OpenSubdiv
#osd_path=${curdir}/deps/OpenSubdiv

builddir=${curdir}/build

rm -rf ${builddir}
mkdir ${builddir}


cd ${builddir} && cmake \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  ..

