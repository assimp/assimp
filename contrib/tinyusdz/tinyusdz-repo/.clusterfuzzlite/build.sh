#!/bin/bash -eu

# build project
# e.g.
# ./autogen.sh
# ./configure
# make -j$(nproc) all

# build fuzzers
# e.g.
# $CXX $CXXFLAGS -std=c++11 -Iinclude \
#     /path/to/name_of_fuzzer.cc -o $OUT/name_of_fuzzer \
#     $LIB_FUZZING_ENGINE /path/to/library.a


cd tests/fuzzer

rm -rf build
CXX=clang++ CC=clang meson -Dprefix=$OUT build
cd build
ninja && cp fuzz_intcoding fuzz_tinyusdz fuzz_usdaparser $OUT/
