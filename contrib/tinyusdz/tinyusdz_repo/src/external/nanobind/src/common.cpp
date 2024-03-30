/*
    src/common.cpp: miscellaneous libnanobind functionality

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include <nanobind/nanobind.h>
#include "nb_internals.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)


#if defined(__GNUC__)
    __attribute__((noreturn, __format__ (__printf__, 1, 2)))
#else
    [[noreturn]]
#endif
void raise(const char *fmt, ...) {
    char buf[512];
    va_list args;

    va_start(args, fmt);
    size_t size = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (size < sizeof(buf))
        throw std::runtime_error(buf);

    scoped_pymalloc<char> temp(size + 1);

    va_start(args, fmt);
    vsnprintf(temp.get(), size + 1, fmt, args);
    va_end(args);

    throw std::runtime_error(temp.get());
}

/// Abort the process with a fatal error
#if defined(__GNUC__)
    __attribute__((noreturn, __format__ (__printf__, 1, 2)))
#else
    [[noreturn]]
#endif
void fail(const char *fmt, ...) noexcept {
    va_list args;
    fprintf(stderr, "Critical nanobind error: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    abort();
}

PyObject *capsule_new(const void *ptr, void (*free)(void *) noexcept) noexcept {
    auto capsule_free = [](PyObject *o) {
        auto free_2 = (void (*)(void *))(PyCapsule_GetContext(o));
        if (free_2)
            free_2(PyCapsule_GetPointer(o, nullptr));
    };

    PyObject *c = PyCapsule_New((void *) ptr, nullptr, capsule_free);

    if (!c)
        fail("nanobind::detail::capsule_new(): allocation failed!");

    if (PyCapsule_SetContext(c, (void *) free) != 0)
        fail("nanobind::detail::capsule_new(): could not set context!");

    return c;
}

void raise_python_error() {
    if (PyErr_Occurred())
        throw python_error();
    else
        fail("nanobind::detail::raise_python_error() called without "
             "an error condition!");
}

void raise_next_overload() {
    throw next_overload();
}

// ========================================================================

void cleanup_list::release() noexcept {
    /* Don't decrease the reference count of the first
       element, it stores the 'self' element. */
    for (size_t i = 1; i < m_size; ++i)
        Py_DECREF(m_data[i]);
    if (m_capacity != Small)
        free(m_data);
    m_data = nullptr;
}

void cleanup_list::expand() noexcept {
    uint32_t new_capacity = m_capacity * 2;
    PyObject **new_data = (PyObject **) malloc(new_capacity * sizeof(PyObject *));
    if (!new_data)
        fail("nanobind::detail::cleanup_list::expand(): out of memory!");
    memcpy(new_data, m_data, m_size * sizeof(PyObject *));
    if (m_capacity != Small)
        free(m_data);
    m_data = new_data;
    m_capacity = new_capacity;
}

// ========================================================================

PyObject *module_new(const char *name, PyModuleDef *def) noexcept {
    memset(def, 0, sizeof(PyModuleDef));
    def->m_name = name;
    def->m_size = -1;
    PyObject *m = PyModule_Create(def);
    if (!m)
        fail("nanobind::detail::module_new(): allocation failed!");
    return m;
}

PyObject *module_import(const char *name) {
    PyObject *res = PyImport_ImportModule(name);
    if (!res)
        throw python_error();
    return res;
}

PyObject *module_new_submodule(PyObject *base, const char *name,
                               const char *doc) noexcept {

    PyObject *base_name = PyModule_GetNameObject(base),
             *name_py, *res;
    if (!base_name)
        goto fail;

    name_py = PyUnicode_FromFormat("%U.%s", base_name, name);
    if (!name_py)
        goto fail;

    res = PyImport_AddModuleObject(name_py);
    if (doc) {
        PyObject *doc_py = PyUnicode_FromString(doc);
        if (!doc_py || PyObject_SetAttrString(res, "__doc__", doc_py))
            goto fail;
        Py_DECREF(doc_py);
    }
    Py_DECREF(name_py);
    Py_DECREF(base_name);

    Py_INCREF(res);
    if (PyModule_AddObject(base, name, res))
        goto fail;

    return res;

fail:
    fail("nanobind::detail::module_new_submodule(): failed.");
}

