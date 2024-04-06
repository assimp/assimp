// SPDX-License-Identifier: MIT
// Copyright 2024 - Present, Light Transport Entertainment Inc.
//
// UTF-8 Unicode identifier XID_Start and XID_Continue validation utility.
//
// Based on UAX31 Default Identifier and Unicode 5.1.0
#pragma once

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>
#include <limits>

namespace unicode_xid {

constexpr uint32_t kMaxCodepoint = 0x10FFFF;

namespace detail {

// Assume table is sorted by the first key(lower)
#include "unicode-xid-table.inc"

}


inline bool is_xid_start(uint32_t codepoint) {
  if (codepoint > kMaxCodepoint) {
    return false;
  }

  // first find lower location based on the first key, then test with second key with linear search for (lower <= codepoint <= upper) range check.
  // NOTE: second item in query is not used. fill it T::min just in case.
  auto it = std::lower_bound(detail::kXID_StartTable.begin(), detail::kXID_StartTable.end(), std::make_pair(int(codepoint), (std::numeric_limits<int>::min)()));

  // subtract 1 to get the first entry of possible hit(lower <= codepoint <= upper)
  if ((it != detail::kXID_StartTable.begin() && (int(codepoint) < it->second))) {
    it--;
  }

  for (; it != detail::kXID_StartTable.end(); it++) {
    if ((int(codepoint) >= it->first) && (int(codepoint) <= it->second)) { // range end is inclusive.  
      return true;
    }
  }

  return false;
}

inline bool is_xid_continue(uint32_t codepoint) {
  if (codepoint > kMaxCodepoint) {
    return false;
  }

  auto it = std::lower_bound(detail::kXID_ContinueTable.begin(), detail::kXID_ContinueTable.end(), std::make_pair(int(codepoint), (std::numeric_limits<int>::min)()));

  // subtract 1 to get the first entry of possible hit(lower <= codepoint <= upper)
  if ((it != detail::kXID_ContinueTable.begin() && (int(codepoint) < it->second))) {
    it--;
  }

  for (; it != detail::kXID_ContinueTable.end(); it++) {
    if ((int(codepoint) >= it->first) && (int(codepoint) <= it->second)) { // range end is inclusive.  
      return true;
    }
  }

  return false;
}

} // namespace unicode_xid


