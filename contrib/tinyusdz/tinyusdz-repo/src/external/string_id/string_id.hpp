// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STRING_ID_HPP_INCLUDED
#define FOONATHAN_STRING_ID_HPP_INCLUDED

#include <cstring>
#include <functional>

#include "basic_database.hpp"
#include "config.hpp"
#include "hash.hpp"

namespace foonathan { namespace string_id
{
    /// \brief Information about a string.
    /// \detail It is used to reduce the number of constructors of \ref string_id.
    struct string_info
    {
        /// \brief A pointer to a null-terminated string.
        const char *string;
        /// \brief The length of this string.
        std::size_t length;
        
        /// \brief Creates it from a null-terminated string (implicit conversion).
        string_info(const char *str) FOONATHAN_NOEXCEPT
        : string(str), length(std::strlen(str)) {}
        
        /// \brief Creates it from a string with given length.
        /// \arg \c str does not need to be null-terminated.
        string_info(const char *str, std::size_t length)
        : string(str), length(length) {}
    };
    
    /// \brief The string identifier class.
    /// \detail This is a lightweight class to store strings.<br>
    /// It only stores a hash of the string allowing fast copying and comparisons.
    class string_id
    {
    public:
        //=== constructors ===//
        /// \brief Creates a new id by hashing a given string.
        /// \detail It will insert the string into the given \ref database which will copy it.<br>
        /// If it encounters a collision, the \ref collision_handler will be called.
        string_id(string_info str, basic_database &db);
        
        /// \brief Creates a new id with a given prefix.
        /// \detail The new id will be inserted into the same database as the prefix.<br>
        //// Otherwise the same as other constructor.
        string_id(const string_id &prefix, string_info str);
        
        /// @{
        /// \brief Sames as other constructor versions but instead of calling the \ref collision_handler,
        /// they set the output parameter to the appropriate status.
        /// \detail This also allows information whether or not the string was already stored inside the database.
        string_id(string_info str, basic_database &db,
                 basic_database::insert_status &status);
                 
        string_id(const string_id &prefix, string_info str,
                  basic_database::insert_status &status);
        /// @}
        
        //=== accessors ===//
        /// \brief Returns the hashed value of the string.
        hash_type hash_code() const FOONATHAN_NOEXCEPT
        {
            return id_;
        }
        
        /// \brief Returns a reference to the database.
        basic_database& database() const FOONATHAN_NOEXCEPT
        {
            return *db_;
        }
        
        /// \brief Returns the string value itself.
        /// \detail This calls the \c lookup function on the database.
        const char* string() const FOONATHAN_NOEXCEPT;
        
        //=== comparision ===//
        /// @{
        /// \brief Compares string ids with another or hashed values.
        /// \detail Two string ids are equal if they are from the same database and they have the same value.<br>
        /// A hashed value is equal to a string id if it is the same value.
        friend bool operator==(string_id a, string_id b) FOONATHAN_NOEXCEPT
        {
            return a.db_ == b.db_ && a.id_ == b.id_;
        }
        
        friend bool operator==(hash_type a, const string_id &b) FOONATHAN_NOEXCEPT
        {
            return a == b.id_;
        }
        
        friend bool operator==(const string_id &a, hash_type b) FOONATHAN_NOEXCEPT
        {
            return a.id_ == b;
        }
        
        friend bool operator!=(const string_id &a, const string_id &b) FOONATHAN_NOEXCEPT
        {
            return !(a == b);
        }
        
        friend bool operator!=(hash_type a, const string_id &b) FOONATHAN_NOEXCEPT
        {
            return !(a == b);
        }
        
        friend bool operator!=(const string_id &a, hash_type b) FOONATHAN_NOEXCEPT
        {
            return !(a == b);
        }
        /// @}
        
    private:
        hash_type id_;
        basic_database *db_;
    };
    
    namespace literals
    {
        /// \brief Same as the literal version, additional replacement if not supported.
        FOONATHAN_CONSTEXPR_FNC hash_type id(const char *str)
        {
            return detail::sid_hash(str);
        }
        
        /// \brief A useful literal to hash a string.
        /// \detail Since this function does not check for collisions only use it to compare a \ref string_id.<br>
        /// It is also useful in places where a compile-time constant is needed.
    #if FOONATHAN_STRING_ID_HAS_LITERAL
        FOONATHAN_CONSTEXPR_FNC hash_type operator""_id(const char *str, std::size_t)
        {
            return detail::sid_hash(str);
        }
    #endif
    } // namespace literals
}} // namespace foonathan::string_id

namespace std
{
    /// \brief \c std::hash support for \ref string_id.
    template <>
    struct hash<foonathan::string_id::string_id>
    {
        typedef foonathan::string_id::string_id argument_type;
        typedef size_t result_type;        
 
        result_type operator()(const argument_type &arg) const FOONATHAN_NOEXCEPT
        {
            return static_cast<result_type>(arg.hash_code());
        }
    };
} // namspace std

#endif // FOONATHAN_STRING_ID_HPP_INCLUDED 
