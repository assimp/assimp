// SPDX-License-Identifier: Apache 2.0
// Copyright 2022 - 2023, Syoyo Fujita.
// Copyright 2024 - Present, Light Transport Entertainment Inc.
#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace tinyusdz {

// Simple image class.
// No colorspace conversion will be applied when decoding image data
// (e.g. from .jpg, .png).
struct Image {
  enum class PixelFormat {
    UInt, // LDR and HDR image
    Int, // For normal/displacement map
    Float // HDR image
    // TODO
    // Half
  };
   
  std::string uri;  // filename or uri;

  int width{-1};     // -1 = invalid
  int height{-1};    // -1 = invalid
  int channels{-1};  // Image channels. 3=RGB, 4=RGBA. -1 = invalid
  int bpp{-1};       // bits per pixel. 8=LDR, 16,32=HDR
  PixelFormat format{PixelFormat::UInt};
  
  std::vector<uint8_t> data; // Raw data.
};

} // namespace tinyusdz
