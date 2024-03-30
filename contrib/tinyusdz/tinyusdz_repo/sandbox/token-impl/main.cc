#include <iostream>

#include "nonstd/optional.hpp"
#include "string_id/database.hpp"
#include "string_id/string_id.hpp"

namespace sid = foonathan::string_id;

// Singleton
class TokenStorage
{
  public:
    TokenStorage(const TokenStorage &) = delete;
    TokenStorage& operator=(const TokenStorage&) = delete;
    TokenStorage(TokenStorage&&) = delete;
    TokenStorage& operator=(TokenStorage&&) = delete;

    static sid::default_database &GetInstance() {
      static sid::default_database s_database;
      return s_database;
    }

  private:
    TokenStorage() = default;
    ~TokenStorage() = default; 
};

class Token {

 public:
  Token() {}

  explicit Token(const char *str) {
    str_ = sid::string_id(str, TokenStorage::GetInstance());
  }

  const std::string str() const {
    if (!str_) {
      return std::string();
    }
    return str_.value().string();
  }

  const uint64_t hash() const {
    if (!str_) {
      return 0;
    }

    return str_.value().hash_code();
  }

 private:
  nonstd::optional<sid::string_id> str_;
};

std::ostream &operator<<(std::ostream &os, const Token &tok) {

  os << tok.str();

  return os;
}

int main(int argc, char **argv) {

  Token tok("bora");

  std::cout << tok << ", id " << tok.hash() << "\n";
 
  return 0;
}
