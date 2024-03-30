/*
    src/implicit.cpp: functions for registering implicit conversions

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include <nanobind/trampoline.h>
#include "nb_internals.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

void implicitly_convertible(const std::type_info *src,
                            const std::type_info *dst) noexcept {
    nb_internals &internals = internals_get();

    auto it = internals.type_c2p.find(std::type_index(*dst));
    if (it == internals.type_c2p.end())
        fail("nanobind::detail::implicitly_convertible(src=%s, dst=%s): "
             "destination type unknown!", type_name(src), type_name(dst));

    type_data *t = it->second;
    size_t size = 0;

    if (t->flags & (uint32_t) type_flags::has_implicit_conversions) {
        while (t->implicit && t->implicit[size])
            size++;
    } else {
        t->implicit = nullptr;
        t->implicit_py = nullptr;
        t->flags |= (uint32_t) type_flags::has_implicit_conversions;
    }

    void **data = (void **) malloc(sizeof(void *) * (size + 2));

    memcpy(data, t->implicit, size * sizeof(void *));
    data[size] = (void *) src;
    data[size + 1] = nullptr;
    free(t->implicit);
    t->implicit = (decltype(t->implicit)) data;
}

void implicitly_convertible(bool (*predicate)(PyTypeObject *, PyObject *,
                                              cleanup_list *),
                            const std::type_info *dst) noexcept {
    nb_internals &internals = internals_get();

    auto it = internals.type_c2p.find(std::type_index(*dst));
    if (it == internals.type_c2p.end())
        fail("nanobind::detail::implicitly_convertible(src=<predicate>, dst=%s): "
             "destination type unknown!", type_name(dst));

    type_data *t = it->second;
    size_t size = 0;

    if (t->flags & (uint32_t) type_flags::has_implicit_conversions) {
        while (t->implicit_py && t->implicit_py[size])
            size++;
    } else {
        t->implicit = nullptr;
        t->implicit_py = nullptr;
        t->flags |= (uint32_t) type_flags::has_implicit_conversions;
    }

    void **data = (void **) malloc(sizeof(void *) * (size + 2));
    memcpy(data, t->implicit_py, size * sizeof(void *));
    data[size] = (void *) predicate;
    data[size + 1] = nullptr;
    free(t->implicit_py);
    t->implicit_py = (decltype(t->implicit_py)) data;
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