// ========================================================================

size_t obj_len(PyObject *o) {
    Py_ssize_t res = PyObject_Length(o);
    if (res < 0)
        raise_python_error();
    return (size_t) res;
}

PyObject *obj_repr(PyObject *o) {
    PyObject *res = PyObject_Repr(o);
    if (!res)
        raise_python_error();
    return res;
}

bool obj_comp(PyObject *a, PyObject *b, int value) {
    int rv = PyObject_RichCompareBool(a, b, value);
    if (rv == -1)
        raise_python_error();
    return rv == 1;
}

PyObject *obj_op_1(PyObject *a, PyObject* (*op)(PyObject*)) {
    PyObject *res = op(a);
    if (!res)
        raise_python_error();
    return res;
}

PyObject *obj_op_2(PyObject *a, PyObject *b,
                   PyObject *(*op)(PyObject *, PyObject *) ) {
    PyObject *res = op(a, b);
    if (!res)
        raise_python_error();

    return res;
}

#if defined(Py_LIMITED_API)
PyObject *_PyObject_Vectorcall(PyObject *base, PyObject *const *args,
                               size_t nargsf, PyObject *kwnames) {
    size_t nargs = NB_VECTORCALL_NARGS(nargsf);

    PyObject *args_tuple = PyTuple_New(nargs);
    if (!args_tuple)
        return nullptr;

    for (size_t i = 0; i < nargs; ++i) {
        Py_INCREF(args[i]);
        NB_TUPLE_SET_ITEM(args_tuple, i, args[i]);
    }

    PyObject *kwargs = nullptr;
    if (kwnames) {
        kwargs = PyDict_New();
        if (!kwargs) {
            Py_DECREF(args_tuple);
            return nullptr;
        }

        for (size_t i = 0, l = NB_TUPLE_GET_SIZE(kwnames); i < l; ++i) {
            PyObject *k = NB_TUPLE_GET_ITEM(kwnames, i),
                     *v = args[i + nargs];
            if (PyDict_SetItem(kwargs, k, v)) {
                Py_DECREF(kwargs);
                Py_DECREF(args_tuple);
                return nullptr;
            }

        }
    }

    PyObject *res = PyObject_Call(base, args_tuple, kwargs);

    Py_DECREF(args_tuple);
    Py_XDECREF(kwargs);

    return res;
}
#endif


PyObject *obj_vectorcall(PyObject *base, PyObject *const *args, size_t nargsf,
                         PyObject *kwnames, bool method_call) {
    const char *error = nullptr;
    PyObject *res = nullptr;

    size_t nargs_total =
        NB_VECTORCALL_NARGS(nargsf) + (kwnames ? NB_TUPLE_GET_SIZE(kwnames) : 0);

#if !defined(Py_LIMITED_API)
    if (!PyGILState_Check()) {
        error = "nanobind::detail::obj_vectorcall(): PyGILState_Check() failure." ;
        goto end;
    }
#endif

    for (size_t i = 0; i < nargs_total; ++i) {
        if (!args[i]) {
            error = "nanobind::detail::obj_vectorcall(): argument conversion failure." ;
            goto end;
        }
    }

#if PY_VERSION_HEX < 0x03090000 || defined(Py_LIMITED_API)
    if (method_call) {
        PyObject *self = PyObject_GetAttr(args[0], /* name = */ base);
        if (self) {
            res = _PyObject_Vectorcall(self, args + 1, nargsf - 1, kwnames);
            Py_DECREF(self);
        }
    } else {
        res = _PyObject_Vectorcall(base, args, nargsf, kwnames);
    }
#else
    res = (method_call ? PyObject_VectorcallMethod
                       : PyObject_Vectorcall)(base, args, nargsf, kwnames);
#endif

end:
    for (size_t i = 0; i < nargs_total; ++i)
        Py_XDECREF(args[i]);
    Py_XDECREF(kwnames);
    Py_DECREF(base);

    if (error)
        raise("%s", error);
    else if (!res)
        raise_python_error();

    return res;
}


