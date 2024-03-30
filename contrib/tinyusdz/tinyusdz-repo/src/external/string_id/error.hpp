// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STRING_ID_ERROR_HPP_INCLUDED
#define FOONATHAN_STRING_ID_ERROR_HPP_INCLUDED

//#include <exception>
#include <string>

#include "config.hpp"
#include "hash.hpp"

namespace foonathan { namespace string_id
{
    /// \brief The base class for all custom exception classes of this library.
    //class error : public std::exception
    class error 
    {
    protected:
        error() = default;
    };
    
    /// \brief The type of the collision handler.
    /// \detail It will be called when a string hashing results in a collision giving it the two strings collided.
    /// The default handler throws an exception of type \ref collision_error.
    typedef void(*collision_handler)(hash_type hash, const char *a, const char *b);
    
    /// \brief Exchanges the \ref collision_handler.
    /// \detail This function is thread safe if \ref FOONATHAN_STRING_ID_ATOMIC_HANDLER is \c true.
    collision_handler set_collision_handler(collision_handler h);
    
    /// \brief Returns the current \ref collision_handler.
    collision_handler get_collision_handler();
    
    /// \brief The exception class thrown by the default \ref collision_handler.
    class collision_error : public error
    {
    public:
        //=== constructor/destructor ===//
        /// \brief Creates a new exception, same parameter as \ref collision_handler.
        collision_error(hash_type hash, const char *a, const char *b)
        : a_(a), b_(b),
          what_(R"(foonathan::string_id::collision_error: strings ")" + a_ + R"(" and ")" + b_ +
                R"(") are both producing the value )" + std::to_string(hash)), hash_(hash) {}
        
        //~collision_error() FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE {}
        ~collision_error() FOONATHAN_NOEXCEPT {}
        
        //=== accessors ===//
        //const char* what() const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE;
        const char* what() const FOONATHAN_NOEXCEPT;
        
        /// @{
        /// \brief Returns one of the two strings that colllided.
        const char* first_string() const FOONATHAN_NOEXCEPT
        {
            return a_.c_str();
        }
        
        const char* second_string() const FOONATHAN_NOEXCEPT
        {
            return b_.c_str();
        }
        /// @}
        
        /// \brief Returns the hash code of the collided strings.
        hash_type hash_code() const FOONATHAN_NOEXCEPT
        {
            return hash_;
        }
        
    private:
        std::string a_, b_, what_;
        hash_type hash_;
    };
    
    /// \brief The type of the generator error handler.
    /// \detail It will be called when a generator would generate a \ref string_id that already was generated.
    /// The generator will try again until the handler returns \c false in which case it just returns the old \c string_id.
    /// It passes the number of tries, the name of the generator and the hash and string of the generated \c string_id.<br>
    /// The default handler allows 8 tries and then throws an exception of type \ref generation_error.
    typedef bool(*generation_error_handler)(std::size_t no, const char *generator_name,
                                             hash_type hash, const char *str);
    
    /// \brief Exchanges the \ref generation_error_handler.
    /// \detail This function is thread safe if \ref FOONATHAN_STRING_ID_ATOMIC_HANDLER is \c true.
    generation_error_handler set_generation_error_handler(generation_error_handler h);
    
    /// \brief Returns the current \ref generation_error_handler.
    generation_error_handler get_generation_error_handler();
    
    /// \brief The exception class thrown by the default \ref generation_error_handler.
    class generation_error : public error
    {
    public:
        //=== constructor/destructor ===//
        /// \brief Creates it by giving it the name of the generator.
        generation_error(const char *generator_name)
        : name_(generator_name), what_("foonathan::string_id::generation_error: Generator \"" + name_ +
                                       "\" was unable to generate new string id.") {}
        
        //~generation_error() FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE {}
        ~generation_error() FOONATHAN_NOEXCEPT {}
        
        //=== accessors ===//
        //const char* what() const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE;
        const char* what() const FOONATHAN_NOEXCEPT;
        
        /// \brief Returns the name of the generator.
        const char* generator_name() const FOONATHAN_NOEXCEPT
        {
            return name_.c_str();
        }
        
    private:
        std::string name_, what_;
    };
}} // namespace foonathan::string_id

#endif // FOONATHAN_STRING_ID_ERROR_HPP_INCLUDED
