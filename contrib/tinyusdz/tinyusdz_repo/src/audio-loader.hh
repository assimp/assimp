// Simple audio loader
// supported file format: WAV, MP3  
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "tinyusdz.hh"

#include "nonstd/expected.hpp"

namespace tinyusdz {
namespace audio {

struct AudioResult {
  //Image image;
  std::string warning;
};

///
/// @param[in] filename Input filename(or URI)
/// @return ImageResult or error message(std::string)
///
nonstd::expected<AudioResult, std::string> LoadAudioFromFile(const std::string &filename);

///
/// @param[in] addr Memory address
/// @param[in] datasize Data size(in bytes)
/// @param[in] uri Input URI(or filename) as a hint. This is used only in error message.
/// @return ImageResult or error message(std::string)
///
nonstd::expected<AudioResult, std::string> LoadAudioFromMemory(const uint8_t *addr, const size_t datasize, const std::string &uri);

} // namespace audio
} // namespace tinyusdz
