#include <cstdint>

#define WUFFS_IMPLEMENTATION

// functions have static storage.
//
// This can help the compiler ignore or discard unused code, which can produce
// faster compiles and smaller binaries. Other motivations are discussed in the
// "ALLOW STATIC IMPLEMENTATION" section of
// https://raw.githubusercontent.com/nothings/stb/master/docs/stb_howto.txt
#define WUFFS_CONFIG__STATIC_FUNCTIONS

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c choose which parts of Wuffs to build. That file contains the
// entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__JPEG

#include "external/wuffs-unsupported-snapshot.c"

static uint8_t *wuffs_decode_jpeg(const uint8_t *pData, size_t data_len,
                                  uint32_t &width, uint32_t &height,
                                  std::string *err) {
  constexpr uint64_t kMaxDataLen = 1024ull * 1024ull * 1024ull * 2;

  // Up to 64K x 64K image
  //constexpr uint64_t kMaxPixels = 65536ull * 65536ull;

  // Up to 16K x 16K
  constexpr uint64_t kMaxPixels = 16384ull * 16384ull;

  wuffs_jpeg__decoder *pDec = wuffs_jpeg__decoder__alloc();
  if (!pDec) {
    if (err) {
      (*err) = "JPEG decoder allocation failed.\n";
    }

    return nullptr;
  }

  // wuffs_jpeg__decoder__set_quirk_enabled(pDec,
  // WUFFS_BASE__QUIRK_IGNORE_CHECKSUM, true);
  wuffs_jpeg__decoder__set_quirk(pDec, WUFFS_BASE__QUIRK_IGNORE_CHECKSUM, true);

  wuffs_base__image_config ic;
  wuffs_base__io_buffer src =
      wuffs_base__ptr_u8__reader((uint8_t *)pData, data_len, true);
  wuffs_base__status status =
      wuffs_jpeg__decoder__decode_image_config(pDec, &ic, &src);

  if (status.repr) {
    free(pDec);
    if (err) {
      (*err) = "JPEG header decode failed.\n";
    }
    return nullptr;
  }

  width = wuffs_base__pixel_config__width(&ic.pixcfg);
  height = wuffs_base__pixel_config__height(&ic.pixcfg);

  wuffs_base__pixel_config__set(
      &ic.pixcfg, WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL,
      WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width, height);

  uint64_t workbuf_len = wuffs_jpeg__decoder__workbuf_len(pDec).max_incl;
  if (workbuf_len > kMaxDataLen) {
    free(pDec);
    if (err) {
      (*err) = "Seems JPEG image is too big(2GB+).\n";
    }

    return nullptr;
  }

  wuffs_base__slice_u8 workbuf_slice = wuffs_base__make_slice_u8(
      (uint8_t *)malloc((size_t)workbuf_len), (size_t)workbuf_len);
  if (!workbuf_slice.ptr) {
    free(pDec);
    if (err) {
      (*err) = "Failed to allocate slice buffer to decode JPEG.\n";
    }
    return nullptr;
  }

  const uint64_t total_pixels = (uint64_t)width * (uint64_t)height;
  if (total_pixels > kMaxPixels) {
    free(workbuf_slice.ptr);
    free(pDec);
    if (err) {
      (*err) = "Image extent is too large.\n";
    }
    return nullptr;
  }

  void *pDecode_buf = malloc((size_t)(total_pixels * sizeof(uint32_t)));
  if (!pDecode_buf) {
    free(workbuf_slice.ptr);
    free(pDec);

    if (err) {
      (*err) = "Failed to allocate decode buffer.\n";
    }
    return nullptr;
  }

  wuffs_base__slice_u8 pixbuf_slice = wuffs_base__make_slice_u8(
      (uint8_t *)pDecode_buf, (size_t)(total_pixels * sizeof(uint32_t)));

  wuffs_base__pixel_buffer pb;
  status =
      wuffs_base__pixel_buffer__set_from_slice(&pb, &ic.pixcfg, pixbuf_slice);

  if (status.repr) {
    free(workbuf_slice.ptr);
    free(pDecode_buf);
    free(pDec);
    if (err) {
      (*err) = "Failed to setup Pixbuf.\n";
    }
    return nullptr;
  }

  status = wuffs_jpeg__decoder__decode_frame(
      pDec, &pb, &src, WUFFS_BASE__PIXEL_BLEND__SRC, workbuf_slice, NULL);

  if (status.repr) {
    free(workbuf_slice.ptr);
    free(pDecode_buf);
    free(pDec);
    if (err) {
      (*err) = "Failed to decode JPEG frame.\n";
    }
    return nullptr;
  }

  free(workbuf_slice.ptr);
  free(pDec);

  return reinterpret_cast<uint8_t *>(pDecode_buf);
}

static int decode_jpeg(const uint8_t *data, size_t size) {
  // Up to 2GB
  if (uint64_t(size) > 1024ull * 1024ull * 1024ull * 2) {
    return -1;
  }

  uint32_t w, h;
  std::string err;
  uint8_t *img = wuffs_decode_jpeg(data, size, w, h, &err);
  if (!img) {
    return -1;
  }

  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data,
                                      std::size_t size) {
  return decode_jpeg(data, size);
}
