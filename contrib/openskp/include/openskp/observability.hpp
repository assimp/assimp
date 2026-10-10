#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace openskp {

enum class LogLevel { debug, information };
enum class ParseStage { header, zip_extract, tlv_walk, legacy_walk, legacy_defs, build_scene };

constexpr std::string_view stage_name(ParseStage s) noexcept {
  switch (s) {
    case ParseStage::header:
      return "header";
    case ParseStage::zip_extract:
      return "zip_extract";
    case ParseStage::tlv_walk:
      return "tlv_walk";
    case ParseStage::legacy_walk:
      return "legacy_walk";
    case ParseStage::legacy_defs:
      return "legacy_defs";
    case ParseStage::build_scene:
      return "build_scene";
  }
  return {};
}

struct ParseProgress {
  ParseStage stage;
  std::uint64_t current{};
  std::uint64_t total{};
};

struct ParseOptions {
  std::function<void(const ParseProgress&)> progress;
  std::function<void(LogLevel, std::string_view)> log;
};

inline constexpr std::uint64_t progress_interval = 500;
}  // namespace openskp
