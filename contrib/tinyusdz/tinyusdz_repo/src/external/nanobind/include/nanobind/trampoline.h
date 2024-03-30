/*
    nanobind/trampoline.h: functionality for overriding C++ virtual
    functions from within Python

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

NB_CORE void trampoline_new(void **data, size_t size, void *ptr,
                            const std::type_info *cpp_type) noexcept;
NB_CORE void trampoline_release(void **data, size_t size) noexcept;

NB_CORE PyObject *trampoline_lookup(void **data, size_t size, const char *name,
                                    bool pure);

template <size_t Size> struct trampoline {
    mutable void *data[2 * Size + 1];

    NB_INLINE trampoline(void *ptr, const std::type_info *cpp_type) {
        trampoline_new(data, Size, ptr, cpp_type);
    }

    NB_INLINE ~trampoline() { trampoline_release(data, Size); }

    NB_INLINE handle lookup(const char *name, bool pure) const {
        return trampoline_lookup(data, Size, name, pure);
    }

    NB_INLINE handle base() const { return (PyObject *) data[0]; }
};

#define NB_TRAMPOLINE(base, size)                                              \
    nanobind::detail::trampoline<size> trampoline{ this, &typeid(base) };

#define NB_OVERRIDE_NAME(ret_type, base_type, name, func, ...)                 \
    nanobind::handle key = trampoline.lookup(name, false);                     \
    if (key.is_valid()) {                                                      \
        nanobind::gil_scoped_acquire guard;                                    \
        return nanobind::cast<ret_type>(                                       \
            trampoline.base().attr(key)(__VA_ARGS__));                         \
    } else {                                                                   \
        return base_type::func(__VA_ARGS__);                                   \
    }

#define NB_OVERRIDE_PURE_NAME(ret_type, base_type, name, func, ...)            \
    nanobind::handle key = trampoline.lookup(name, true);                      \
    nanobind::gil_scoped_acquire guard;                                        \
    return nanobind::cast<ret_type>(trampoline.base().attr(key)(__VA_ARGS__));

#define NB_OVERRIDE(ret_type, base_type, func, ...)                            \
    NB_OVERRIDE_NAME(ret_type, base_type, #func, func, __VA_ARGS__)

#define NB_OVERRIDE_PURE(ret_type, base_type, func, ...)                       \
    NB_OVERRIDE_PURE_NAME(ret_type, base_type, #func, func, __VA_ARGS__)

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
