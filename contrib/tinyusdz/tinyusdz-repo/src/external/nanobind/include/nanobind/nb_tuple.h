/*
    nanobind/nb_tuple.h: tiny self-contained tuple class

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/**
 * \brief nanobind::tuple<...>: a tiny recursive tuple class
 *
 * std::tuple<...> is one of those STL types that just seems unnecessarily
 * complex for typical usage. It pulls in a large amount of headers (22K LOC,
 * 530 KiB with Clang/libc++) and goes through elaborate contortions to avoid a
 * recursive definition. This is helpful when dealing with very large tuples
 * (e.g. efficient compilation of std::get<1000>() in a tuple with 10K entries).
 * When working with small tuples used to pass around a few arguments, a simple
 * recursive definition compiles faster (generated code is identical).
 */

template <typename... Ts> struct tuple;
template <> struct tuple<> {
    template <size_t> using type = void;
};

template <typename T, typename... Ts> struct tuple<T, Ts...> : tuple<Ts...> {
    using Base = tuple<Ts...>;

    tuple() = default;
    tuple(const tuple &) = default;
    tuple(tuple &&) noexcept = default;
    tuple& operator=(tuple &&) noexcept = default;
    tuple& operator=(const tuple &) = default;

    template <typename A, typename... As>
    NB_INLINE tuple(A &&a, As &&...as)
        : Base((detail::forward_t<As>) as...), value((detail::forward_t<A>) a) {}

    template <size_t I> NB_INLINE auto& get() {
        if constexpr (I == 0)
            return value;
        else
            return Base::template get<I - 1>();
    }

    template <size_t I> NB_INLINE const auto& get() const {
        if constexpr (I == 0)
            return value;
        else
            return Base::template get<I - 1>();
    }

    template <size_t I>
    using type =
        std::conditional_t<I == 0, T, typename Base::template type<I - 1>>;

private:
    T value;
};

template <typename... Ts> tuple(Ts &&...) -> tuple<std::decay_t<Ts>...>;

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)

// Support for C++17 structured bindings
template <typename... Ts>
struct std::tuple_size<nanobind::detail::tuple<Ts...>>
    : std::integral_constant<size_t, sizeof...(Ts)> { };

template <size_t I, typename... Ts>
struct std::tuple_element<I, nanobind::detail::tuple<Ts...>> {
    using type = typename nanobind::detail::tuple<Ts...>::template type<I>;
};
