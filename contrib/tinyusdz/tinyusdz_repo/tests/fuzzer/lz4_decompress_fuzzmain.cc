#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <iostream>

#include "lz4-compression.hh"

static int lz4_decompress_main(const uint8_t *data, size_t size)
{
  if (size < (16+1)) return -1;

  // first 8 byte: uncompress size
  // second 8 byte: compress size
  // 1 byte: # chunks
  // remaining : compressed data
  uint64_t uncompressedSize; // nInts
  uint64_t compressedSize;
  
  memcpy(&uncompressedSize, data, 8); 
  memcpy(&compressedSize, data+8, 8); 

  // FIXME: Currently up to 4GB
  if ((uncompressedSize < 4) || (uncompressedSize > 1024*1024*4)) {
    return 0;
  }

  if ((compressedSize < 4) || (compressedSize > 1024*1024*4)) {
    return 0;
  }

  if (compressedSize > (size - (16+1))) {
    return 0;
  }

  std::vector<char> dst;
  dst.resize(uncompressedSize);

  std::string err;
  size_t n = tinyusdz::LZ4Compression::DecompressFromBuffer(reinterpret_cast<const char *>(data + 8), dst.data(), compressedSize, uncompressedSize, &err);
  (void)n;

  return 0;
}

extern "C"
int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size)
{
    int ret = lz4_decompress_main(data, size);
    return ret;
}
