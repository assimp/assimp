/*
    src/nb_enum.cpp: nanobind enumeration type

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include <nanobind/nanobind.h>
#include "nb_internals.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

static PyObject *nb_enum_int(PyObject *o);

/// Map to unique representative enum instance, returns a borrowed reference
static PyObject *nb_enum_lookup(PyObject *self) {
    PyObject *int_val = nb_enum_int(self),
             *dict    = PyObject_GetAttrString((PyObject *) Py_TYPE(self), "__entries");

    PyObject *rec = nullptr;
    if (int_val && dict)
        rec = (PyObject *) PyDict_GetItem(dict, int_val);

    Py_XDECREF(int_val);
    Py_XDECREF(dict);

    if (rec && PyTuple_CheckExact(rec) && NB_TUPLE_GET_SIZE(rec) == 3) {
        return rec;
    } else {
        PyErr_Clear();
        PyErr_SetString(PyExc_RuntimeError, "nb_enum: could not find entry!");
        return nullptr;
    }
}

static PyObject *nb_enum_repr(PyObject *self) {
    PyObject *entry = nb_enum_lookup(self);
    if (!entry)
        return nullptr;

    PyObject *name = nb_inst_name(self);
    PyObject *result =
        PyUnicode_FromFormat("%U.%U", name, NB_TUPLE_GET_ITEM(entry, 0));
    Py_DECREF(name);

    return result;
}

static PyObject *nb_enum_get_name(PyObject *self, void *) {
    PyObject *entry = nb_enum_lookup(self);
    if (!entry)
        return nullptr;

    PyObject *result = NB_TUPLE_GET_ITEM(entry, 0);
    Py_INCREF(result);
    return result;
}

static PyObject *nb_enum_get_doc(PyObject *self, void *) {
    PyObject *entry = nb_enum_lookup(self);
    if (!entry)
        return nullptr;

    PyObject *result = NB_TUPLE_GET_ITEM(entry, 1);
    Py_INCREF(result);
    return result;
}

static PyObject *nb_enum_int(PyObject *o) {
    type_data *t = nb_type_data(Py_TYPE(o));

    const void *p = inst_ptr((nb_inst *) o);
    if (t->flags & (uint32_t) type_flags::is_unsigned_enum) {
        unsigned long long value;
        switch (t->size) {
            case 1: value = (unsigned long long) *(const uint8_t *)  p; break;
            case 2: value = (unsigned long long) *(const uint16_t *) p; break;
            case 4: value = (unsigned long long) *(const uint32_t *) p; break;
            case 8: value = (unsigned long long) *(const uint64_t *) p; break;
            default: PyErr_SetString(PyExc_TypeError, "nb_enum: invalid type size!");
                     return nullptr;
        }
        return PyLong_FromUnsignedLongLong(value);
    } else if (t->flags & (uint32_t) type_flags::is_signed_enum) {
        long long value;
        switch (t->size) {
            case 1: value = (long long) *(const int8_t *)  p; break;
            case 2: value = (long long) *(const int16_t *) p; break;
            case 4: value = (long long) *(const int32_t *) p; break;
            case 8: value = (long long) *(const int64_t *) p; break;
            default: PyErr_SetString(PyExc_TypeError, "nb_enum: invalid type size!");
                     return nullptr;
        }
        return PyLong_FromLongLong(value);
    } else {
        PyErr_SetString(PyExc_TypeError, "nb_enum: input is not an enumeration!");
        return nullptr;
    }
}

static PyObject *nb_enum_init(PyTypeObject *subtype, PyObject *args, PyObject *kwds) {
    PyObject *arg;

    if (kwds || NB_TUPLE_GET_SIZE(args) != 1)
        goto error;

    arg = NB_TUPLE_GET_ITEM(args, 0);
    if (PyLong_Check(arg)) {
        PyObject *entries =
            PyObject_GetAttrString((PyObject *) subtype, "__entries");
        if (!entries)
            goto error;

        PyObject *item = PyDict_GetItem(entries, arg);
        Py_DECREF(entries);

        if (item && PyTuple_CheckExact(item) && NB_TUPLE_GET_SIZE(item) == 3) {
            item = NB_TUPLE_GET_ITEM(item, 2);
            Py_INCREF(item);
            return item;
        }
    } else if (Py_TYPE(arg) == subtype) {
        Py_INCREF(arg);
        return arg;
    }

error:
    PyErr_Clear();
    PyErr_Format(PyExc_RuntimeError,
                 "%s(): could not convert the input into an enumeration value!",
                 nb_type_data(subtype)->name);
    return nullptr;
}

static PyGetSetDef nb_enum_getset[] = {
    { "__doc__", nb_enum_get_doc, nullptr, nullptr, nullptr },
    { "__name__", nb_enum_get_name, nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr, nullptr, nullptr }
};

PyObject *nb_enum_richcompare(PyObject *a, PyObject *b, int op) {
    PyObject *ia = PyNumber_Long(a);
    PyObject *ib = PyNumber_Long(b);
    if (!ia || !ib)
        return nullptr;
    PyObject *result = PyObject_RichCompare(ia, ib, op);
    Py_DECREF(ia);
    Py_DECREF(ib);
    return result;
}

#define NB_ENUM_UNOP(name, op)                                                 \
    PyObject *nb_enum_##name(PyObject *a) {                                    \
        PyObject *ia = PyNumber_Long(a);                                       \
        if (!ia)                                                               \
            return nullptr;                                                    \
        PyObject *result = op(ia);                                             \
        Py_DECREF(ia);                                                         \
        return result;                                                         \
    }

#define NB_ENUM_BINOP(name, op)                                                \
    PyObject *nb_enum_##name(PyObject *a, PyObject *b) {                       \
        PyObject *ia = PyNumber_Long(a);                                       \
        PyObject *ib = PyNumber_Long(b);                                       \
        if (!ia || !ib)                                                        \
            return nullptr;                                                    \
        PyObject *result = op(ia, ib);                                         \
        Py_DECREF(ia);                                                         \
        Py_DECREF(ib);                                                         \
        return result;                                                         \
    }

NB_ENUM_BINOP(add, PyNumber_Add)
NB_ENUM_BINOP(sub, PyNumber_Subtract)
NB_ENUM_BINOP(mul, PyNumber_Multiply)
NB_ENUM_BINOP(div, PyNumber_FloorDivide)
NB_ENUM_BINOP(and, PyNumber_And)
NB_ENUM_BINOP(or, PyNumber_Or)
NB_ENUM_BINOP(xor, PyNumber_Xor)
NB_ENUM_BINOP(lshift, PyNumber_Lshift)
NB_ENUM_BINOP(rshift, PyNumber_Rshift)
NB_ENUM_UNOP(neg, PyNumber_Negative)
NB_ENUM_UNOP(inv, PyNumber_Invert)
NB_ENUM_UNOP(abs, PyNumber_Absolute)

int nb_enum_clear(PyObject *) {
    return 0;
}

int nb_enum_traverse(PyObject *o, visitproc visit, void *arg) {
    Py_VISIT(Py_TYPE(o));
    return 0;
}

void nb_enum_prepare(PyType_Slot **s, bool is_arithmetic) {
    PyType_Slot *t = *s;

    *t++ = { Py_tp_new, (void *) nb_enum_init };
    *t++ = { Py_tp_init, (void *) nullptr };
    *t++ = { Py_tp_repr, (void *) nb_enum_repr };
    *t++ = { Py_tp_richcompare, (void *) nb_enum_richcompare };
    *t++ = { Py_nb_int, (void *) nb_enum_int };
    *t++ = { Py_tp_getset, (void *) nb_enum_getset };
    *t++ = { Py_tp_traverse, (void *) nb_enum_traverse };
    *t++ = { Py_tp_clear, (void *) nb_enum_clear };

    if (is_arithmetic) {
        *t++ = { Py_nb_add, (void *) nb_enum_add };
        *t++ = { Py_nb_subtract, (void *) nb_enum_sub };
        *t++ = { Py_nb_multiply, (void *) nb_enum_sub };
        *t++ = { Py_nb_floor_divide, (void *) nb_enum_div };
        *t++ = { Py_nb_or, (void *) nb_enum_or };
        *t++ = { Py_nb_xor, (void *) nb_enum_xor };
        *t++ = { Py_nb_and, (void *) nb_enum_and };
        *t++ = { Py_nb_rshift, (void *) nb_enum_rshift };
        *t++ = { Py_nb_lshift, (void *) nb_enum_lshift };
        *t++ = { Py_nb_negative, (void *) nb_enum_neg };
        *t++ = { Py_nb_invert, (void *) nb_enum_inv };
        *t++ = { Py_nb_absolute, (void *) nb_enum_abs };
    }

    *s = t;
}

void nb_enum_put(PyObject *type, const char *name, const void *value,
                 const char *doc) noexcept {
    PyObject *doc_obj, *rec, *dict, *int_val;

    PyObject *name_obj = PyUnicode_InternFromString(name);
    if (doc) {
        doc_obj = PyUnicode_FromString(doc);
    } else {
        doc_obj = Py_None;
        Py_INCREF(Py_None);
    }

    nb_inst *inst = (nb_inst *) inst_new_impl((PyTypeObject *) type, nullptr);

    if (!doc_obj || !name_obj || !inst)
        goto error;

    rec = PyTuple_New(3);
    NB_TUPLE_SET_ITEM(rec, 0, name_obj);
    NB_TUPLE_SET_ITEM(rec, 1, doc_obj);
    NB_TUPLE_SET_ITEM(rec, 2, (PyObject *) inst);

    memcpy(inst_ptr(inst), value, nb_type_data((PyTypeObject *) type)->size);
    inst->destruct = false;
    inst->cpp_delete = false;
    inst->ready = true;

    if (PyObject_SetAttr(type, name_obj, (PyObject *) inst))
        goto error;

    int_val = nb_enum_int((PyObject *) inst);
    if (!int_val)
        goto error;

    dict = PyObject_GetAttrString(type, "__entries");
    if (!dict) {
        PyErr_Clear();
        dict = PyDict_New();
        if (!dict)
            goto error;

        if (PyObject_SetAttrString(type, "__entries", dict))
            goto error;
    }

    if (PyDict_SetItem(dict, int_val, rec))
        goto error;

    Py_DECREF(int_val);
    Py_DECREF(dict);
    Py_DECREF(rec);

    return;

error:
    fail("nanobind::detail::nb_enum_add(): could not create enum entry!");
}

void nb_enum_export(PyObject *tp) {
    type_data *t = nb_type_data((PyTypeObject *) tp);
    PyObject *entries = PyObject_GetAttrString(tp, "__entries");

    if (!entries || !(t->flags & (uint32_t) type_flags::has_scope))
        fail("nanobind::detail::nb_enum_export(): internal error!");

    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(entries, &pos, &key, &value)) {
        if (!PyTuple_CheckExact(value) || NB_TUPLE_GET_SIZE(value) != 3)
            fail("nanobind::detail::nb_enum_export(): internal error! (2)");

        setattr(t->scope,
                NB_TUPLE_GET_ITEM(value, 0),
                NB_TUPLE_GET_ITEM(value, 2));
    }

    Py_DECREF(entries);
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
