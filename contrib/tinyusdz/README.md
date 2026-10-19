# tinyusdz (rebranded as "lightusd" late 2026)
"tinyusdz" C++ project provides USD/USDA/UDSC/UDSZ 3D model file format suport

## Suppress warnings
At the risk of stating the obvious, if treating warnings-as-errors as per 
best practices, depending on local environment it may be necessary to add 
exceptions to allow build to complete successfully.

Known problems include
- `src/external/tinyvdb/tinyvdb_io.c`: `unused-variable`

To fix, add flags to `CMakeLists.txt` e.g.:
```
set(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} -Wno-error=unused-variable")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MY_CMAKE_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_CMAKE_FLAGS}")
```

After a few attempts should be able to surgically add only the exceptions
required to allow successful build, keeping errors-as-warnings policy in
place...

## Automatic repo clone
tinyusdz repo is automatically cloned.  Users who haven't opted-in to USD support
won't be burdened with the extra download volume.

To update the git commit hash pulled down, modify `TINYUSDZ_GIT_TAG` in file
    `code/CMakeLists.txt`

## Notes
Couldn't leverage tinyusdz CMakeLists.txt.  Fell back to compiling source files specified in
"android" example.
