This is an AndroidJNI module to ease assets extraction from android asset manager.

To use this module please provide floowing cmake defines:

-DASSIMP_JNIIOSYSTEM_ANDROID=ON
-DCMAKE_TOOLCHAIN_FILE=$SOME_PATH/android.toolchain.cmake

"SOME_PATH" is a path to your cmake android toolchain script

