curdir=`pwd`

# You can choose your own OpenSubdiv repo path if required.
# osd_path=${curdir}/deps/OpenSubdiv
#
# -Dosd_DIR=${osd_path}

builddir=${curdir}/build

rm -rf ${builddir}
mkdir ${builddir}


cd ${builddir} && cmake \
  -DCMAKE_VERBOSE_MAKEFILE=1 \
  -DTINYUSDZ_WITH_OPENSUBDIV=On \
  ..

