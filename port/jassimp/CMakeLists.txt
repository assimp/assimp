cmake_minimum_required( VERSION 2.6 )

project (Jassimp)

LINK_DIRECTORIES( ${Assimp_BINARY_DIR} ${Assimp_BINARY_DIR}/lib )

SET( JAVA_SRC_PATH jassimp/src/jassimp )
FILE(GLOB JassimpJAVA_SRCS "${JAVA_SRC_PATH}/*.java")

find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)

# sync with main CMakeList.txt?
enable_testing()

# required?
set(CMAKE_JNI_TARGET TRUE)

## compile *.java to class file
set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.6" "-target" "1.6")
set(CMAKE_JAR_CLASSES_PREFIX jassimp)
# hm this do the stuff on install time?
set(CMAKE_JAVA_TARGET_OUTPUT_NAME jassimp.jar)
add_jar( _jjassimp ${JassimpJAVA_SRCS} )
get_target_property(_jarFile _jjassimp JAR_FILE)
get_target_property(_classDir _jjassimp CLASSDIR)

# i better not touch the header file...
## generate JNIFoo.h stub
#set (_stubDir "${CMAKE_CURRENT_BINARY_DIR}")
#add_custom_command(
#    OUTPUT JNIFoo.h
#    COMMAND ${Java_JAVAH_EXECUTABLE} -verbose
#        -classpath ${_classDir}
#        -d ${_stubDir}
#        -jni JNIFoo
#    )

# generate jnilib
include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2} ${_classDir} ${_stubDir} ${Assimp_SOURCE_DIR}/include)
# next line just if debug?
ADD_DEFINITIONS( -DJNI_LOG )
ADD_LIBRARY( jassimp MODULE jassimp-native/src/jassimp.cpp jassimp-native/src/jassimp.h )
#TARGET_LINK_LIBRARIES(jassimp ${JNI_LIBRARIES})
TARGET_LINK_LIBRARIES(jassimp assimp)
INSTALL( TARGETS jassimp LIBRARY DESTINATION ${ASSIMP_LIB_INSTALL_DIR} )
#install_jar(_jjassimp ${LIB_INSTALL_DIR}/jassimp)
#install_jni_symlink(_jjassimp ${JAVA_LIB_INSTALL_DIR})

## add test to run a sample modelparser
#add_test(NAME TestJNIFoo
#    COMMAND ${Java_JAVA_EXECUTABLE}
#    -Djava.library.path=${CMAKE_CURRENT_BINARY_DIR}
#    -cp ${_jarFile} JNIFoo)

