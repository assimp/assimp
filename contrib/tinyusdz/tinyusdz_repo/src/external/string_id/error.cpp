// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "error.hpp"

#include <atomic>
#include <sstream>

namespace sid = foonathan::string_id;

namespace
{    
    void default_collision_handler(sid::hash_type hash, const char *a, const char *b)
    {
        auto ret = sid::collision_error(hash, a, b);
        // TODO
        (void)ret;

        //throw sid::collision_error(hash, a, b);
    }
    
#if FOONATHAN_STRING_ID_ATOMIC_HANDLER
    std::atomic<sid::collision_handler> collision_h(default_collision_handler);
#else
    sid::collision_handler collision_h(default_collision_handler);
#endif
}

sid::collision_handler sid::set_collision_handler(collision_handler h)
{
#if FOONATHAN_STRING_ID_ATOMIC_HANDLER
    return collision_h.exchange(h);
#else
    auto val = collision_h;
    collision_h = h;
    return val;
#endif
}

sid::collision_handler sid::get_collision_handler()
{
    return collision_h;
}

#if 0
const char* sid::collision_error::what() const FOONATHAN_NOEXCEPT try
{
    return what_.c_str();
}
catch (...)
{
    return "foonathan::string_id::collision_error: two different strings are producing the same value";
}
#else
const char* sid::collision_error::what() const FOONATHAN_NOEXCEPT
{
    return what_.c_str();
}
#endif

namespace
{
    FOONATHAN_CONSTEXPR auto no_tries_generation = 8u;
    
    bool default_generation_error_handler(std::size_t no, const char *generator_name,
                                          sid::hash_type, const char *)
    {
        if (no >= no_tries_generation) {
            //throw sid::generation_error(generator_name);
            return false;
        }
        return true;
    }
    
#if FOONATHAN_STRING_ID_ATOMIC_HANDLER
    std::atomic<sid::generation_error_handler> generation_error_h(default_generation_error_handler);
#else
    sid::generation_error_handler generation_error_h(default_generation_error_handler);
#endif
}

sid::generation_error_handler sid::set_generation_error_handler(generation_error_handler h)
{
#if FOONATHAN_STRING_ID_ATOMIC_HANDLER
    return generation_error_h.exchange(h);
#else
    auto val = generation_error_h;
    generation_error_h = h;
    return val;
#endif
}

sid::generation_error_handler sid::get_generation_error_handler()
{
    return generation_error_h;
}

#if 0
const char* sid::generation_error::what() const FOONATHAN_NOEXCEPT try
{
    return what_.c_str();
}
catch (...)
{
    return "foonathan::string_id::generation_error: unable to generate new string id.";
}
#else
const char* sid::generation_error::what() const FOONATHAN_NOEXCEPT
{
    return what_.c_str();
}
#endif
