
#ifndef BOOST_FOREACH

///////////////////////////////////////////////////////////////////////////////
// A stripped down version of FOREACH for
// illustration purposes. NOT FOR GENERAL USE.
// For a complete implementation, see BOOST_FOREACH at
// http://boost-sandbox.sourceforge.net/vault/index.php?directory=eric_niebler
//
// Copyright 2004 Eric Niebler.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Adapted to Assimp November 29th, 2008 (Alexander Gessler).
// Added code to handle both const and non-const iterators, simplified some
// parts.
///////////////////////////////////////////////////////////////////////////////

namespace boost {
namespace foreach_detail {

///////////////////////////////////////////////////////////////////////////////
// auto_any

struct auto_any_base
{
    operator bool() const { return false; }
};

template<typename T>
struct auto_any : auto_any_base
{
    auto_any(T const& t) : item(t) {}
    mutable T item;
};

template<typename T>
T& auto_any_cast(auto_any_base const& any)
{
    return static_cast<auto_any<T> const&>(any).item;
}

///////////////////////////////////////////////////////////////////////////////
// FOREACH helper function

template<typename T>
auto_any<typename T::const_iterator> begin(T const& t)
{
    return t.begin();
}

template<typename T>
auto_any<typename T::const_iterator> end(T const& t)
{
    return t.end();
}

// iterator
template<typename T>
bool done(auto_any_base const& cur, auto_any_base const& end, T&)
{
    typedef typename T::iterator iter_type;
    return auto_any_cast<iter_type>(cur) == auto_any_cast<iter_type>(end);
}

template<typename T>
void next(auto_any_base const& cur, T&)
{
    ++auto_any_cast<typename T::iterator>(cur);
}

template<typename T>
typename T::reference deref(auto_any_base const& cur, T&)
{
    return *auto_any_cast<typename T::iterator>(cur);
}

template<typename T>
typename T::const_reference deref(auto_any_base const& cur, const T&)
{
    return *auto_any_cast<typename T::iterator>(cur);
}

} // end foreach_detail

///////////////////////////////////////////////////////////////////////////////
// FOREACH

#define BOOST_FOREACH(item, container)                      \
	if(boost::foreach_detail::auto_any_base const& foreach_magic_b = boost::foreach_detail::begin(container)) {} else       \
    if(boost::foreach_detail::auto_any_base const& foreach_magic_e = boost::foreach_detail::end(container))   {} else       \
    for(;!boost::foreach_detail::done(foreach_magic_b,foreach_magic_e,container);  boost::foreach_detail::next(foreach_magic_b,container))   \
        if (bool ugly_and_unique_break = false) {} else							\
        for(item = boost::foreach_detail::deref(foreach_magic_b,container); !ugly_and_unique_break; ugly_and_unique_break = true)

} // end boost

#endif
