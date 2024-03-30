#pragma once

#include <nanobind/nanobind.h>
#include <tuple>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename... Ts> struct type_caster<std::tuple<Ts...>> {
    static constexpr size_t N  = sizeof...(Ts),
                            N1 = N > 0 ? N : 1;

    using Value = std::tuple<Ts...>;
    using Indices = std::make_index_sequence<N>;

    static constexpr bool IsClass = false;
    static constexpr auto Name = const_name("tuple[") +
                                 concat(make_caster<Ts>::Name...) +
                                 const_name("]");

    template <typename T> using Cast = Value;

    bool from_python(handle src, uint8_t flags,
                     cleanup_list *cleanup) noexcept {
        return from_python_impl(src, flags, cleanup, Indices{});
    }

    template <size_t... Is>
    NB_INLINE bool from_python_impl(handle src, uint8_t flags,
                                    cleanup_list *cleanup,
                                    std::index_sequence<Is...>) noexcept {
        (void) src; (void) flags; (void) cleanup;

        PyObject *temp; // always initialized by the following line
        PyObject **o = seq_get_with_size(src.ptr(), N, &temp);

        bool success =
            (o && ... &&
             std::get<Is>(casters).from_python(o[Is], flags, cleanup));

        Py_XDECREF(temp);

        return success;
    }

    template <typename T>
    static handle from_cpp(T&& value, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        return from_cpp_impl((forward_t<T>) value, policy, cleanup, Indices{});
    }

    template <typename T>
    static handle from_cpp(T *value, rv_policy policy, cleanup_list *cleanup) {
        if (!value)
            return none().release();
        return from_cpp_impl(*value, policy, cleanup, Indices{});
    }

    template <typename T, size_t... Is>
    static NB_INLINE handle from_cpp_impl(T &&value, rv_policy policy,
                                          cleanup_list *cleanup,
                                          std::index_sequence<Is...>) noexcept {
        (void) value; (void) policy; (void) cleanup;
        object o[N1];

        bool success =
            (... &&
             ((o[Is] = steal(make_caster<Ts>::from_cpp(
                   forward_like<T>(std::get<Is>(value)), policy, cleanup))),
              o[Is].is_valid()));

        if (!success)
            return handle();

        PyObject *r = PyTuple_New(N);
        (NB_TUPLE_SET_ITEM(r, Is, o[Is].release().ptr()), ...);
        return r;
    }

    explicit operator Value() & {
        return cast_impl(Indices{});
    }

    explicit operator Value() && {
        return ((type_caster &&) *this).cast_impl(Indices{});
    }

    template <size_t... Is>
    NB_INLINE Value cast_impl(std::index_sequence<Is...>) & {
        return Value(std::get<Is>(casters).operator cast_t<Ts>()...);
    }

    template <size_t... Is>
    NB_INLINE Value cast_impl(std::index_sequence<Is...>) && {
        return Value(((make_caster<Ts> &&) std::get<Is>(casters)).operator cast_t<Ts>()...);
    }

    std::tuple<make_caster<Ts>...> casters;
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
