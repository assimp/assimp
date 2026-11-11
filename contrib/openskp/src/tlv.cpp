#include <algorithm>
#include <iomanip>
#include <sstream>

#include "internal.hpp"

namespace openskp {
static void require_range(const ByteBuffer& d, std::size_t o, std::size_t n) {
  if (o > d.size() || n > d.size() - o) throw std::out_of_range("binary read exceeds buffer");
}

std::uint16_t read_u16(const ByteBuffer& d, std::size_t o) {
  require_range(d, o, 2);
  return std::uint16_t(d[o]) | std::uint16_t(d[o + 1]) << 8;
}

std::uint32_t read_u32(const ByteBuffer& d, std::size_t o) {
  require_range(d, o, 4);
  return std::uint32_t(d[o]) | std::uint32_t(d[o + 1]) << 8 | std::uint32_t(d[o + 2]) << 16 |
         std::uint32_t(d[o + 3]) << 24;
}

std::int32_t read_i32(const ByteBuffer& d, std::size_t o) {
  return static_cast<std::int32_t>(read_u32(d, o));
}

double read_f64(const ByteBuffer& d, std::size_t o) {
  require_range(d, o, 8);
  std::uint64_t bits = 0;
  for (int i = 0; i < 8; ++i) bits |= std::uint64_t(d[o + i]) << (8 * i);
  double v;
  std::memcpy(&v, &bits, 8);
  return v;
}

std::uint64_t parse_varint(const ByteBuffer& d, std::size_t o, std::size_t n) {
  require_range(d, o, n);
  if (n > 8) throw std::overflow_error("varint exceeds 64 bits");
  std::uint64_t v = 0;
  for (std::size_t i = 0; i < n; ++i) v |= std::uint64_t(d[o + i]) << (8 * i);
  return v;
}

std::string tag_at(const ByteBuffer& d, std::size_t o) {
  require_range(d, o, 2);
  static const char h[] = "0123456789ABCDEF";
  std::string s(4, '0');
  s[0] = h[d[o] >> 4];
  s[1] = h[d[o] & 15];
  s[2] = h[d[o + 1] >> 4];
  s[3] = h[d[o + 1] & 15];
  return s;
}

// VFF (2021+) model.dat binary structure uses Type-Length-Value (TLV) records.
// Most TLV tags carry raw leaf binary/string payloads, but container tags contain
// nested sub-TLV nodes (e.g. definitions, drawing elements, component instances).
// containers lists every tag hex ID that the TLV parser must recursively traverse.
static const std::set<std::string> containers = {
    "F401", "F701", "D430", "D530", "C832", "7C15", "8813", "8913", "8A13", "8B13", "8C13", "8D13",
    "4C1D", "6419", "F901", "7017", "7117", "D007", "C409", "9411", "9511", "0F01", "384A", "B80B",
    "9713", "2C4C", "AC0D", "AE0D", "F601", "F801", "983A", "993A", "8C3C", "8D3C", "9013", "401F"};

std::vector<TlvNode> parse_tlv_recursive(const ByteBuffer& d, std::size_t start, std::size_t end) {
  if (end > d.size() || start > end) throw std::out_of_range("invalid TLV range");
  std::vector<TlvNode> out;
  std::size_t p = start;
  while (p <= end && end - p >= 6) {
    auto tag = tag_at(d, p);
    auto size = read_u32(d, p + 2);
    if (size > end - p - 6) break;
    TlvNode n;
    n.offset = p;
    n.size = size;
    n.tag = tag;
    if (size && containers.count(tag)) n.children = parse_tlv_recursive(d, p + 6, p + 6 + size);
    if (n.children.empty() && size)
      n.payload.assign(d.begin() + static_cast<std::ptrdiff_t>(p + 6),
                       d.begin() + static_cast<std::ptrdiff_t>(p + 6 + size));
    out.push_back(std::move(n));
    p += 6 + size;
  }
  return out;
}

std::vector<std::pair<std::string, ByteBuffer>> parse_flat(const ByteBuffer& d) {
  std::vector<std::pair<std::string, ByteBuffer>> out;
  std::size_t p = 0;
  while (d.size() - std::min(p, d.size()) >= 6) {
    auto n = read_u32(d, p + 2);
    if (n > d.size() - p - 6) break;
    out.push_back({tag_at(d, p), ByteBuffer(d.begin() + p + 6, d.begin() + p + 6 + n)});
    p += 6 + n;
  }
  return out;
}
}  // namespace openskp
