#include <cstdint>
#include <cstdlib>
#include <string>

static int parse_m4a_alac(const uint8_t* data, size_t size) {
  if (size > 1024 * 1024 * 128 * 4) {
    return -1;
  }

  // TODO

  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data,
                                      std::size_t size) {
  return parse_m4a_alac(data, size);
}
