#include <cstdint>

#define DR_WAV_IMPLEMENTATION
#include "external/dr_wav.h"

static int parse_wav(const uint8_t* data, size_t size) {
  if (size > 1024 * 1024 * 128 * 4) {
    return -1;
  }

  uint32_t channels;
  uint32_t sampleRate;
  drwav_uint64 totalFrameCount; 
  
  float* psampledata = drwav_open_memory_and_read_pcm_frames_f32(
    data, size, &channels, &sampleRate, &totalFrameCount, /* alloc callbacks */nullptr);

  if (!data) {
    // err
    return 0;
  }

  drwav_free(psampledata, /* alloc callbacks */nullptr );

  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data,
                                      std::size_t size) {
  return parse_wav(data, size);
}
