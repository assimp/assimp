Build Asset Importer Lib for Android
====================================
This module provides a facade for the io-stream-access to files behind the android-asset-management within 
an Android-native application.
- It is built as a static library
- It requires Android NDK with android API > 9 support.

### Building ###
To use this module please provide following cmake defines:
```
-DASSIMP_ANDROID_JNIIOSYSTEM=ON
-DCMAKE_TOOLCHAIN_FILE=$SOME_PATH/android.toolchain.cmake
```

"SOME_PATH" is a path to your cmake android toolchain script.


The build script for this port is based on [android-cmake](https://github.com/taka-no-me/android-cmake).  
See its documentation for more Android-specific cmake options (e.g. -DANDROID_ABI for the target ABI).
Check [Asset-Importer-Docs](https://assimp-docs.readthedocs.io/en/latest/) for more information.

### Code ###
A small example how to wrap assimp for Android:
```cpp
#include <assimp/port/AndroidJNI/AndroidJNIIOSystem.h>

Assimp::Importer* importer = new Assimp::Importer();
Assimp::AndroidJNIIOSystem *ioSystem = new Assimp::AndroidJNIIOSystem(app->activity);
if ( nullptr != iosSystem ) {
  importer->SetIOHandler(ioSystem);
}  
```
