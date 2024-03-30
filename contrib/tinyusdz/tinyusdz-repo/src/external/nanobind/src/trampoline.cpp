/*
    src/trampoline.cpp: support for overriding virtual functions in Python

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include <nanobind/trampoline.h>
#include "nb_internals.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

void trampoline_new(void **data, size_t size, void *ptr,
                    const std::type_info *cpp_type) noexcept {
    // GIL is held when the trampoline constructor runs
    nb_internals &internals = internals_get();
    auto it = internals.inst_c2p.find(
        std::pair<void *, std::type_index>(ptr, *cpp_type));
    if (it == internals.inst_c2p.end())
        fail("nanobind::detail::trampoline_new(): instance not found!");

    data[0] = it->second;
    memset(data + 1, 0, sizeof(void *) * 2 * size);
}

void trampoline_release(void **data, size_t size) noexcept {
    // GIL is held when the trampoline destructor runs
    for (size_t i = 0; i < size; ++i)
        Py_XDECREF((PyObject *) data[i*2 + 2]);
}

PyObject *trampoline_lookup(void **data, size_t size, const char *name,
                            bool pure) {
    const PyObject *None = Py_None;

    current_method cm = current_method_data;
    if (cm.self == data[0] && (cm.name == name || strcmp(cm.name, name) == 0))
        return nullptr;

    // First quick sweep without lock
    for (size_t i = 0; i < size; i++) {
        void *d_name  = data[2*i + 1],
             *d_value = data[2*i + 2];
        if (name == d_name && d_value)
            return d_value != None ? (PyObject *) d_value : nullptr;
    }

    PyGILState_STATE state = PyGILState_Ensure();

    // Nothing found -- retry, now with lock held
    for (size_t i = 0; i < size; i++) {
        void *d_name  = data[2*i + 1],
             *d_value = data[2*i + 2];
        if (name == d_name && d_value) {
            PyGILState_Release(state);
            return d_value != None ? (PyObject *) d_value : nullptr;
        }
    }

    // Sill no luck -- perform a lookup and populate the trampoline
    const char *error = nullptr;
    PyTypeObject *value_tp = nullptr;

    size_t offset = 0;
    for (; offset < size; offset++) {
        if (data[2 * offset + 1] == nullptr &&
            data[2 * offset + 2] == nullptr)
            break;
    }

    nb_internals &internals = internals_get();
    PyObject *key = nullptr, *value = nullptr;

    if (offset == size) {
        error = "the trampoline ran out of slots (you will need to increase "
                "the value provided to the NB_TRAMPOLINE() macro)";
        goto fail;
    }

    key = PyUnicode_InternFromString(name);
    if (!key) {
        error = "could not intern string";
        goto fail;
    }

    value = PyObject_GetAttr((PyObject *) data[0], key);
    if (!value) {
        error = "lookup failed";
        goto fail;
    }

    value_tp = Py_TYPE(value);
    Py_CLEAR(value);

    if (value_tp == internals.nb_func || value_tp == internals.nb_method ||
        value_tp == internals.nb_bound_method) {
        if (pure) {
            error = "tried to call a pure virtual function";
            goto fail;
        }

        Py_DECREF(key);
        key = Py_None;
        Py_INCREF(Py_None);
    }

    data[2 * offset + 1] = (void *) name;
    data[2 * offset + 2] = key;

    PyGILState_Release(state);
    return key != None ? (PyObject *) key : nullptr;

fail:
    type_data *t = nb_type_data(Py_TYPE((PyObject *) data[0]));
    PyGILState_Release(state);

    raise("nanobind::detail::get_trampoline('%s::%s()'): %s!",
          t->name, name, error);
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
