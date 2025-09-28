# Tinyusdz patch files

Pending acceptance of proposed changes upstream, need to resort to patching to keep things moving

## Tinyusdz files needing patches

### `tinyusdz_repo/src/external/stb_image_resize2.h`
Without patch, build will fail for armeabi-v7a ABI via android NDK

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

