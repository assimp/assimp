#include <cstdint>

#include "tinyusdz.hh"
#include "usdc-reader.hh"

static void parse_usdc(const uint8_t *data, size_t size)
{
  // append magic header
  std::string content("PXR-USDC");

  std::vector<uint8_t> buf;
  buf.resize(content.size() + size);
  memcpy(&buf[0], &content[0], content.size());
  
  memcpy(&buf[content.size()], data, size);

  size_t total_size = content.size() + size;


  tinyusdz::StreamReader sr(buf.data(), total_size, /* endianswap */false);

  tinyusdz::usdc::USDCReaderConfig config;

  // For fuzzer run
  config.kMaxAllowedMemoryInMB = 1024*4; // 4GB.

  tinyusdz::usdc::USDCReader reader(&sr);
  
  bool ret = reader.ReadUSDC(); 
  (void)ret;

  return;
}

extern "C"
int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size)
{
    parse_usdc(data, size);
    return 0;
}
