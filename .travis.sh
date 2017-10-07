function generate()
{
    OPTIONS="-DASSIMP_WERROR=ON"

    if [ "$DISABLE_EXPORTERS" = "YES" ] ; then
        OPTIONS="$OPTIONS -DASSIMP_NO_EXPORT=YES"
    else
        OPTIONS="$OPTIONS -DASSIMP_NO_EXPORT=NO"
    fi

    if [ "$SHARED_BUILD" = "ON" ] ; then
        OPTIONS="$OPTIONS -DBUILD_SHARED_LIBS=ON"
    else
        OPTIONS="$OPTIONS -DBUILD_SHARED_LIBS=OFF"
    fi

    if [ "$ENABLE_COVERALLS" = "ON" ] ; then
        OPTIONS="$OPTIONS -DASSIMP_COVERALLS=ON"
    else
        OPTIONS="$OPTIONS -DASSIMP_COVERALLS=OFF"
    fi

    if [ "$ASAN" = "ON" ] ; then
        OPTIONS="$OPTIONS -DASSIMP_ASAN=ON"
    else
        OPTIONS="$OPTIONS -DASSIMP_ASAN=OFF"
    fi

    cmake -G "Unix Makefiles" $OPTIONS
}

if [ $ANDROID ]; then
    ant -v -Dmy.dir=${TRAVIS_BUILD_DIR} -f ${TRAVIS_BUILD_DIR}/port/jassimp/build.xml ndk-jni
fi
if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    generate \
    && make -j4 \
    && sudo make install \
    && sudo ldconfig \
    && (cd test/unit; ../../bin/unit) \
    #&& (cd test/regression; chmod 755 run.py; ./run.py ../../bin/assimp; \
	#   chmod 755 result_checker.py; ./result_checker.py)
fi
