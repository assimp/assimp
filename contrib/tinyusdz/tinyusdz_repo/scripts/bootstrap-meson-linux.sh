rm -rf builddir

CXX=clang++ meson builddir -Db_sanitize=address --buildtype debug
