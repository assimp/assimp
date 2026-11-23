// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <GL/freeglut.h>

#include <set>
#include <map>
#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <fstream>
#include <sstream>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <algorithm>

// -------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------------------------
// Adopted from https://i-a-s.me vulkan series

#define CHECK_RESULT(_result)                                                                                          \
  do                                                                                                                   \
  {                                                                                                                    \
    const auto check_result_ = _result;                                                                                \
    if (check_result_.is_error())                                                                                      \
      return check_result_.get_error();                                                                                \
  } while (0)

#define CHECK_ASSIGN_RESULT(_result, _var)                                                                             \
  do                                                                                                                   \
  {                                                                                                                    \
    auto check_result_ = _result;                                                                                      \
    if (check_result_.is_error())                                                                                      \
      return check_result_.get_error();                                                                                \
    _var = std::move(check_result_).take_value();                                                                      \
  } while (0)

struct Ok
{
};

// A very simple Result type to hold either
// an error message or the value. To be used
// mostly as return types in failable functions.
template<typename T = Ok> class [[nodiscard]] Result
{
  static_assert(!std::is_same_v<std::decay_t<T>, std::string>, "std::string values are not allowed");
  static_assert(!std::is_same_v<T, void>, "void values are not allowed. Use Result<> or Result<Ok> instead.");

public:
  Result(std::nullptr_t) = delete;

  constexpr Result(const char *error) : m_variant(std::in_place_index<1>, error)
  {
  }

  constexpr Result(const std::string &error) : m_variant(std::in_place_index<1>, error)
  {
  }

  constexpr Result(const T &value) : m_variant(std::in_place_index<0>, value)
  {
  }

  constexpr Result(T &&value) : m_variant(std::in_place_index<0>, std::move(value))
  {
  }

  constexpr bool is_ok() const
  {
    return m_variant.index() == 0;
  }

  constexpr bool is_error() const
  {
    return m_variant.index() == 1;
  }

  T get_value() && = delete;

  T &get_value() &
  {
    assert(is_ok());
    return std::get<0>(m_variant);
  }

  const T &get_value() const &
  {
    assert(is_ok());
    return std::get<0>(m_variant);
  }

  T take_value() &&
  {
    assert(is_ok());
    return std::get<0>(std::move(m_variant));
  }

  const std::string &get_error() const &
  {
    assert(is_error());
    return std::get<1>(m_variant);
  }

  std::string get_error() &&
  {
    assert(is_error());
    return std::get<1>(std::move(m_variant));
  }

private:
  std::variant<T, std::string> m_variant;
};

// Alias std::unique_ptr to Box. Easier to type.
// (Inspired by Rust of course, just like Result above).
template<typename T> using Box = std::unique_ptr<T>;

// -------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------------------------
