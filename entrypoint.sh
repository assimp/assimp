#!/bin/sh

NAME=assimp-`date +%F`
TARGET=$NAME.tar
PACKAGE=$TARGET.gz

cd /code
mkdir -p build
cd build
rm -f $PACKAGE $TARGET
cmake ..
make -j4
tar cf $TARGET \
    bin/assimp \
    bin/libassimp.so \
    bin/libassimp.so.5 \
    bin/libassimp.so.5.0.1 \
    bin/libzlib.so \
    bin/libzlib.so.1
gzip $TARGET
rm -f $TARGET
