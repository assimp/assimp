#!/bin/bash

#
# Written and maintained by the.arul@gmail.com (2014)
#

BUILD_DIR="./lib/iOS"

###################################
# 		 SDK Version
###################################
IOS_SDK_VERSION=$(xcodebuild -version -sdk iphoneos | grep SDKVersion | cut -f2 -d ':' | tr -d '[[:space:]]')
###################################

###################################
# 		 BUILD Configuration
###################################

BUILD_SHARED_LIBS=OFF
BUILD_TYPE=Release

################################################
# 		 Minimum iOS deployment target version
################################################
MIN_IOS_VERSION="10.0"

IOS_SDK_TARGET=$MIN_IOS_VERSION
XCODE_ROOT_DIR=$(xcode-select  --print-path)
TOOLCHAIN=$XCODE_ROOT_DIR/Toolchains/XcodeDefault.xctoolchain

CMAKE_C_COMPILER=$(xcrun -find cc)
CMAKE_CXX_COMPILER=$(xcrun -find c++)

BUILD_ARCHS_DEVICE="arm64e arm64"
BUILD_ARCHS_SIMULATOR="arm64-simulator x86_64-simulator"

CPP_DEV_TARGET_LIST=(miphoneos-version-min mios-simulator-version-min)
CPP_DEV_TARGET=
CPP_STD_LIB_LIST=(libc++ libstdc++)
CPP_STD_LIB=
CPP_STD_LIST=(c++11 c++14)
CPP_STD=c++11

function join { local IFS="$1"; shift; echo "$*"; }

