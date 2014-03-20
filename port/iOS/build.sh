#!/bin/bash

#######################
# BUILD ASSIMP for iOS and iOS Simulator
#######################

BUILD_DIR="./lib/iOS"

IOS_SDK_VERSION=7.1
IOS_SDK_TARGET=6.0
#(iPhoneOS iPhoneSimulator) -- determined from arch
IOS_SDK_DEVICE=

XCODE_ROOT_DIR=/Applications/Xcode.app/Contents

BUILD_ARCHS_DEVICE="armv7 armv7s arm64"
BUILD_ARCHS_SIMULATOR="i386 x86_64"
BUILD_ARCHS_ALL=(armv7 armv7s arm64 i386 x86_64)

CPP_DEV_TARGET_LIST=(miphoneos-version-min mios-simulator-version-min)
CPP_DEV_TARGET=
CPP_STD_LIB_LIST=(libc++ libstdc++)
CPP_STD_LIB=

function join { local IFS="$1"; shift; echo "$*"; }

build_arch()
{
    IOS_SDK_DEVICE=iPhoneOS
    CPP_DEV_TARGET=${CPP_DEV_TARGET_LIST[0]}

    if [[ "$BUILD_ARCHS_SIMULATOR" =~ "$1" ]]
    then
        echo '[!] Target SDK set to SIMULATOR.'
        IOS_SDK_DEVICE=iPhoneSimulator
        CPP_DEV_TARGET=${CPP_DEV_TARGET_LIST[1]}
    else
        echo '[!] Target SDK set to DEVICE.'
    fi

    unset DEVROOT SDKROOT CFLAGS CC LD CPP CXX AR AS NM CXXCPP RANLIB LDFLAGS CPPFLAGS CXXFLAGS

    export TOOLCHAIN=$XCODE_ROOT_DIR//Developer/Toolchains/XcodeDefault.xctoolchain
    export DEVROOT=$XCODE_ROOT_DIR/Developer/Platforms/$IOS_SDK_DEVICE.platform/Developer
    export SDKROOT=$DEVROOT/SDKs/$IOS_SDK_DEVICE$IOS_SDK_VERSION.sdk
    export CFLAGS="-arch $1 -pipe -no-cpp-precomp -stdlib=$CPP_STD_LIB -isysroot $SDKROOT -$CPP_DEV_TARGET=$IOS_SDK_TARGET -I$SDKROOT/usr/include/"
    export CPP="$TOOLCHAIN/usr/bin/clang++"
    export CXX="$TOOLCHAIN/usr/bin/clang++"
    export CXXCPP="$TOOLCHAIN/usr/bin/clang++"
    export CC="$TOOLCHAIN/usr/bin/clang"
    export LD=$TOOLCHAIN/usr/bin/ld
    export AR=$TOOLCHAIN/usr/bin/ar
    export AS=$TOOLCHAIN/usr/bin/as
    export NM=$TOOLCHAIN/usr/bin/nm
    export RANLIB=$TOOLCHAIN/usr/bin/ranlib
    export LDFLAGS="-L$SDKROOT/usr/lib/"
    export CPPFLAGS=$CFLAGS
    export CXXFLAGS=$CFLAGS

    rm CMakeCache.txt

    cmake  -G 'Unix Makefiles' -DCMAKE_TOOLCHAIN_FILE=./port/iOS/IPHONEOS_$(echo $1 | tr '[:lower:]' '[:upper:]')_TOOLCHAIN.cmake -DENABLE_BOOST_WORKAROUND=ON -DASSIMP_BUILD_STATIC_LIB=ON

    echo "[!] Building $1 library"

    $XCODE_ROOT_DIR/Developer/usr/bin/make clean
    $XCODE_ROOT_DIR/Developer/usr/bin/make assimp -j 8 -l

    echo "[!] Moving built library into: $BUILD_DIR/$1/"

    mv ./lib/libassimp.a $BUILD_DIR/$1/
}

echo "[!] $0 - assimp iOS build script"

CPP_STD_LIB=${CPP_STD_LIB_LIST[0]}
DEPLOY_ARCHS=${BUILD_ARCHS_ALL[*]}
DEPLOY_FAT=1

for i in "$@"; do
    case $i in
    -l=*|--stdlib=*)
        CPP_STD_LIB=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        echo "[!] Selecting c++ std lib: $DEPLOY_STD"
    ;;
    -a=*|--archs=*)
        DEPLOY_ARCHS=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        echo "[!] Selecting architectures: $DEPLOY_ARCHS"
    ;;
    -n|--no-fat)
        DEPLOY_FAT=0
        echo "[!] Fat binary will not be created."
    ;;
    -h|--help)
        echo " - supported architectures: $(echo $(join , ${BUILD_ARCHS_ALL[*]}) | sed 's/,/, /g')"
        echo " - supported C++ STD libs.: $(echo $(join , ${CPP_STD_LIB_LIST[*]}) | sed 's/,/, /g')"
        exit
    ;;
    *)
    ;;
    esac
done

cd ../../
rm -rf $BUILD_DIR

for ARCH_TARGET in $DEPLOY_ARCHS; do
    mkdir -p $BUILD_DIR/$ARCH_TARGET
    build_arch $ARCH_TARGET
    #rm ./lib/libassimp.a
done

if [[ "$DEPLOY_FAT" -eq 1 ]]; then
    echo '  creating fat binary ...'
    for ARCH_TARGET in $DEPLOY_ARCHS; do
        LIPO_ARGS="$LIPO_ARGS-arch $ARCH_TARGET $BUILD_DIR/$ARCH_TARGET/libassimp.a "
    done
    LIPO_ARGS="$LIPO_ARGS-create -output $BUILD_DIR/libassimp-fat.a"
    lipo $LIPO_ARGS
    echo "Done! You will find the libaries and fat binary library in $BUILD_DIR"
fi


