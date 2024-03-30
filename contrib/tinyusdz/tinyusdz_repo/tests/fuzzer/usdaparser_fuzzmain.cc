#include <cstdint>

#include "tinyusdz.hh"
#include "usda-reader.hh"

static void parse_usda(const uint8_t *data, size_t size)
{
  // append magic header
  std::string content("#usda 1.0\n");

  std::vector<uint8_t> buf;
  buf.resize(content.size() + size);
  memcpy(&buf[0], &content[0], content.size());
  
  memcpy(&buf[content.size()], data, size);

  size_t total_size = content.size() + size;

  tinyusdz::StreamReader sr(buf.data(), total_size, /* endianswap */false);

  tinyusdz::usda::USDAReader reader(&sr);
  
  bool ret = reader.Read(); 
  (void)ret;

  return;
}

extern "C"
int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size)
{
    parse_usda(data, size);
    return 0;
}