build_arch()
{
    ARCH=$1
    if [[ "$ARCH" == *"-simulator" ]]; then
        echo '[!] Target SDK set to SIMULATOR.'
        IOS_SDK_DEVICE="iphonesimulator" # Use lowercase matching xcrun naming
        BUILD_ARCH="${ARCH%-simulator}"  # Remove "-simulator" from architecture name
        OUTPUT_FOLDER="$BUILD_DIR/ios-$ARCH"
        MIN_VERSION_FLAG="-mios-simulator-version-min=$IOS_SDK_TARGET"
    else
        echo '[!] Target SDK set to DEVICE.'
        IOS_SDK_DEVICE="iphoneos" # For device builds
        BUILD_ARCH="$ARCH"
        OUTPUT_FOLDER="$BUILD_DIR/ios-$ARCH"
        MIN_VERSION_FLAG="-miphoneos-version-min=$IOS_SDK_TARGET"
    fi

    unset DEVROOT SDKROOT CFLAGS LDFLAGS CPPFLAGS CXXFLAGS CMAKE_CLI_INPUT

    # Use xcrun with the correct SDK to find clang
    export CC="$(xcrun -sdk $IOS_SDK_DEVICE -find clang)"
    export CPP="$CC -E"

    # Derive correct platform directory names
    # Note: iPhoneOS.platform and iPhoneSimulator.platform are used by Xcode internally.
    if [[ "$IOS_SDK_DEVICE" == "iphonesimulator" ]]; then
        PLATFORM_NAME="iPhoneSimulator"
    else
        PLATFORM_NAME="iPhoneOS"
    fi

    export DEVROOT="$XCODE_ROOT_DIR/Platforms/$PLATFORM_NAME.platform/Developer"
    export SDKROOT="$DEVROOT/SDKs/$PLATFORM_NAME$IOS_SDK_VERSION.sdk"

    # Set flags. For simulator builds, we use -mios-simulator-version-min; for device, -miphoneos-version-min.
    export CFLAGS="-arch $BUILD_ARCH -pipe -no-cpp-precomp -isysroot $SDKROOT -I$SDKROOT/usr/include/ $MIN_VERSION_FLAG"
    if [[ "$BUILD_TYPE" =~ "Debug" ]]; then
        export CFLAGS="$CFLAGS -Og"
    else
        export CFLAGS="$CFLAGS -O3"
    fi
    export LDFLAGS="-arch $BUILD_ARCH -isysroot $SDKROOT -L$SDKROOT/usr/lib/"
    export CPPFLAGS="$CFLAGS"
    export CXXFLAGS="$CFLAGS -std=$CPP_STD"

    rm -f CMakeCache.txt

    # Construct the CMake toolchain file path
    # Make sure these toolchain files differentiate between device and simulator builds properly.
    TOOLCHAIN_FILE="./port/iOS/${PLATFORM_NAME}_$(echo "$BUILD_ARCH" | tr '[:lower:]' '[:upper:]')_TOOLCHAIN.cmake"

    CMAKE_CLI_INPUT="-DCMAKE_C_COMPILER=$CMAKE_C_COMPILER -DCMAKE_CXX_COMPILER=$CMAKE_CXX_COMPILER \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_SHARED_LIBS=$BUILD_SHARED_LIBS \
    -DASSIMP_BUILD_ZLIB=ON"

    echo "[!] Running CMake with -G 'Unix Makefiles' $CMAKE_CLI_INPUT"
    cmake -G 'Unix Makefiles' ${CMAKE_CLI_INPUT}

    echo "[!] Building $ARCH library"
    xcrun -run make clean
    xcrun -run make assimp -j 8 -l

    mkdir -p $OUTPUT_FOLDER

    if [[ "$BUILD_SHARED_LIBS" =~ "ON" ]]; then
        echo "[!] Moving built dynamic libraries into: $OUTPUT_FOLDER"
        mv ./lib/*.dylib $OUTPUT_FOLDER/
    fi

    echo "[!] Moving built static libraries into: $OUTPUT_FOLDER"
    mv ./lib/*.a $OUTPUT_FOLDER/
}

echo "[!] $0 - assimp iOS build script"

CPP_STD_LIB=${CPP_STD_LIB_LIST[0]}
CPP_STD=${CPP_STD_LIST[0]}
DEPLOY_FAT=1
DEPLOY_XCFramework=1

for i in "$@"; do
    case $i in
    -s=*|--std=*)
        CPP_STD=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        echo "[!] Selecting c++ standard: $CPP_STD"
    ;;
    -l=*|--stdlib=*)
        CPP_STD_LIB=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        echo "[!] Selecting c++ std lib: $CPP_STD_LIB"
    ;;
    -a=*|--archs=*)
        DEPLOY_ARCHS=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        echo "[!] Selecting architectures: $DEPLOY_ARCHS"
    ;;
    --min-version=*)
        MIN_IOS_VERSION=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        IOS_SDK_TARGET=$MIN_IOS_VERSION
        echo "[!] Selecting minimum iOS version: $MIN_IOS_VERSION"
    ;;
    --debug)
    	BUILD_TYPE=Debug        
        echo "[!] Selecting build type: Debug"
    ;;
    --shared-lib)
    	BUILD_SHARED_LIBS=ON        
        echo "[!] Will generate dynamic libraries"
    ;;
    -n|--no-fat)
        DEPLOY_FAT=0
        echo "[!] Fat binary will not be created."
    ;;
    --no-xcframework)
        DEPLOY_XCFramework=0
        echo "[!] XCFramework will not be created."
    ;;
    -h|--help)
        echo " - don't build fat library (--no-fat)."
        echo " - don't build XCFramework (--no-xcframework)."
        echo " - Include debug information and symbols, no compiler optimizations (--debug)."
        echo " - generate dynamic libraries rather than static ones (--shared-lib)."
        echo " - supported architectures (--archs):  $(echo $(join , ${BUILD_ARCHS_ALL[*]}) | sed 's/,/, /g')"
        echo " - minimum iOS version (--min-version): 16.0"
        echo " - supported C++ STD libs (--stdlib): $(echo $(join , ${CPP_STD_LIB_LIST[*]}) | sed 's/,/, /g')"
        echo " - supported C++ standards (--std): $(echo $(join , ${CPP_STD_LIST[*]}) | sed 's/,/, /g')"
        exit
    ;;
    *)
    ;;
    esac
done

cd ../../
rm -rf $BUILD_DIR

for ARCH in $BUILD_ARCHS_DEVICE; do
    echo "Building for DEVICE arch: $ARCH"
    build_arch $ARCH
done

for ARCH in $BUILD_ARCHS_SIMULATOR; do
    echo "Building for SIMULATOR arch: $ARCH"
    build_arch $ARCH
done

make_fat_static_or_shared_binary()
{
    LIB_NAME=$1
    LIPO_ARGS=''
    for ARCH_TARGET in $DEPLOY_ARCHS; do
        if [[ "$BUILD_SHARED_LIBS" =~ "ON" ]]; then
            LIPO_ARGS="$LIPO_ARGS-arch $ARCH_TARGET $BUILD_DIR/$ARCH_TARGET/$LIB_NAME.dylib "
        else
            LIPO_ARGS="$LIPO_ARGS-arch $ARCH_TARGET $BUILD_DIR/$ARCH_TARGET/$LIB_NAME.a "
        fi
    done
    if [[ "$BUILD_SHARED_LIBS" =~ "ON" ]]; then
    	LIPO_ARGS="$LIPO_ARGS -create -output $BUILD_DIR/$LIB_NAME-fat.dylib"
    else
    	LIPO_ARGS="$LIPO_ARGS -create -output $BUILD_DIR/$LIB_NAME-fat.a"
    fi
    lipo $LIPO_ARGS
}

make_fat_static_binary()
{
    LIB_NAME=$1
    LIPO_ARGS=''
    for ARCH_TARGET in $DEPLOY_ARCHS; do
        LIPO_ARGS="$LIPO_ARGS-arch $ARCH_TARGET $BUILD_DIR/$ARCH_TARGET/$LIB_NAME.a "
    done
    LIPO_ARGS="$LIPO_ARGS -create -output $BUILD_DIR/$LIB_NAME-fat.a"
    lipo $LIPO_ARGS
}

if [[ "$DEPLOY_FAT" -eq 1 ]]; then
    echo '[+] Creating fat binaries ...'
    
    if [[ "$BUILD_TYPE" =~ "Debug" ]]; then
    	make_fat_static_or_shared_binary 'libassimpd'
	else
		make_fat_static_or_shared_binary 'libassimp'
	fi
    
    echo "[!] Done! The fat binaries can be found at $BUILD_DIR"
fi

make_xcframework() {
    LIB_NAME=$1
    FRAMEWORK_PATH="$BUILD_DIR/$LIB_NAME.xcframework"

    # Paths to device and simulator libraries
    DEVICE_LIB_PATH="$BUILD_DIR/ios-arm64/libassimp.a"
    ARM64_SIM_LIB_PATH="$BUILD_DIR/ios-arm64-simulator/libassimp.a"
    X86_64_SIM_LIB_PATH="$BUILD_DIR/ios-x86_64-simulator/libassimp.a"
    UNIVERSAL_SIM_LIB_PATH="$BUILD_DIR/ios-simulator/libassimp.a"

    # Ensure we have a clean location for the universal simulator lib
    mkdir -p "$BUILD_DIR/ios-simulator"

    # Combine simulator libraries if both arm64 and x86_64 simulator slices are present
    if [[ -f "$ARM64_SIM_LIB_PATH" && -f "$X86_64_SIM_LIB_PATH" ]]; then
        echo "[+] Combining arm64 and x86_64 simulator libs into a universal simulator library..."
        lipo -create "$ARM64_SIM_LIB_PATH" "$X86_64_SIM_LIB_PATH" -output "$UNIVERSAL_SIM_LIB_PATH" || {
            echo "[ERROR] lipo failed to combine simulator libraries."
            exit 1
        }
        SIM_LIB_PATH="$UNIVERSAL_SIM_LIB_PATH"
    elif [[ -f "$ARM64_SIM_LIB_PATH" ]]; then
        echo "[!] Only arm64 simulator library found. Using it as is."
        SIM_LIB_PATH="$ARM64_SIM_LIB_PATH"
    elif [[ -f "$X86_64_SIM_LIB_PATH" ]]; then
        echo "[!] Only x86_64 simulator library found. Using it as is."
        SIM_LIB_PATH="$X86_64_SIM_LIB_PATH"
    else
        SIM_LIB_PATH=""
    fi

    ARGS=""

    # Device library
    if [[ -f "$DEVICE_LIB_PATH" ]]; then
        echo "[DEBUG] Adding library $DEVICE_LIB_PATH for device arm64"
        ARGS="$ARGS -library $DEVICE_LIB_PATH -headers ./include"
    else
        echo "[WARNING] Device library not found: $DEVICE_LIB_PATH"
    fi

    # Simulator library (could be universal or a single-arch one)
    if [[ -n "$SIM_LIB_PATH" && -f "$SIM_LIB_PATH" ]]; then
        echo "[DEBUG] Adding library $SIM_LIB_PATH for simulator"
        ARGS="$ARGS -library $SIM_LIB_PATH -headers ./include"
    fi

    if [[ -z "$ARGS" ]]; then
        echo "[ERROR] No valid libraries found to create XCFramework."
        exit 1
    fi

    # Create XCFramework
    echo "[+] Creating XCFramework ..."
    xcodebuild -create-xcframework $ARGS -output $FRAMEWORK_PATH

    echo "[!] Done! The XCFramework can be found at $FRAMEWORK_PATH"
}

if [[ "$DEPLOY_XCFramework" -eq 1 ]]; then
    echo '[+] Creating XCFramework ...'

    if [[ "$BUILD_TYPE" =~ "Debug" ]]; then
        make_xcframework 'libassimpd'
    else
        make_xcframework 'libassimp'
    fi

    echo "[!] Done! The XCFramework can be found at $BUILD_DIR"
fi