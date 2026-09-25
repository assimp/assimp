#include "internal.hpp"

namespace openskp {
void emit_log(const ParseOptions& o, LogLevel l, const std::string& m) {
  if (o.log) o.log(l, m);
}

void emit_progress(const ParseOptions& o, ParseStage s, std::uint64_t c, std::uint64_t t) {
  if (o.progress) o.progress({s, c, t});
}
}  // namespace openskp