PyObject *obj_iter(PyObject *o) {
    PyObject *result = PyObject_GetIter(o);
    if (!result)
        raise_python_error();
    return result;
}

PyObject *obj_iter_next(PyObject *o) {
    PyObject *result = PyIter_Next(o);
    if (!result && PyErr_Occurred())
        raise_python_error();
    return result;
}


// ========================================================================

PyObject *getattr(PyObject *obj, const char *key) {
    PyObject *res = PyObject_GetAttrString(obj, key);
    if (!res)
        raise_python_error();
    return res;
}

PyObject *getattr(PyObject *obj, PyObject *key) {
    PyObject *res = PyObject_GetAttr(obj, key);
    if (!res)
        raise_python_error();
    return res;
}

PyObject *getattr(PyObject *obj, const char *key, PyObject *def) noexcept {
    PyObject *res = PyObject_GetAttrString(obj, key);
    if (res)
        return res;
    PyErr_Clear();
    Py_XINCREF(def);
    return def;
}

PyObject *getattr(PyObject *obj, PyObject *key, PyObject *def) noexcept {
    PyObject *res = PyObject_GetAttr(obj, key);
    if (res)
        return res;
    PyErr_Clear();
    Py_XINCREF(def);
    return def;
}

void getattr_maybe(PyObject *obj, const char *key, PyObject **out) {
    if (*out)
        return;

    PyObject *res = PyObject_GetAttrString(obj, key);
    if (!res)
        raise_python_error();

    *out = res;
}

void getattr_maybe(PyObject *obj, PyObject *key, PyObject **out) {
    if (*out)
        return;

    PyObject *res = PyObject_GetAttr(obj, key);
    if (!res)
        raise_python_error();

    *out = res;
}

void setattr(PyObject *obj, const char *key, PyObject *value) {
    int rv = PyObject_SetAttrString(obj, key, value);
    if (rv)
        raise_python_error();
}

void setattr(PyObject *obj, PyObject *key, PyObject *value) {
    int rv = PyObject_SetAttr(obj, key, value);
    if (rv)
        raise_python_error();
}

// ========================================================================

void getitem_maybe(PyObject *obj, Py_ssize_t key, PyObject **out) {
    if (*out)
        return;

    PyObject *res = PySequence_GetItem(obj, key);
    if (!res)
        raise_python_error();

    *out = res;
}

void getitem_maybe(PyObject *obj, const char *key_, PyObject **out) {
    if (*out)
        return;

    PyObject *key, *res;

    key = PyUnicode_FromString(key_);
    if (!key)
        raise_python_error();

    res = PyObject_GetItem(obj, key);
    Py_DECREF(key);

    if (!res)
        raise_python_error();

    *out = res;
}

void getitem_maybe(PyObject *obj, PyObject *key, PyObject **out) {
    if (*out)
        return;

    PyObject *res = PyObject_GetItem(obj, key);
    if (!res)
        raise_python_error();

    *out = res;
}

void setitem(PyObject *obj, Py_ssize_t key, PyObject *value) {
    int rv = PySequence_SetItem(obj, key, value);
    if (rv)
        raise_python_error();
}

void setitem(PyObject *obj, const char *key_, PyObject *value) {
    PyObject *key = PyUnicode_FromString(key_);
    if (!key)
        raise_python_error();

    int rv = PyObject_SetItem(obj, key, value);
    Py_DECREF(key);

    if (rv)
        raise_python_error();
}

void setitem(PyObject *obj, PyObject *key, PyObject *value) {
    int rv = PyObject_SetItem(obj, key, value);
    if (rv)
        raise_python_error();
}

