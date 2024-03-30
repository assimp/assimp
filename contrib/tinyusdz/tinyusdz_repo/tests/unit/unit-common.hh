// SPDX-License-Identifier: Apache 2.0
// Copyrigh 2022 - Present, Light Transport Entertainment Inc.
#pragma once

#include <iostream>
#include <limits>
#include <cmath>

namespace tinyusdz_test {

template <typename T>
static bool float_equals(T x, T y, T eps = std::numeric_limits<T>::epsilon()) {
  if (std::fabs(x - y) < eps) {
    return true;
  }

  return false;
}

// Simple function to check if two float array values are same.
// Assume two arrays have same length.
template <typename T>
static bool float_array_equals(const T *a, const T *b, const int n) {
  for (int i = 0; i < n; i++) {
    if (!float_equals(a[i], b[i])) {
      std::cerr << "float diff. a[" << i << "] = " << a[i] << ", b[" << i
                << "] = " << b[i] << std::endl;
      return false;
    }
  }

  return true;
}

}  // namespace tinyusdz_test
