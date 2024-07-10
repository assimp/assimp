# tinyusdz
"tinyusdz" C++ project provides USD/USDA/UDSC/UDSZ 3D model file format suport

## Automatic repo clone
tinyusdz repo is automatically cloned.  Users who haven't opted-in to USD support
won't be burdened with the extra download volume.

To update te git commit hash pulled down, modify `TINYUSDZ_GIT_TAG` in file
    `code/CMakeLists.txt`

## Notes
Couldn't leverage tinyusdz CMakeLists.txt.  Fell back to compiling source files specified in
"android" example.