// ========================================================================

PyObject *str_from_obj(PyObject *o) {
    PyObject *result = PyObject_Str(o);
    if (!result)
        raise_python_error();
    return result;
}

PyObject *str_from_cstr(const char *str) {
    PyObject *result = PyUnicode_FromString(str);
    if (!result)
        raise("nanobind::detail::str_from_cstr(): conversion error!");
    return result;
}

PyObject *str_from_cstr_and_size(const char *str, size_t size) {
    PyObject *result = PyUnicode_FromStringAndSize(str, (Py_ssize_t) size);
    if (!result)
        raise("nanobind::detail::str_from_cstr_and_size(): conversion error!");
    return result;
}

// ========================================================================

PyObject **seq_get(PyObject *seq, size_t *size_out, PyObject **temp_out) noexcept {
    PyObject *temp = nullptr;
    size_t size = 0;
    PyObject **result = nullptr;

    /* This function is used during overload resolution; if anything
       goes wrong, it fails gracefully without reporting errors. Other
       overloads will then be tried. */

#if !defined(Py_LIMITED_API)
    if (PyTuple_CheckExact(seq)) {
        size = (size_t) PyTuple_GET_SIZE(seq);
        result = ((PyTupleObject *) seq)->ob_item;
        /* Special case for zero-sized lists/tuples. CPython
           sets ob_item to NULL, which this function incidentally uses to
           signal an error. Return a nonzero pointer that will, however,
           still trigger a segfault if dereferenced. */
        if (size == 0)
            result = (PyObject **) 1;
    } else if (PyList_CheckExact(seq)) {
        size = (size_t) PyList_GET_SIZE(seq);
        result = ((PyListObject *) seq)->ob_item;
        if (size == 0) // ditto
            result = (PyObject **) 1;
    } else if (PySequence_Check(seq)) {
        temp = PySequence_Fast(seq, "");

        if (temp)
            result = seq_get(temp, &size, temp_out);
        else
            PyErr_Clear();
    }
#else
    /* There isn't a nice way to get a PyObject** in Py_LIMITED_API. This
       is going to be slow, but hopefully also very future-proof.. */
    if (PySequence_Check(seq)) {
        Py_ssize_t size_seq = PySequence_Length(seq);

        if (size_seq >= 0) {
            result = (PyObject **) PyObject_Malloc(sizeof(PyObject *) *
                                                   (size_seq + 1));

            if (result) {
                result[size_seq] = nullptr;

                for (Py_ssize_t i = 0; i < size_seq; ++i) {
                    PyObject *o = PySequence_GetItem(seq, i);

                    if (o) {
                        result[i] = o;
                    } else {
                        for (Py_ssize_t j = 0; j < i; ++j)
                            Py_DECREF(result[j]);

                        PyObject_Free(result);
                        result = nullptr;
                        break;
                    }
                }
            }

            if (result) {
                temp = PyCapsule_New(result, nullptr, [](PyObject *o) {
                    PyObject **ptr = (PyObject **) PyCapsule_GetPointer(o, nullptr);
                    for (size_t i = 0; ptr[i] != nullptr; ++i)
                        Py_DECREF(ptr[i]);
                    PyObject_Free(ptr);
                });

                if (temp) {
                    size = (size_t) size_seq;
                } else if (!temp) {
                    PyErr_Clear();
                    for (Py_ssize_t i = 0; i < size_seq; ++i)
                        Py_DECREF(result[i]);

                    PyObject_Free(result);
                    result = nullptr;
                }
            }
        } else if (size_seq < 0) {
            PyErr_Clear();
        }
    }
#endif

    *temp_out = temp;
    *size_out = size;
    return result;
}


