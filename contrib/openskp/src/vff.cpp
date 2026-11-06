#include <algorithm>

#include "internal.hpp"

namespace openskp {
bool valid_header(const ByteBuffer& d) {
  return d.size() >= 4 && d[0] == 0xff && d[1] == 0xfe && d[2] == 0xff && d[3] == 0x0e;
}

std::string extract_version(const ByteBuffer& d) {
  std::size_t marker = std::string::npos;
  for (std::size_t i = 4; i + 2 < d.size() && i < 512; ++i)
    if (d[i] == 0xff && d[i + 1] == 0xfe && d[i + 2] == 0xff) {
      marker = i;
      break;
    }
  if (marker == std::string::npos) return "unknown";
  std::string s;
  for (std::size_t i = marker + 4; i + 1 < d.size() && i < 512; i += 2)
    if (d[i]) s.push_back(char(d[i]));
  auto a = s.find('{'), b = s.find('}', a);
  return a != std::string::npos && b != std::string::npos ? s.substr(a, b - a + 1) : "unknown";
}

namespace {
// meta/meta.dat uses the exact same low-level TLV framing as model.dat
// (2-byte tag + 4-byte little-endian length + payload), but as one flat,
// non-recursive record list wrapped in a single outer record (tag
// 0x6400/"6400"). Tag 0x6D00/"6D00" carries the model's unit-system
// string ("Millimeter" etc.) as plain text. Confirmed byte-for-byte
// against a real fixture - never opened by any parser in this codebase
// before.
constexpr const char* kMetaWrapperTag = "6400";
constexpr const char* kMetaUnitsTag = "6D00";
}  // namespace

// Extract the model's unit-system string from a VFF file's meta/meta.dat
// contents, or nullopt if the expected tags aren't found.
std::optional<std::string> read_meta_units(const ByteBuffer& meta_bytes) {
  auto records = parse_flat(meta_bytes);
  for (auto& [tag, body] : records) {
    if (tag == kMetaWrapperTag) return read_meta_units(body);
  }
  for (auto& [tag, body] : records) {
    if (tag == kMetaUnitsTag) return std::string(body.begin(), body.end());
  }
  return std::nullopt;
}

bool is_legacy(const ByteBuffer& d) {
  if (!valid_header(d)) return false;
  auto v = extract_version(d);
  auto dot = v.find('.');
  if (v.size() < 2 || dot == std::string::npos) return false;
  try {
    return std::stoi(v.substr(1, dot - 1)) <= 20;
  } catch (...) {
    return false;
  }
}
}  // namespace openskp
