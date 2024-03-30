/*
    nanobind/stl/unique_ptr.h: Type caster for std::unique_ptr<T>

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <memory>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename T, typename Deleter>
struct type_caster<std::unique_ptr<T, Deleter>> {
    using Value = std::unique_ptr<T, Deleter>;
    using Caster = make_caster<T>;

    static constexpr bool IsDefaultDeleter =
        std::is_same_v<Deleter, std::default_delete<T>>;
    static constexpr bool IsNanobindDeleter =
        std::is_same_v<Deleter, deleter<T>>;

    static_assert(Caster::IsClass,
                  "Binding 'std::unique_ptr<T>' requires that 'T' can also be "
                  "bound by nanobind. It appears that you specified a type which "
                  "would undergo conversion/copying, which is not allowed.");

    static_assert(IsDefaultDeleter || IsNanobindDeleter,
                  "Binding std::unique_ptr<T, Deleter> requires that 'Deleter' is either "
                  "'std::default_delete<T>' or 'nanobind::deleter<T>'");

    static constexpr auto Name = Caster::Name;
    static constexpr bool IsClass = true;
    template <typename T_> using Cast = Value;

    Caster caster;
    handle src;

    bool from_python(handle src_, uint8_t, cleanup_list *) noexcept {
        // Stash source python object
        src = src_;

        /* Try casting to a pointer of the underlying type. We pass flags=0 and
           cleanup=nullptr to prevent implicit type conversions (they are
           problematic since the instance then wouldn't be owned by 'src') */
        return caster.from_python(src_, 0, nullptr);
    }

    template <typename T2>
    static handle from_cpp(T2 *value, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        if (!value)
            return handle();

        return from_cpp(*value, policy, cleanup);
    }

    template <typename T2>
    static handle from_cpp(T2 &&value,
                           rv_policy, cleanup_list *cleanup) noexcept {

        bool cpp_delete = true;
        if constexpr (IsNanobindDeleter)
            cpp_delete = value.get_deleter().owned_by_cpp();

        handle result =
            nb_type_put_unique(&typeid(T), value.get(), cleanup, cpp_delete);

        if (result.is_valid()) {
            if (cpp_delete)
                value.release();
            else
                value.reset();
        }

        return result;
    }

    explicit operator Value() {
        nb_type_relinquish_ownership(src.ptr(), IsDefaultDeleter);

        T *value = caster.operator T *();
        if constexpr (IsNanobindDeleter)
            return Value(value, deleter<T>(src.inc_ref()));
        else
            return Value(value);
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