PyObject **seq_get_with_size(PyObject *seq, size_t size,
                             PyObject **temp_out) noexcept {

    /* This function is used during overload resolution; if anything
       goes wrong, it fails gracefully without reporting errors. Other
       overloads will then be tried. */

    PyObject *temp = nullptr,
             **result = nullptr;

#if !defined(Py_LIMITED_API)
    if (PyTuple_CheckExact(seq)) {
        if (size == (size_t) PyTuple_GET_SIZE(seq)) {
            result = ((PyTupleObject *) seq)->ob_item;
            /* Special case for zero-sized lists/tuples. CPython
               sets ob_item to NULL, which this function incidentally uses to
               signal an error. Return a nonzero pointer that will, however,
               still trigger a segfault if dereferenced. */
            if (size == 0)
                result = (PyObject **) 1;
        }
    } else if (PyList_CheckExact(seq)) {
        if (size == (size_t) PyList_GET_SIZE(seq)) {
            result = ((PyListObject *) seq)->ob_item;
            if (size == 0) // ditto
                result = (PyObject **) 1;
        }
    } else if (PySequence_Check(seq)) {
        temp = PySequence_Fast(seq, "");

        if (temp)
            result = seq_get_with_size(temp, size, temp_out);
        else
            PyErr_Clear();
    }
#else
    /* There isn't a nice way to get a PyObject** in Py_LIMITED_API. This
       is going to be slow, but hopefully also very future-proof.. */
    if (PySequence_Check(seq)) {
        Py_ssize_t size_seq = PySequence_Length(seq);

        if (size == (size_t) size_seq) {
            result =
                (PyObject **) PyObject_Malloc(sizeof(PyObject *) * (size + 1));

            if (result) {
                result[size] = nullptr;

                for (Py_ssize_t i = 0; i < size_seq; ++i) {
                    PyObject *o = PySequence_GetItem(seq, i);

                    if (o) {
                        result[i] = o;
                    } else {
                        for (Py_ssize_t j = 0; j < i; ++j)
                            Py_DECREF(result[j]);

                        PyObject_Free(result);
                        result = nullptr;
                        break;
                    }
                }
            }

            if (result) {
                temp = PyCapsule_New(result, nullptr, [](PyObject *o) {
                    PyObject **ptr = (PyObject **) PyCapsule_GetPointer(o, nullptr);
                    for (size_t i = 0; ptr[i] != nullptr; ++i)
                        Py_DECREF(ptr[i]);
                    PyObject_Free(ptr);
                });

                if (!temp) {
                    PyErr_Clear();
                    for (Py_ssize_t i = 0; i < size_seq; ++i)
                        Py_DECREF(result[i]);

                    PyObject_Free(result);
                    result = nullptr;
                }
            }
        } else if (size_seq < 0) {
            PyErr_Clear();
        }
    }
#endif

    *temp_out = temp;
    return result;
}

// ========================================================================

void property_install(PyObject *scope, const char *name, bool is_static,
                      PyObject *getter, PyObject *setter) noexcept {
    const nb_internals &internals = internals_get();
    handle property = (PyObject *) (is_static ? internals.nb_static_property
                                              : &PyProperty_Type);
    (void) is_static;
    PyObject *m = getter ? getter : setter;
    object doc = none();

    if (m && (Py_TYPE(m) == internals.nb_func ||
              Py_TYPE(m) == internals.nb_method)) {
        func_data *f = nb_func_data(m);
        if (f->flags & (uint32_t) func_flags::has_doc)
            doc = str(f->doc);
    }

    handle(scope).attr(name) = property(
        getter ? handle(getter) : handle(Py_None),
        setter ? handle(setter) : handle(Py_None),
        handle(Py_None), // deleter
        doc
    );
}

// ========================================================================

void tuple_check(PyObject *tuple, size_t nargs) {
    for (size_t i = 0; i < nargs; ++i) {
        if (!NB_TUPLE_GET_ITEM(tuple, i))
            raise("nanobind::detail::tuple_check(...): conversion of argument "
                  "%zu failed!", i + 1);
    }
}

// ========================================================================

