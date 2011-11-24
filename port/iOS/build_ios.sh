#!/bin/sh
# build.sh

#######################
# BUILD ASSIMP for iOS and iOS Simulator
#######################

BUILD_DIR="./lib/ios"

IOS_BASE_SDK="5.0"
IOS_DEPLOY_TGT="3.2"

setenv_all()
{
	# Add internal libs
	export CFLAGS="$CFLAGS"
	export CPP="$DEVROOT/usr/bin/llvm-cpp-4.2"
	export CXX="$DEVROOT/usr/bin/llvm-g++-4.2"
	export CXXCPP="$DEVROOT/usr/bin/llvm-cpp-4.2"
	export CC="$DEVROOT/usr/bin/llvm-gcc-4.2"
	export LD=$DEVROOT/usr/bin/ld
	export AR=$DEVROOT/usr/bin/ar
	export AS=$DEVROOT/usr/bin/as
	export NM=$DEVROOT/usr/bin/nm
	export RANLIB=$DEVROOT/usr/bin/ranlib
	export LDFLAGS="-L$SDKROOT/usr/lib/"
	
	export CPPFLAGS=$CFLAGS
	export CXXFLAGS=$CFLAGS
}

setenv_arm6()
{
	unset DEVROOT SDKROOT CFLAGS CC LD CPP CXX AR AS NM CXXCPP RANLIB LDFLAGS CPPFLAGS CXXFLAGS
	export DEVROOT=/Developer/Platforms/iPhoneOS.platform/Developer
	export SDKROOT=$DEVROOT/SDKs/iPhoneOS$IOS_BASE_SDK.sdk
	export CFLAGS="-arch armv6 -pipe -no-cpp-precomp -isysroot $SDKROOT -miphoneos-version-min=$IOS_DEPLOY_TGT -I$SDKROOT/usr/include/"
	setenv_all
	rm CMakeCache.txt
	cmake  -G 'Unix Makefiles' -DCMAKE_TOOLCHAIN_FILE=./port/iOS/IPHONEOS_ARM6_TOOLCHAIN.cmake -DENABLE_BOOST_WORKAROUND=ON -DBUILD_STATIC_LIB=ON
}

setenv_arm7()
{
	unset DEVROOT SDKROOT CFLAGS CC LD CPP CXX AR AS NM CXXCPP RANLIB LDFLAGS CPPFLAGS CXXFLAGS
	export DEVROOT=/Developer/Platforms/iPhoneOS.platform/Developer
	export SDKROOT=$DEVROOT/SDKs/iPhoneOS$IOS_BASE_SDK.sdk
	export CFLAGS="-arch armv7 -pipe -no-cpp-precomp -isysroot $SDKROOT -miphoneos-version-min=$IOS_DEPLOY_TGT -I$SDKROOT/usr/include/"
	setenv_all
	rm CMakeCache.txt
	cmake  -G 'Unix Makefiles' -DCMAKE_TOOLCHAIN_FILE=./port/iOS/IPHONEOS_ARM7_TOOLCHAIN.cmake -DENABLE_BOOST_WORKAROUND=ON -DBUILD_STATIC_LIB=ON
}

setenv_i386()
{
	unset DEVROOT SDKROOT CFLAGS CC LD CPP CXX AR AS NM CXXCPP RANLIB LDFLAGS CPPFLAGS CXXFLAGS
	export DEVROOT=/Developer/Platforms/iPhoneSimulator.platform/Developer
	export SDKROOT=$DEVROOT/SDKs/iPhoneSimulator$IOS_BASE_SDK.sdk
	export CFLAGS="-arch i386 -pipe -no-cpp-precomp -isysroot $SDKROOT -miphoneos-version-min=$IOS_DEPLOY_TGT"
	setenv_all
	rm CMakeCache.txt
	cmake  -G 'Unix Makefiles' -DCMAKE_TOOLCHAIN_FILE=./port/iOS/IPHONEOS_i386_TOOLCHAIN.cmake -DENABLE_BOOST_WORKAROUND=ON -DBUILD_STATIC_LIB=ON
}

create_outdir()
{
	for lib_i386 in `find $BUILD_DIR/i386 -name "lib*\.a"`; do
		lib_arm6=`echo $lib_i386 | sed "s/i386/arm6/g"`
		lib_arm7=`echo $lib_i386 | sed "s/i386/arm7/g"`
		lib=`echo $lib_i386 | sed "s/i386\///g"`
		echo 'Creating fat binary...'
		lipo -arch armv6 $lib_arm6 -arch armv7 $lib_arm7 -arch i386 $lib_i386 -create -output $lib
	done
	echo 'Done! You will find the libaries and fat binary library in /lib/ios'
}
cd ../../

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR/arm6 $BUILD_DIR/arm7 $BUILD_DIR/i386

setenv_arm6
echo 'Building armv6 library'
make clean
make assimp -j 8 -l
cp ./lib/libassimp.a $BUILD_DIR/arm6/

setenv_arm7
echo 'Building armv7 library'
make clean
make assimp -j 8 -l
cp ./lib/libassimp.a $BUILD_DIR/arm7/


setenv_i386
echo 'Building i386 library'
make clean
make assimp -j 8 -l
cp ./lib/libassimp.a $BUILD_DIR/i386/

rm ./lib/libassimp.a

create_outdir


