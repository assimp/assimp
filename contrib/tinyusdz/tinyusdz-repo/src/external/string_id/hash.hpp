// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STRING_ID_HASH_HPP_INCLUDED
#define FOONATHAN_STRING_ID_HASH_HPP_INCLUDED

#include <cstdint>

#include "config.hpp"

namespace foonathan { namespace string_id
{
    /// \brief The type of a hashed string.
    /// \details This is an unsigned integral type.
    typedef std::uint64_t hash_type;

    namespace detail
    {
        FOONATHAN_CONSTEXPR hash_type fnv_basis = 14695981039346656037ull;
        FOONATHAN_CONSTEXPR hash_type fnv_prime = 1099511628211ull;

        // FNV-1a 64 bit hash
        FOONATHAN_CONSTEXPR_FNC hash_type sid_hash(const char *str, hash_type hash = fnv_basis)
        {
            return *str ? sid_hash(str + 1, (hash ^ *str) * fnv_prime) : hash;
        }
    } // namespace detail
}} // foonathan::string_id

#endif // FOONATHAN_STRING_ID_DETAIL_HASH_HPP_INCLUDED