void print(PyObject *value, PyObject *end, PyObject *file) {
    if (!file)
        file = PySys_GetObject("stdout");

    int rv = PyFile_WriteObject(value, file, Py_PRINT_RAW);
    if (rv)
        raise_python_error();

    if (end)
        rv = PyFile_WriteObject(end, file, Py_PRINT_RAW);
    else
        rv = PyFile_WriteString("\n", file);

    if (rv)
        raise_python_error();
}

// ========================================================================

std::pair<double, bool> load_f64(PyObject *o, uint8_t flags) noexcept {
    const bool convert = flags & (uint8_t) cast_flags::convert;

    if (convert || PyFloat_Check(o)) {
        double result = PyFloat_AsDouble(o);

        if (result != -1.0 || !PyErr_Occurred())
            return { result, true };
        else
            PyErr_Clear();
    }

    return { 0.0, false };
}

std::pair<float, bool> load_f32(PyObject *o, uint8_t flags) noexcept {
    const bool convert = flags & (uint8_t) cast_flags::convert;
    if (convert || PyFloat_Check(o)) {
        double result = PyFloat_AsDouble(o);

        if (result != -1.0 || !PyErr_Occurred())
            return { (float) result, true };
        else
            PyErr_Clear();
    }

    return { 0.f, false };
}

template <typename T>
NB_INLINE std::pair<T, bool> load_int(PyObject *o, uint32_t flags) noexcept {
    using T0 = std::conditional_t<sizeof(T) <= sizeof(long), long, long long>;
    using Tp = std::conditional_t<std::is_signed_v<T>, T0, std::make_unsigned_t<T0>>;

    const bool convert = flags & (uint8_t) cast_flags::convert;
    object temp;

    if (!PyLong_Check(o)) {
        if (convert) {
            if constexpr (std::is_unsigned_v<T>) {
                // Unsigned PyLong_* conversions don't call __index__()..
                temp = steal(PyNumber_Long(o));
                if (!temp.is_valid()) {
                    PyErr_Clear();
                    return { T(0), false };
                }

                o = temp.ptr();
            }
        } else {
            return { T(0), false };
        }
    }

    Tp value_p;
    if constexpr (std::is_unsigned_v<Tp>) {
        value_p = sizeof(T) <= sizeof(long) ? (Tp) PyLong_AsUnsignedLong(o)
                                            : (Tp) PyLong_AsUnsignedLongLong(o);
    } else {
        value_p = sizeof(T) <= sizeof(long) ? (Tp) PyLong_AsLong(o)
                                            : (Tp) PyLong_AsLongLong(o);
    }

    if (value_p == Tp(-1) && PyErr_Occurred()) {
        PyErr_Clear();
        return { T(0), false };
    }

    T value = (T) value_p;
    if constexpr (sizeof(Tp) != sizeof(T)) {
        if (value_p != (Tp) value)
            return { T(0), false };
    }
    return { value, true };
}

std::pair<uint8_t, bool> load_u8(PyObject *o, uint8_t flags) noexcept {
    return load_int<uint8_t>(o, flags);
}

std::pair<int8_t, bool> load_i8(PyObject *o, uint8_t flags) noexcept {
    return load_int<int8_t>(o, flags);
}

std::pair<uint16_t, bool> load_u16(PyObject *o, uint8_t flags) noexcept {
    return load_int<uint16_t>(o, flags);
}

std::pair<int16_t, bool> load_i16(PyObject *o, uint8_t flags) noexcept {
    return load_int<int16_t>(o, flags);
}

std::pair<uint32_t, bool> load_u32(PyObject *o, uint8_t flags) noexcept {
    return load_int<uint32_t>(o, flags);
}

std::pair<int32_t, bool> load_i32(PyObject *o, uint8_t flags) noexcept {
    return load_int<int32_t>(o, flags);
}

std::pair<uint64_t, bool> load_u64(PyObject *o, uint8_t flags) noexcept {
    return load_int<uint64_t>(o, flags);
}

std::pair<int64_t, bool> load_i64(PyObject *o, uint8_t flags) noexcept {
    return load_int<int64_t>(o, flags);
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
