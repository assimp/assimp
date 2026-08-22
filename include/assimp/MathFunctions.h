/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

#pragma once

#ifdef __GNUC__
#   pragma GCC system_header
#endif

/** @file  MathFunctions.h
*  @brief Implementation of math utility functions.
 *
*/

#include <limits>

namespace Assimp {
namespace Math {

/// @brief  Will return the greatest common divisor.
/// @param  a   [in] Value a.
/// @param  b   [in] Value b.
/// @return The greatest common divisor.
template <typename IntegerType>
inline IntegerType gcd( IntegerType a, IntegerType b ) {
	const IntegerType zero = (IntegerType)0;
	while ( true ) {
		if ( a == zero ) {
			return b;
        }
		b %= a;

		if ( b == zero ) {
			return a;
        }
		a %= b;
	}
}

/// @brief  Will return the greatest common divisor.
/// @param  a   [in] Value a.
/// @param  b   [in] Value b.
/// @return The greatest common divisor.
template < typename IntegerType >
inline IntegerType lcm( IntegerType a, IntegerType b ) {
	const IntegerType t = gcd (a,b);
	if (!t) {
        return t;
    }
	return a / t * b;
}
/// @brief  Will return the smallest epsilon-value for the requested type.
/// @return The numercical limit epsilon depending on its type.
template<class T>
inline T getEpsilon() {
    return std::numeric_limits<T>::epsilon();
}

/// @brief  Will return the constant PI for the requested type.
/// @return Pi
template<class T>
inline T aiPi() {
    return static_cast<T>(3.14159265358979323846);
}

/// @brief  Safely multiply two values, checking for integer overflow.
/// @param  a   [in] First value
/// @param  b   [in] Second value
/// @param  result [out] Result of multiplication if no overflow
/// @return true if multiplication succeeded without overflow, false if overflow detected
template<typename T>
inline bool SafeMultiply(T a, T b, T& result) {
    // Handle zero cases
    if (a == 0 || b == 0) {
        result = 0;
        return true;
    }

    // Check if multiplication would overflow
    if (a > std::numeric_limits<T>::max() / b) {
        return false;
    }

    result = a * b;
    return true;
}

/// @brief  Safely multiply a size and count for buffer allocation, checking for overflow.
/// @param  pSize   [in] Size of each element
/// @param  pCount  [in] Number of elements
/// @return Result of multiplication, or 0 if overflow detected
template<typename T>
inline T SafeMultiplySizeCount(T pSize, T pCount) {
    T result;
    if (!SafeMultiply(pSize, pCount, result)) {
        return 0;  // Return 0 to indicate overflow/error
    }
    return result;
}

} // namespace Math
} // namespace Assimp
