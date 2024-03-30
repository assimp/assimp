#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "nonstd/expected.hpp"
#include "value-types.hh"

///
/// Simple Python-like format utility. Only supports "{}".
///
namespace detail {

template <class T>
std::ostringstream &format_sv_rec(std::ostringstream &ss,
                               const std::vector<std::string> &sv, size_t idx,
                               T const &v) {
  if (idx >= sv.size()) {
    return ss;
  }

  // Print remaininig items
  bool fmt_printed{false};

  for (size_t i = idx; i < sv.size(); i++) {
    if (sv[i] == "{}") {
      if (fmt_printed) {
        ss << sv[i];
      } else {
        ss << v;
        fmt_printed = true;
      }
    } else {
      ss << sv[i];
    }
  }

  return ss;
}

template <class T, class... Rest>
std::ostringstream &format_sv_rec(std::ostringstream &ss,
                                  const std::vector<std::string> &sv,
                                  size_t idx, T const &v, Rest const &...args) {
  if (idx >= sv.size()) {
    return ss;
  }

  if (sv[idx] == "{}") {
    ss << v;
    format_sv_rec(ss, sv, idx + 1, args...);
  } else {
    ss << sv[idx];
    format_sv_rec(ss, sv, idx + 1, v, args...);
  }

  return ss;
}

template <class... Args>
std::ostringstream &format_sv(std::ostringstream &ss,
                              const std::vector<std::string> &sv,
                              Args const &...args) {
  format_sv_rec(ss, sv, 0, args...);

  return ss;
}

template <>
std::ostringstream &format_sv(std::ostringstream &ss,
                              const std::vector<std::string> &sv) {
  if (sv.empty()) {
    return ss;
  }

  for (const auto &item : sv) {
    ss << item;
  }

  return ss;
}

nonstd::expected<std::vector<std::string>, std::string> tokenize(
    const std::string &s) {
  size_t n = s.length();

  bool open_curly_brace = false;

  std::vector<std::string> toks;
  size_t si = 0;

  for (size_t i = 0; i < n; i++) {
    if (s[i] == '{') {
      if (open_curly_brace) {
        // nested '{'
        return nonstd::make_unexpected("Nested '{'.");
      }

      open_curly_brace = true;

      if (si >= i) {  // previous char is '}'
        // do nothing
      } else {
        toks.push_back(std::string(s.begin() + si, s.begin() + i));

        si = i;
      }

    } else if (s[i] == '}') {
      if (open_curly_brace) {
        // must be "{}" for now
        if ((i - si) > 1) {
          return nonstd::make_unexpected(
              "Format specifier in '{}' is not yet supported.");
        }

        open_curly_brace = false;

        toks.push_back("{}");

        si = i + 1;  // start from next char.

      } else {
        // Currently we allow string like '}', "}}", "bora}".
        // TODO: strict check for '{' pair.
      }
    }
  }

  if (si < n) {
    toks.push_back(std::string(s.begin() + si, s.begin() + n));
  }

  return std::move(toks);
}

} // namespace detail

template <class... Args>
std::string format(const std::string &in, Args const &...args) {
  auto ret = detail::tokenize(in);
  if (!ret) {
    return in + "(format error: " + ret.error() + ")";
  }

  std::ostringstream ss;
  detail::format_sv(ss, (*ret), args...);

  return ss.str();
}

template <>
std::string format(const std::string &in) {
  return in;
}

void test(const std::string &in) {
  std::cout << format(in) << "\n";
  std::cout << format(in, 1.0f) << "\n";
  std::cout << format(in, 1.0f, 2.0f) << "\n";
  std::cout << format(in, 1.0f, 2.0f, 3.0f) << "\n";
}

int main(int argc, char **argv) {
  test("{}");
  test("{");
  test("}");
  test("{{");
  test("}}");
  test("{a}");
  test("bora {}");
  test("{} dora");
  test("{} dora{} bora muda {");
  test("{} dora{} bora muda{}");

  return 0;
}
