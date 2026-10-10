#include <sstream>

#include "internal.hpp"

namespace openskp {
static std::string format_error(std::string m, const std::optional<ParseStage>& s,
                                const std::optional<std::uint64_t>& r,
                                const std::optional<std::uint64_t>& t,
                                const std::optional<std::string>& tag,
                                const std::optional<std::uint64_t>& o,
                                const std::optional<std::int64_t>& d) {
  std::ostringstream x;
  x << m;
  if (s) x << " | stage=" << stage_name(*s);
  if (r && t) x << " | record=" << *r << '/' << *t;
  if (tag) x << " | tag=" << *tag;
  if (o) x << " | offset=0x" << std::hex << *o << std::dec;
  if (d) x << " | definitionId=" << *d;
  return x.str();
}

SkpParseError::SkpParseError(std::string m, std::optional<ParseStage> s,
                             std::optional<std::uint64_t> r, std::optional<std::uint64_t> t,
                             std::optional<std::string> tag, std::optional<std::uint64_t> o,
                             std::optional<std::int64_t> d, std::exception_ptr c)
    : runtime_error(format_error(std::move(m), s, r, t, tag, o, d)),
      stage_(s),
      record_index_(r),
      total_records_(t),
      offset_(o),
      tag_(std::move(tag)),
      definition_id_(d),
      cause_(c) {}
}  // namespace openskp
