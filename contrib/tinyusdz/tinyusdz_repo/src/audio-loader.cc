#include "nonstd/expected.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

//#include "external/fpng.h"
//#include "external/stb_audio.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "audio-loader.hh"

namespace tinyusdz {
namespace audio {

namespace {

}  // namespace

nonstd::expected<audio::AudioResult, std::string> LoadAudioFromMemory(
    const uint8_t *addr, size_t sz, const std::string &uri) {
  audio::AudioResult ret;
  std::string err;

  (void)addr;
  (void)sz;
  (void)uri;

  return nonstd::make_unexpected("TODO: LoadAudioFromMemory.\n");
}

}  // namespace audio
}  // namespace tinyusdz
