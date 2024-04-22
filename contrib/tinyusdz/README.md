# tinyusdz

## MODIFICATIONS REQUIRED

### Need to patch `contrib/tinyusdz/tinyusdz_repo/src/external/stb_image_resize2.h` to allow build on
armeabi-v7a via android NDK

Add `#elif` block as indicated below around line `2407`
```
#elif defined(STBIR_WASM) || (defined(STBIR_NEON) && defined(_MSC_VER) && defined(_M_ARM)) // WASM or 32-bit ARM on MSVC/clang
...
#elif defined(STBIR_NEON) && (defined(__ANDROID__) && defined(__arm__)) // 32-bit ARM on android NDK

  static stbir__inline void stbir__half_to_float_SIMD(float * output, stbir__FP16 const * input)
  {
    // TODO: this stub is just to allow build on armeabi-v7a via android NDK
  }

  static stbir__inline void stbir__float_to_half_SIMD(stbir__FP16 * output, float const * input)
  {
    // TODO: this stub is just to allow build on armeabi-v7a via android NDK
  }

  static stbir__inline float stbir__half_to_float( stbir__FP16 h )
  {
    // TODO: this stub is just to allow build on armeabi-v7a via android NDK
    return 0;
  }

  static stbir__inline stbir__FP16 stbir__float_to_half( float f )
  {
    // TODO: this stub is just to allow build on armeabi-v7a via android NDK
    return 0;
  }

#elif defined(STBIR_NEON) && defined(_MSC_VER) && defined(_M_ARM64) && !defined(__clang__) // 64-bit ARM on MSVC (not clang)
```

## Notes
Couldn't leverage tinyusdz CMakeLists.txt.  Fell back to compiling source files specified in
"android" example.

## Assimp update history
### Apr 2024
Updated to [tinyusdz](https://github.com/syoyo/tinyusdz) branch `rendermesh-refactor` at 18 Mar 2024 commit `f9792ce67c4ef08d779cdf91f49ad97acc426466 `

### Mar 2024
Cloned github project [tinyusdz](https://github.com/syoyo/tinyusdz) branch `dev` at 10 Mar 2024 commit `912d27e8b632d2352e7284feb86584832c6015d5`

Removed folders:
- [.clusterfuzzlite](tinyusdz_repo%2F.clusterfuzzlite)
- [.git](tinyusdz_repo%2F.git)
- [.github](tinyusdz_repo%2F.github)
- [android](tinyusdz_repo%2Fandroid)
- [benchmarks](tinyusdz_repo%2Fbenchmarks)
- [cmake](tinyusdz_repo%2Fcmake)
- [data](tinyusdz_repo%2Fdata)
- [doc](tinyusdz_repo%2Fdoc)
- [examples](tinyusdz_repo%2Fexamples)
- [models](tinyusdz_repo%2Fmodels)
- [python](tinyusdz_repo%2Fpython)
- [sandbox](tinyusdz_repo%2Fsandbox)
- [schema](tinyusdz_repo%2Fschema)
- [scripts](tinyusdz_repo%2Fscripts)
- [tests](tinyusdz_repo%2Ftests)

Removed folders in `src`:
- [attic](tinyusdz_repo%2Fsrc%2Fattic)
- [blender](tinyusdz_repo%2Fsrc%2Fblender)
- [osd](tinyusdz_repo%2Fsrc%2Fosd)

Removed unused `.cc` files in `src`, `external` etc

Removed all files at root level except
- [LICENSE](tinyusdz_repo%2FLICENSE)
- [README.md](tinyusdz_repo%2FREADME.md)
