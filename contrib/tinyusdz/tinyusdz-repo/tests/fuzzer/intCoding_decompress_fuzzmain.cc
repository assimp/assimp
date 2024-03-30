#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <iostream>

#include "lz4-compression.hh"
#include "integerCoding.h"

static void parse_intCoding4(const uint8_t *data, size_t size)
{
  // header
  //
  // 4 bytes : nInts
  // 8 bytes : compressedBytes

  if (size <= 8 + 4) return;

  // TODO: Use Compress() to compute nInts.
  uint32_t n; // nInts
  memcpy(&n, data, 4); 

  // HARD limit to avoid stopping fuzzer with oom. Currently set to 2GB(
  if (n > ((1024ull*1024ull*1024ull*2ull) / sizeof(uint32_t))) {
    return;
  }

  using Compressor = tinyusdz::Usd_IntegerCompression;
  size_t compBufferSize = Compressor::GetCompressedBufferSize(n);


  uint64_t compSize;
  memcpy(&compSize, data + 4, 8); 

  if (compSize < 4) {
    // too small
    return;
  }

  if ((compSize + 8 + 4) > size) {
    return;
  }

  if (compSize > compBufferSize) {
    return;
  }

  if (compBufferSize > tinyusdz::LZ4Compression::GetMaxInputSize()) {
    return;
  }

  // HARD limit to avoid stopping fuzzer with oom. Currently set to 2GB
  if (compSize > (1024ull*1024ull*1024ull*2ull)) {
    return;
  }


  std::vector<char> compBuffer;
  compBuffer.resize(compBufferSize);

  //std::cout << "n = " << n << "\n";
  //std::cout << "compSize = " << compSize << "\n";
  memcpy(compBuffer.data(), data + 8 + 4, compSize);

  std::vector<uint32_t> output(n);

  std::string err;
  bool ret = Compressor::DecompressFromBuffer(compBuffer.data(), compSize, output.data(), n, &err);
  (void)ret;

}

extern "C"
int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size)
{
    parse_intCoding4(data, size);
    return 0;
}
