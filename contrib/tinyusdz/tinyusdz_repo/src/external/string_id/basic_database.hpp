// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STRING_ID_BASIC_DATABASE_HPP_INCLUDED
#define FOONATHAN_STRING_ID_BASIC_DATABASE_HPP_INCLUDED

#include "config.hpp"
#include "hash.hpp"

namespace foonathan { namespace string_id
{
	/// \brief The interface for all databases.
    /// \detail You can derive own databases from it.
    class basic_database
    {
    public:        
        /// @{
        /// \brief Databases are not copy- or moveable.
        /// \detail You must not write a swap function either!<br>
        /// This has implementation reasons.
        basic_database(const basic_database &) = delete;
        basic_database(basic_database &&) = delete;
        /// @}

        virtual ~basic_database() = default;
        
        /// \brief The status of an insert operation.
        enum insert_status
        {
            /// \brief Two different strings collide on the same value.
            collision,
            /// \brief A new string was inserted.
            new_string,
            /// \brief The string already existed inside the database.
            old_string
        };
        
        /// \brief Should insert a new hash-string-pair with prefix (optional) into the internal database.
        /// \detail The string must be copied prior to storing, it may not stay valid.
        /// \arg \c hash is the hash of the string.
        /// \arg \c str is the string which does not need to be null-terminated.
        /// \arg \c length is the length of the string.
        /// \return The \ref insert_status.
        virtual insert_status insert(hash_type hash, const char* str, std::size_t length) = 0;
        
        /// \brief Inserts a hash-string-pair with given prefix into the internal database.
        /// \detail The default implementation performs a lookup of the prefix string and appends it,
        /// then it calls \ref insert.<br>
        /// Override it if you can do it more efficiently.
        /// \arg \c hash is the hash of the string plus prefix.
        /// \arg \c prefix is the hash of the prefix-string.
        /// \arg \c str is the suffix which does not need to be null-terminated.
        /// \arg \c length is the length of the suffix.
        /// \return The \ref insert_status.
        virtual insert_status insert_prefix(hash_type hash, hash_type prefix,
                                            const char *str, std::size_t length);
        
        /// \brief Should return the string stored with a given hash.
        /// \detail It is guaranteed that the hash value has been inserted before.
        /// \return A null-terminated string belonging to the hash code or
        /// an error message if the database does not store anything.<br>
        /// The return value must stay valid as long as the database exists.
        virtual const char* lookup(hash_type hash) const FOONATHAN_NOEXCEPT = 0;
        
    protected:
        basic_database() = default;
    };
}} // foonathan::string_id

#endif // FOONATHAN_STRING_ID_BASIC_DATABASE_HPP_INCLUDED
