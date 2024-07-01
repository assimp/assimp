// SPDX-License-Identifier: Apache 2.0
// Copyright 2021-2022 Syoyo Fujita.
// Copyright 2023-present Light Transport Entertainment Inc.

///
/// Type-erasure technique for Attribute/PrimVar(Primitive Variables), a Value class which can have 30+ different types(and can be compound-types(e.g. 1D/2D array, dictionary).
/// Neigher std::any nor std::variant is applicable for such usecases, so write our own, handy typesystem.
///
/// TODO: Rename PrimVar to something more better one(AttributeValue?).
///
#pragma once

#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>
#include <cmath>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

// TODO(syoyo): Use C++17 std::optional, std::string_view when compiled with C++-17 compiler
#include "nonstd/optional.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "value-types.hh"

namespace tinyusdz {
namespace primvar {

struct PrimVar {
  value::Value _value{nullptr}; // For scalar(default) value
  bool _blocked{false}; // ValueBlocked.
  value::TimeSamples _ts; // For TimeSamples value.

  bool is_scalar() const {
    return _ts.empty();
  }

  bool is_timesamples() const {
    return _ts.size();
  }

  bool is_blocked() const {
    // Fist check if stored value is ValueBlock, then return _blocked.
    if (_value.type_id() == value::TYPE_ID_VALUEBLOCK) {
      return true;
    }
    return _blocked;
  }

  void set_blocked(bool onoff) {
    // fast path
    _blocked = onoff;
  }

  bool is_valid() const {
    if (is_timesamples()) {
      return _ts.type_id() != value::TypeId::TYPE_ID_INVALID;
    } else {
      return _value.type_id() != value::TypeId::TYPE_ID_INVALID;
    }
  }

  std::string type_name() const {
    if (is_timesamples()) {
      return _ts.type_name();
    } else { // Assume scalar.
      return _value.type_name();
    }
  }

  uint32_t type_id() const {
    if (!is_valid()) {
      return value::TYPE_ID_INVALID;
    }

    if (is_timesamples()) {
      return _ts.type_id();
    } else {
      return _value.type_id();
    }
  }

  // Type-safe way to get concrete value for non-timesamples data.
  // NOTE: This consumes lots of stack size(rougly 1000 bytes),
  // If you need to handle multiple types, use as() insted.
  // 
  template <class T>
  nonstd::optional<T> get_value() const {

    if (!is_scalar()) {
      return nonstd::nullopt;
    }

    return _value.get_value<T>();
  }

  nonstd::optional<double> get_ts_time(size_t idx) const {

    if (!is_timesamples()) {
      return nonstd::nullopt;
    }

    if (idx >= _ts.size()) {
      return nonstd::nullopt;
    }

    return _ts.get_time(idx);
  }

  nonstd::optional<value::TimeSamples::Sample> get_timesample(size_t idx) const {
    if (idx < _ts.get_samples().size()) {
      return _ts.get_samples()[idx];
    }
    return nonstd::nullopt;
  }

  // Type-safe way to get concrete value for timesampled variable.
  // No interpolation.
  template <class T>
  nonstd::optional<T> get_ts_value(size_t idx) const {

    if (!is_timesamples()) {
      return nonstd::nullopt;
    }

    nonstd::optional<value::Value> pv = _ts.get_value(idx);
    if (!pv) {
      return nonstd::nullopt;
    }

    return pv.value().get_value<T>();
  }

  // Check if specific TimeSample value for a specified index is ValueBlock or not.
  nonstd::optional<bool> is_ts_value_blocked(size_t idx) const {

    if (!is_timesamples()) {
      return nonstd::nullopt;
    }

    if (idx >= _ts.get_samples().size()) {
      return nonstd::nullopt;
    }

    return _ts.get_samples()[idx].blocked;
  }

  // For Scalar only
  // Returns nullptr when type-mismatch.
  template <class T>
  const T* as() const {

    if (!is_scalar()) {
      return nullptr;
    }

    return _value.as<T>();
  }

  template <class T>
  void set_value(const T &v) {
    //_ts.clear(); // timeSamples and (defaut) value should co-exist.
    _value = v;
  }

  void set_timesamples(const value::TimeSamples &v) {
    _ts = v;
  }

  void set_timesamples(value::TimeSamples &&v) {
    _ts = std::move(v);
  }

  template <typename T>
  void set_timesample(double t, const T &v) {
    _ts.add_sample(t, v);
  }

  void set_timesample(double t, value::Value &v) {
    _ts.add_sample(t, v);
  }

#if 0 // TODO
  ///
  /// Get typed TimesSamples
  ///
  template<typename T>
  bool get_timesamples(TypedTimeSamples<T> *dst) {
    if (!is_timesamples()) {
      return false;
    }

    TypedTimeSamples<T> tss;
    std::vector<TimedTimeSample::Sample<T>> buf;
    for (size_t i = 0; i < ts.size(); i++) {
      if (ts.get_samples()[i].value.type_id() != value::TypeTraits<T>::type_id()) {
        return false;
      }
      Sample s;
      s.t = ts.get_samples()[i].t;
      s.blocked = ts.get_samples()[i].blocked;
      if (const auto pv = ts.get_samples()[i].value.as<T>()) {
        s.value = ts.get_samples()[i].value;
      } else {
        return false;
      }
   
      buf.push_back(s);
    }
  
  
      _samples = std::move(buf);
      _dirty = true;
  
      return true;
    }
#endif


  ///
  /// Get interpolated timesample value.
  ///
  bool get_interpolated_value(const double t, const value::TimeSampleInterpolationType tinterp, value::Value *v) const;

  
  template <typename T>
  bool get_interpolated_value(const double t, const value::TimeSampleInterpolationType tinterp, T *v) const {
    return _ts.get(v, t, tinterp);
  }

  size_t num_timesamples() const {
    if (is_timesamples()) {
      return _ts.size();
    }
    return 0;
  }

  const value::TimeSamples &ts_raw() const {
    return _ts;
  }
  
  value::Value &value_raw() {
    return _value;
  }

  const value::Value &value_raw() const {
    return _value;
  }
  
  value::TimeSamples &ts_raw() {
    return _ts;
  }
};


} // namespace primvar
} // namespace tinyusdz
