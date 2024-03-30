/*
    nanobind/stl/shared_ptr.h: Type caster for std::shared_ptr<T>

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <memory>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/**
 * Create a generic std::shared_ptr to evade population of a potential
 * std::enable_shared_from_this weak pointer. The specified deleter reduces the
 * reference count of the Python object.
 *
 * The next two functions are simultaneously marked as 'inline' (to avoid
 * linker errors) and 'NB_NOINLINE' (to avoid them being inlined into every
 * single shared_ptr type_caster, which would enlarge the binding size)
 */
inline NB_NOINLINE std::shared_ptr<void>
shared_from_python(void *ptr, handle h) noexcept {
    struct py_deleter {
        void operator()(void *) noexcept {
            gil_scoped_acquire guard;
            Py_DECREF(o);
        }

        PyObject *o;
    };

    if (ptr)
        return std::shared_ptr<void>(ptr, py_deleter{ h.inc_ref().ptr() });
    else
        return std::shared_ptr<void>((PyObject *) nullptr);
}

inline NB_NOINLINE void shared_from_cpp(std::shared_ptr<void> &&ptr,
                                        PyObject *o) noexcept {
    keep_alive(o, new std::shared_ptr<void>(std::move(ptr)),
               [](void *p) noexcept { delete (std::shared_ptr<void> *) p; });
}

template <class T, class = void>
struct uses_shared_from_this : std::false_type { };

template <class T>
struct uses_shared_from_this<
    T, std::void_t<decltype(std::declval<T>().shared_from_this())>>
    : std::true_type { };

template <typename T> struct type_caster<std::shared_ptr<T>> {
    using Value = std::shared_ptr<T>;
    using Caster = make_caster<T>;
    static_assert(Caster::IsClass,
                  "Binding 'shared_ptr<T>' requires that 'T' can also be bound "
                  "by nanobind. It appears that you specified a type which "
                  "would undergo conversion/copying, which is not allowed.");
    static_assert(
        !uses_shared_from_this<T>::value,
        "nanobind does not permit use of std::shared_from_this, which can "
        "cause undefined behavior. (Refer to docs/ownership.md for details.)");

    static constexpr auto Name = Caster::Name;
    static constexpr bool IsClass = true;

    template <typename T_> using Cast = movable_cast_t<T_>;

    Value value;

    bool from_python(handle src, uint8_t flags,
                     cleanup_list *cleanup) noexcept {
        Caster caster;
        if (!caster.from_python(src, flags, cleanup))
            return false;

        value = std::static_pointer_cast<T>(
            shared_from_python(caster.operator T *(), src));

        return true;
    }

    static handle from_cpp(const Value *value, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        if (!value)
            return (PyObject *) nullptr;
        return from_cpp(*value, policy, cleanup);
    }

    static handle from_cpp(const Value &value, rv_policy,
                           cleanup_list *cleanup) noexcept {
        bool is_new = false;
        handle result = nb_type_put(&typeid(T), value.get(),
                                    rv_policy::reference, cleanup, &is_new);

        if (is_new)
            shared_from_cpp(std::static_pointer_cast<void>(value),
                            result.ptr());

        return result;
    }

    explicit operator Value *() { return &value; }
    explicit operator Value &() { return value; }
    explicit operator Value &&() && { return (Value &&) value; }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
