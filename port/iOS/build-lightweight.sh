#!/bin/sh
XCODE_ROOT_DIR=/Applications/Xcode.app/Contents
IOS_SDK_TARGET=10.0
IOS_SDK_DEVICE=0
CPP_DEV_TARGET=0
CPP_STD_LIB=0
CPP_STD=0

srcDir=./
baseOutputDir=./lib
buildType=MinSizeRel

mkdir $baseOutputDir

build_arch()
{
	arch=$1
	archUp=$(echo $arch | tr '[:lower:]' '[:upper:]')
	generator="Unix Makefiles"
	toolchain="../../port/iOS/IPHONEOS_${archUp}_TOOLCHAIN.cmake"

	cd $baseOutputDir
	mkdir $arch
	cd $arch
	rm CMakeCache.txt

    unset DEVROOT SDKROOT CFLAGS LDFLAGS CPPFLAGS CXXFLAGS
    export DEVROOT=$XCODE_ROOT_DIR/Developer/Platforms/$IOS_SDK_DEVICE.platform/Developer
    export SDKROOT=$DEVROOT/SDKs/$IOS_SDK_DEVICE.sdk
    export CFLAGS="-arch $arch -pipe -no-cpp-precomp -stdlib=$CPP_STD_LIB -isysroot $SDKROOT -$CPP_DEV_TARGET=$IOS_SDK_TARGET -I$SDKROOT/usr/include/"

    export LDFLAGS="-L$SDKROOT/usr/lib/"
    export CPPFLAGS=$CFLAGS
    export CXXFLAGS="$CFLAGS -std=$CPP_STD"

    cmake "../../$srcDir" -G "$generator" -DCMAKE_TOOLCHAIN_FILE=$toolchain -DCMAKE_BUILD_TYPE=$buildType -DASSIMP_BUILD_TESTS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_NO_EXPORT=ON -DBUILD_SHARED_LIBS=OFF -DUSE_AES=OFF -DZIP_64=ON -DSKIP_INSTALL_ALL=ON -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=ON

    $XCODE_ROOT_DIR/Developer/usr/bin/make clean
    $XCODE_ROOT_DIR/Developer/usr/bin/make assimp -j 8 -l

	echo "Working directory is: $PWD"
	echo "Built $arch"
	find -f *.a
	cd ../..
}

do_lipo()
{

	echo "Lipo"
	echo "Working directory is: $PWD"
	
	for ARCH_TARGET1 in $DEPLOY_ARCHS; do
	    LIPO_ARGS="$LIPO_ARGS-arch $ARCH_TARGET1 $baseOutputDir/$ARCH_TARGET1/code/libassimp.a "
	done
	output=$baseOutputDir/libassimp$BUILD_TYPE.a
	LIPO_ARGS="$LIPO_ARGS-create -output $output"
	lipo $LIPO_ARGS

	for ARCH_TARGET2 in $DEPLOY_ARCHS; do
	    LIPO_ARGS2="$LIPO_ARGS2-arch $ARCH_TARGET2 $baseOutputDir/$ARCH_TARGET2/contrib/irrXML/libirrxml.a "
	done
	output=$baseOutputDir/libirrxml$BUILD_TYPE.a
	LIPO_ARGS2="$LIPO_ARGS2-create -output $output"
	lipo $LIPO_ARGS2

	#for ARCH_TARGET3 in $DEPLOY_ARCHS; do
	#    LIPO_ARGS3="$LIPO_ARGS3-arch $ARCH_TARGET3 $baseOutputDir/$ARCH_TARGET3/contrib/zlib/zlib/libz.a "
	#done
	#output=$baseOutputDir/libzlib$BUILD_TYPE.a
	#LIPO_ARGS3="$LIPO_ARGS3-create -output $output"
	#lipo $LIPO_ARGS3

	#for ARCH_TARGET4 in $DEPLOY_ARCHS; do
	#    LIPO_ARGS4="$LIPO_ARGS4-arch $ARCH_TARGET4 $baseOutputDir/$ARCH_TARGET4/contrib/minizip/libminizipstatic.a "
	#done
	#output=$baseOutputDir/libminizip$BUILD_TYPE.a
	#LIPO_ARGS4="$LIPO_ARGS4-create -output $output"
	#lipo $LIPO_ARGS4
}

ARCH_RELEASE=(armv7 armv7s arm64)
ARCH_DEBUG=(i386 x86_64) 
DEPLOY_ARCHS=0
BUILD_TYPE=0

for var in "$@"
do
	if [ "$var" == "debug" ]; then
		buildType=debug
    elif [ "$var" == "iphone" ]; then
		IOS_SDK_DEVICE=iPhoneOS
		CPP_DEV_TARGET=miphoneos-version-min
		CPP_STD_LIB=libc++
		CPP_STD=c++11
		build_arch armv7
		build_arch armv7s
		build_arch arm64
		DEPLOY_ARCHS=${ARCH_RELEASE[*]}
		BUILD_TYPE=release
		do_lipo
    elif [ "$var" == "simulator" ]; then
		IOS_SDK_DEVICE=iPhoneSimulator
		CPP_DEV_TARGET=mios-simulator-version-min
		CPP_STD_LIB=libc++
		CPP_STD=c++11
		build_arch i386
		build_arch x86_64
		DEPLOY_ARCHS=${ARCH_DEBUG[*]}
		BUILD_TYPE=debug
		do_lipo
	fi
done
