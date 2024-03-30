/*
    src/nb_type.cpp: libnanobind functionality for binding classes

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include "nb_internals.h"

#if defined(_MSC_VER)
#  pragma warning(disable: 4706) // assignment within conditional expression
#endif

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

static PyObject **nb_dict_ptr(PyObject *self) {
    PyTypeObject *tp = Py_TYPE(self);
#if !defined(Py_LIMITED_API)
    return (PyObject **) ((uint8_t *) self + tp->tp_dictoffset);
#else
    return (PyObject **) ((uint8_t *) self + nb_type_data(tp)->dictoffset);
#endif
}

static int inst_clear(PyObject *self) {
    PyObject *&dict = *nb_dict_ptr(self);
    Py_CLEAR(dict);
    return 0;
}

static int inst_traverse(PyObject *self, visitproc visit, void *arg) {
    PyObject *&dict = *nb_dict_ptr(self);
    if (dict)
        Py_VISIT(dict);
#if PY_VERSION_HEX >= 0x03090000
    Py_VISIT(Py_TYPE(self));
#endif
    return 0;
}

static int inst_init(PyObject *self, PyObject *, PyObject *) {
    const type_data *t = nb_type_data(Py_TYPE(self));
    PyErr_Format(PyExc_TypeError, "%s: no constructor defined!", t->name);
    return -1;
}

/// Allocate memory for a nb_type instance with internal or external storage
PyObject *inst_new_impl(PyTypeObject *tp, void *value) {
    bool gc = PyType_HasFeature(tp, Py_TPFLAGS_HAVE_GC);
    const type_data *t = nb_type_data(tp);
    size_t align = (size_t) t->align;

    nb_inst *self;

    if (!gc) {
        size_t size = sizeof(nb_inst);
        if (!value) {
            // Internal storage: space for the object and padding for alignment
            size += t->size;
            if (align > sizeof(void *))
                size += align - sizeof(void *);
        }

        self = (nb_inst *) PyObject_Malloc(size);
        if (!self)
            return PyErr_NoMemory();
        memset(self, 0, sizeof(nb_inst));
        PyObject_Init((PyObject *) self, tp);
    } else {
        self = (nb_inst *) PyType_GenericAlloc(tp, 0);
    }

    if (!value) {
        // Compute suitably aligned instance payload pointer
        uintptr_t payload = (uintptr_t) (self + 1);
        payload = (payload + align - 1) / align * align;

        // Encode offset to aligned payload
        self->offset = (int32_t) ((intptr_t) payload - (intptr_t) self);
        self->direct = true;
        self->internal = true;

        value = (void *) payload;
    } else {
        // Compute offset to instance value
        int32_t offset = (int32_t) ((intptr_t) value - (intptr_t) self);

        if ((intptr_t) self + offset == (intptr_t) value) {
            // Offset *is* representable as 32 bit value
            self->offset = offset;
            self->direct = true;
        } else {
            if (!gc) {
                // Offset *not* representable, allocate extra memory for a pointer
                nb_inst *self_2 =
                    (nb_inst *) PyObject_Realloc(self, sizeof(nb_inst) + sizeof(void *));

                if (!self_2) {
                    PyObject_Free(self);
                    return PyErr_NoMemory();
                }

                self = self_2;
            }

            *(void **) (self + 1) = value;
            self->offset = (int32_t) sizeof(nb_inst);
            self->direct = false;
        }

        self->internal = false;
    }

    // Update hash table that maps from C++ to Python instance
    auto [it, success] = internals_get().inst_c2p.try_emplace(
        std::pair<void *, std::type_index>(value, *t->type),
        self);

    if (!success)
        fail("nanobind::detail::inst_new(): duplicate object!");

    return (PyObject *) self;
}

// Allocate a new instance with co-located storage
PyObject *inst_new(PyTypeObject *type, PyObject *, PyObject *) {
    return inst_new_impl(type, nullptr);
}

static void inst_dealloc(PyObject *self) {
    PyTypeObject *tp = Py_TYPE(self);
    const type_data *t = nb_type_data(tp);

    bool gc = PyType_HasFeature(tp, Py_TPFLAGS_HAVE_GC);
    if (gc)
        PyObject_GC_UnTrack(self);

    if (t->flags & (uint32_t) type_flags::has_dynamic_attr) {
        PyObject *&dict = *nb_dict_ptr(self);
        Py_CLEAR(dict);
    }

    nb_inst *inst = (nb_inst *) self;
    void *p = inst_ptr(inst);

    if (inst->destruct) {
        if (t->flags & (uint32_t) type_flags::is_destructible) {
            if (t->flags & (uint32_t) type_flags::has_destruct)
                t->destruct(p);
        } else {
            fail("nanobind::detail::inst_dealloc(\"%s\"): attempted to call "
                 "the destructor of a non-destructible type!", t->name);
        }
    }

    if (inst->cpp_delete) {
        if (t->align <= (uint32_t) __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            operator delete(p);
        else
            operator delete(p, std::align_val_t(t->align));
    }

    nb_internals &internals = internals_get();
    if (inst->clear_keep_alive) {
        auto it = internals.keep_alive.find(self);
        if (it == internals.keep_alive.end())
            fail("nanobind::detail::inst_dealloc(\"%s\"): inconsistent "
                 "keep_alive information", t->name);

        keep_alive_set ref_set = std::move(it.value());
        internals.keep_alive.erase(it);

        for (keep_alive_entry e: ref_set) {
            if (!e.deleter)
                Py_DECREF((PyObject *) e.data);
            else
                e.deleter(e.data);
        }
    }

    // Update hash table that maps from C++ to Python instance
    auto it = internals.inst_c2p.find(
        std::pair<void *, std::type_index>(p, *t->type));
    if (it == internals.inst_c2p.end())
        fail("nanobind::detail::inst_dealloc(\"%s\"): attempted to delete "
             "an unknown instance (%p)!", t->name, p);
    internals.inst_c2p.erase(it);

    if (gc) {
        #if defined(Py_LIMITED_API)
            static freefunc tp_free =
                (freefunc) PyType_GetSlot(tp, Py_tp_free);
        #else
            freefunc tp_free = tp->tp_free;
        #endif

        tp_free(self);
    } else {
        PyObject_Free(self);
    }

    Py_DECREF(tp);
}

void nb_type_dealloc(PyObject *o) {
    type_data *t = nb_type_data((PyTypeObject *) o);

    if (t->type && (t->flags & (uint32_t) type_flags::is_python_type) == 0) {
        nb_internals &internals = internals_get();
        auto it = internals.type_c2p.find(std::type_index(*t->type));
        if (it == internals.type_c2p.end())
            fail("nanobind::detail::nb_type_dealloc(\"%s\"): could not "
                 "find type!", t->name);
        internals.type_c2p.erase(it);
    }

    if (t->flags & (uint32_t) type_flags::has_implicit_conversions) {
        free(t->implicit);
        free(t->implicit_py);
    }

    if (t->flags & (uint32_t) type_flags::has_supplement)
        free(t->supplement);

    free((char *) t->name);

    #if defined(Py_LIMITED_API)
        static destructor tp_dealloc =
            (destructor) PyType_GetSlot(&PyType_Type, Py_tp_dealloc);
    #else
        destructor tp_dealloc = PyType_Type.tp_dealloc;
    #endif

    tp_dealloc(o);
}

/// Called when a C++ type is extended from within Python
int nb_type_init(PyObject *self, PyObject *args, PyObject *kwds) {
    if (NB_TUPLE_GET_SIZE(args) != 3) {
        PyErr_SetString(PyExc_RuntimeError,
                        "nb_type_init(): invalid number of arguments!");
        return -1;
    }

    PyObject *bases = NB_TUPLE_GET_ITEM(args, 1);
    if (!PyTuple_CheckExact(bases) || NB_TUPLE_GET_SIZE(bases) != 1) {
        PyErr_SetString(PyExc_RuntimeError,
                        "nb_type_init(): invalid number of bases!");
        return -1;
    }

    PyObject *base = NB_TUPLE_GET_ITEM(bases, 0);
    if (!PyType_Check(base)) {
        PyErr_SetString(PyExc_RuntimeError, "nb_type_init(): expected a base type object!");
        return -1;
    }

    type_data *t_b = nb_type_data((PyTypeObject *) base);
    if (t_b->flags & (uint32_t) type_flags::is_final) {
        PyErr_Format(PyExc_TypeError, "The type '%s' prohibits subclassing!",
                     t_b->name);
        return -1;
    }

    #if defined(Py_LIMITED_API)
        static initproc tp_init =
            (initproc) PyType_GetSlot(&PyType_Type, Py_tp_init);
    #else
        initproc tp_init = PyType_Type.tp_init;
    #endif

    int rv = tp_init(self, args, kwds);
    if (rv)
        return rv;

    type_data *t = nb_type_data((PyTypeObject *) self);

    *t = *t_b;
    t->flags |=  (uint32_t) type_flags::is_python_type;
    t->flags &= ~((uint32_t) type_flags::has_implicit_conversions |
                  (uint32_t) type_flags::has_supplement);
    PyObject *name = nb_type_name((PyTypeObject *) self);
    t->name = NB_STRDUP(PyUnicode_AsUTF8AndSize(name, nullptr));
    Py_DECREF(name);
    t->type_py = (PyTypeObject *) self;
    t->base = t_b->type;
    t->base_py = t_b->type_py;
    t->implicit = nullptr;
    t->implicit_py = nullptr;
    t->supplement = nullptr;

    return 0;
}

/// Called when a C++ type is bound via nb::class_<>
PyObject *nb_type_new(const type_data *t) noexcept {
    bool is_signed_enum    = t->flags & (uint32_t) type_flags::is_signed_enum,
         is_unsigned_enum  = t->flags & (uint32_t) type_flags::is_unsigned_enum,
         is_arithmetic     = t->flags & (uint32_t) type_flags::is_arithmetic,
         is_enum           = is_signed_enum || is_unsigned_enum,
         has_scope         = t->flags & (uint32_t) type_flags::has_scope,
         has_doc           = t->flags & (uint32_t) type_flags::has_doc,
         has_base          = t->flags & (uint32_t) type_flags::has_base,
         has_base_py       = t->flags & (uint32_t) type_flags::has_base_py,
         has_type_callback = t->flags & (uint32_t) type_flags::has_type_callback,
         has_supplement    = t->flags & (uint32_t) type_flags::has_supplement,
         has_dynamic_attr  = t->flags & (uint32_t) type_flags::has_dynamic_attr,
         intrusive_ptr     = t->flags & (uint32_t) type_flags::intrusive_ptr;

    nb_internals &internals = internals_get();
    str name(t->name), qualname = name;
    object modname;
    PyObject *mod = nullptr;

    if (has_scope) {
        if (PyModule_Check(t->scope)) {
            mod = t->scope;
            modname = getattr(t->scope, "__name__", handle());
        } else {
            modname = getattr(t->scope, "__module__", handle());

            object scope_qualname = getattr(t->scope, "__qualname__", handle());
            if (scope_qualname.is_valid())
                qualname = steal<str>(
                    PyUnicode_FromFormat("%U.%U", scope_qualname.ptr(), name.ptr()));
        }
    }

    if (modname.is_valid())
        name = steal<str>(
            PyUnicode_FromFormat("%U.%U", modname.ptr(), name.ptr()));

    constexpr size_t ptr_size = sizeof(void *);
    size_t basicsize = sizeof(nb_inst) + t->size;
    if (t->align > ptr_size)
        basicsize += t->align - ptr_size;

    PyObject *base = nullptr;
    if (has_base_py) {
        if (has_base)
            fail("nanobind::detail::nb_type_new(\"%s\"): multiple base types "
                 "specified!", t->name);
        base = (PyObject *) t->base_py;
    } else if (has_base) {
        auto it = internals.type_c2p.find(std::type_index(*t->base));
        if (it == internals.type_c2p.end())
            fail("nanobind::detail::nb_type_new(\"%s\"): base type \"%s\" not "
                 "known to nanobind!", t->name, type_name(t->base));
        base = (PyObject *) it->second->type_py;
    }

    type_data *tb = nullptr;
    if (base) {
        // Check if the base type already has dynamic attributes
        tb = nb_type_data((PyTypeObject *) base);
        if (tb->flags & (uint32_t) type_flags::has_dynamic_attr)
            has_dynamic_attr = true;

        /* Handle a corner case (base class larger than derived class)
           which can arise when extending trampoline base classes */
        size_t base_basicsize = sizeof(nb_inst) + tb->size;
        if (tb->align > ptr_size)
            base_basicsize += tb->align - ptr_size;
        if (base_basicsize > basicsize)
            basicsize = base_basicsize;
    }

    char *name_copy = NB_STRDUP(name.c_str());

    PyMemberDef members[2] { };
    PyType_Slot slots[128], *s = slots;
    PyType_Spec spec = {
        .name = name_copy,
        .basicsize = (int) basicsize,
        .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .slots = slots
    };

    if (base)
        *s++ = { Py_tp_base, (void *) base };

    *s++ = { Py_tp_init, (void *) inst_init };
    *s++ = { Py_tp_new, (void *) inst_new };
    *s++ = { Py_tp_dealloc, (void *) inst_dealloc };

    if (has_doc)
        *s++ = { Py_tp_doc, (void *) t->doc };

    if (has_type_callback)
        t->type_callback(&s);

    if (is_enum)
        nb_enum_prepare(&s, is_arithmetic);

    for (PyType_Slot *ts = slots; ts != s; ++ts) {
        if (ts->slot == Py_tp_traverse ||
            ts->slot == Py_tp_clear)
            spec.flags |= Py_TPFLAGS_HAVE_GC;
    }

    if (has_dynamic_attr) {
        if (spec.flags & Py_TPFLAGS_HAVE_GC)
            fail("nanobind::detail::nb_type_new(\"%s\"): internal error -- "
                 "attempted to enable dynamic attributes in a type with its "
                 "own garbage collection hooks!", t->name);

        // realign to sizeof(void*), add one pointer
        basicsize = (basicsize + ptr_size - 1) / ptr_size * ptr_size;
        basicsize += ptr_size;

        members[0] = PyMemberDef{ "__dictoffset__", T_PYSSIZET,
                                  (Py_ssize_t) (basicsize - ptr_size), READONLY,
                                  nullptr };
        *s++ = { Py_tp_members, (void *) members };
        *s++ = { Py_tp_traverse, (void *) inst_traverse };
        *s++ = { Py_tp_clear, (void *) inst_clear };

        spec.basicsize = (int) basicsize;
        spec.flags |= Py_TPFLAGS_HAVE_GC;
    }

    *s++ = { 0, nullptr };

    PyTypeObject *metaclass = is_enum ? internals.nb_enum
                                      : internals.nb_type;

#if PY_VERSION_HEX >= 0x030C0000
    // Life is good, PyType_FromMetaclass() is available
    PyObject *result = PyType_FromMetaclass(metaclass, mod, &spec, base);
    if (!result)
        fail("nanobind::detail::nb_type_new(\"%s\"): type construction failed!", t->name);
#else
    /* The fallback code below is cursed. It provides an alternative when
       PyType_FromMetaclass() is not available we are furthermore *not*
       targeting the stable ABI interface. It calls PyType_FromSpec() to create
       a tentative type, copies its contents into a larger type with a
       different metaclass, then lets the original type expire. */

    (void) mod;

    PyObject *temp = PyType_FromSpec(&spec);
    PyHeapTypeObject *temp_ht = (PyHeapTypeObject *) temp;
    PyTypeObject *temp_tp = &temp_ht->ht_type;

    Py_INCREF(temp_ht->ht_name);
    Py_INCREF(temp_ht->ht_qualname);
    Py_INCREF(temp_tp->tp_base);
    Py_XINCREF(temp_ht->ht_slots);

    PyObject *result = PyType_GenericAlloc(metaclass, 0);
    if (!temp || !result)
        fail("nanobind::detail::nb_type_new(\"%s\"): type construction failed!",
             t->name);

    PyHeapTypeObject *ht = (PyHeapTypeObject *) result;
    PyTypeObject *tp = &ht->ht_type;

    memcpy(ht, temp_ht, sizeof(PyHeapTypeObject));

    tp->ob_base.ob_base.ob_type = metaclass;
    tp->ob_base.ob_base.ob_refcnt = 1;
    tp->ob_base.ob_size = 0;
    tp->tp_as_async = &ht->as_async;
    tp->tp_as_number = &ht->as_number;
    tp->tp_as_sequence = &ht->as_sequence;
    tp->tp_as_mapping = &ht->as_mapping;
    tp->tp_as_buffer = &ht->as_buffer;
    tp->tp_name = name_copy;
    tp->tp_flags = spec.flags | Py_TPFLAGS_HEAPTYPE;

#if PY_VERSION_HEX < 0x03090000
    if (has_dynamic_attr)
        tp->tp_dictoffset = (Py_ssize_t) (basicsize - ptr_size);
#endif

    tp->tp_dict = tp->tp_bases = tp->tp_mro = tp->tp_cache =
        tp->tp_subclasses = tp->tp_weaklist = nullptr;
    ht->ht_cached_keys = nullptr;
    tp->tp_version_tag = 0;

    PyType_Ready(tp);
    Py_DECREF(temp);
#endif

    type_data *to = nb_type_data((PyTypeObject *) result);
    *to = *t;

    if (!intrusive_ptr && tb &&
        (tb->flags & (uint32_t) type_flags::intrusive_ptr)) {
        to->flags |= (uint32_t) type_flags::intrusive_ptr;
        to->set_self_py = tb->set_self_py;
    }

    to->name = name_copy;
    to->type_py = (PyTypeObject *) result;

    if (has_supplement) {
        if (!to->supplement)
            fail("nanobind::detail::nb_type_new(\"%s\"): supplemental data "
                 "allocation failed!", t->name);
    } else {
        to->supplement = nullptr;
    }

    if (has_dynamic_attr) {
        to->flags |= (uint32_t) type_flags::has_dynamic_attr;
        #if defined(Py_LIMITED_API)
            to->dictoffset = (Py_ssize_t) (basicsize - ptr_size);
        #endif
    }

    if (has_scope)
        setattr(t->scope, t->name, result);

    setattr(result, "__qualname__", qualname.ptr());

    if (modname.is_valid())
        setattr(result, "__module__", modname.ptr());

    // Update hash table that maps from std::type_info to Python type
    auto [it, success] =
        internals.type_c2p.try_emplace(std::type_index(*t->type), to);
    if (!success)
        fail("nanobind::detail::nb_type_new(\"%s\"): type was already "
             "registered!", t->name);

    return result;
}

/// Encapsulates the implicit conversion part of nb_type_get()
static NB_NOINLINE bool nb_type_get_implicit(PyObject *src,
                                             const std::type_info *cpp_type_src,
                                             const type_data *dst_type,
                                             nb_internals &internals,
                                             cleanup_list *cleanup, void **out) {
    if (dst_type->implicit && cpp_type_src) {
        const std::type_info **it = dst_type->implicit;
        const std::type_info *v;

        while ((v = *it++)) {
            if (v == cpp_type_src || *v == *cpp_type_src)
                goto found;
        }

        it = dst_type->implicit;
        while ((v = *it++)) {
            auto it2 = internals.type_c2p.find(std::type_index(*v));
            if (it2 != internals.type_c2p.end() &&
                PyType_IsSubtype(Py_TYPE(src), it2->second->type_py))
                goto found;
        }
    }

    if (dst_type->implicit_py) {
        bool (**it)(PyTypeObject *, PyObject *, cleanup_list *) noexcept =
            dst_type->implicit_py;
        bool (*v2)(PyTypeObject *, PyObject *, cleanup_list *) noexcept;

        while ((v2 = *it++)) {
            if (v2(dst_type->type_py, src, cleanup))
                goto found;
        }
    }

    return false;

found:

    PyObject *result;
#if PY_VERSION_HEX < 0x03090000 || defined(Py_LIMITED_API)
    PyObject *args = PyTuple_New(1);
    if (!args) {
        PyErr_Clear();
        return false;
    }
    Py_INCREF(src);
    NB_TUPLE_SET_ITEM(args, 0, src);
    result = PyObject_CallObject((PyObject *) dst_type->type_py, args);
    Py_DECREF(args);
#else
    PyObject *args[2] = { nullptr, src };
    result = PyObject_Vectorcall((PyObject *) dst_type->type_py, args + 1,
                                 NB_VECTORCALL_ARGUMENTS_OFFSET + 1, nullptr);
#endif

    if (result) {
        cleanup->append(result);
        *out = inst_ptr((nb_inst *) result);
        return true;
    } else {
        PyErr_Clear();

        PyObject *name = nb_inst_name(src);
        PyErr_WarnFormat(PyExc_RuntimeWarning, 1,
                         "nanobind: implicit conversion from type '%U' "
                         "to type '%s' failed!", dst_type->name);
        Py_DECREF(name);

        return false;
    }
}

// Attempt to retrieve a pointer to a C++ instance
bool nb_type_get(const std::type_info *cpp_type, PyObject *src, uint8_t flags,
                 cleanup_list *cleanup, void **out) noexcept {
    // Convert None -> nullptr
    if (src == Py_None) {
        *out = nullptr;
        return true;
    }

    nb_internals &internals = internals_get();
    PyTypeObject *src_type = Py_TYPE(src);
    const std::type_info *cpp_type_src = nullptr;
    const PyTypeObject *metaclass = Py_TYPE((PyObject *) src_type);
    const bool src_is_nb_type = metaclass == internals.nb_type ||
                                metaclass == internals.nb_enum;

    type_data *dst_type = nullptr;

    // If 'src' is a nanobind-bound type
    if (src_is_nb_type) {
        type_data *t = nb_type_data(src_type);
        cpp_type_src = t->type;

        // Check if the source / destination typeid are an exact match
        bool valid = cpp_type == cpp_type_src || *cpp_type == *cpp_type_src;

        // If not, look up the Python type and check the inheritance chain
        if (!valid) {
            auto it = internals.type_c2p.find(std::type_index(*cpp_type));
            if (it != internals.type_c2p.end()) {
                dst_type = it->second;
                valid = PyType_IsSubtype(src_type, dst_type->type_py);
            }
        }

        // Success, return the pointer if the instance is correctly initialized
        if (valid) {
            nb_inst *inst = (nb_inst *) src;

            if (!inst->ready &&
                (flags & (uint8_t) cast_flags::construct) == 0) {
                PyErr_WarnFormat(PyExc_RuntimeWarning, 1,
                                 "nanobind: attempted to access an "
                                 "uninitialized instance of type '%s'!\n",
                                 t->name);
                return false;
            }

            *out = inst_ptr(inst);

            return true;
        }
    }

    // Try an implicit conversion as last resort (if possible & requested)
    if ((flags & (uint16_t) cast_flags::convert) && cleanup) {
        if (!src_is_nb_type) {
            auto it = internals.type_c2p.find(std::type_index(*cpp_type));
            if (it != internals.type_c2p.end())
                dst_type = it->second;
        }

        if (dst_type &&
            (dst_type->flags & (uint32_t) type_flags::has_implicit_conversions))
            return nb_type_get_implicit(src, cpp_type_src, dst_type, internals,
                                        cleanup, out);
    }
    return false;
}

static PyObject *keep_alive_callback(PyObject *self, PyObject *const *args,
                                     Py_ssize_t nargs) {
    if (nargs != 1 || !PyWeakref_CheckRefExact(args[0]))
        fail("nanobind::detail::keep_alive_callback(): invalid input!");
    Py_DECREF(args[0]); // self
    Py_DECREF(self); // patient
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef keep_alive_callback_def = {
    "keep_alive_callback",
    (PyCFunction) (void *) keep_alive_callback,
    METH_FASTCALL,
    "Implementation detail of nanobind::detail::keep_alive"
};


void keep_alive(PyObject *nurse, PyObject *patient) noexcept {
    if (!patient)
        return;

    if (!nurse)
        fail("nanobind::detail::keep_alive(): the 'nurse' argument must be "
             "provided!");

    nb_internals &internals = internals_get();
    PyTypeObject *metaclass = Py_TYPE((PyObject *) Py_TYPE(nurse));

    if (metaclass == internals.nb_type || metaclass == internals.nb_enum) {
        // Populate nanobind-internal data structures
        keep_alive_set &keep_alive = internals.keep_alive[nurse];

        auto [it, success] = keep_alive.emplace(patient);
        if (success) {
            Py_INCREF(patient);
            ((nb_inst *) nurse)->clear_keep_alive = true;
        } else {
            if (it->deleter)
                fail("nanobind::detail::keep_alive(): internal error: entry "
                     "has a deletion callback!");
        }
    } else {
        PyObject *callback =
            PyCFunction_New(&keep_alive_callback_def, patient);
        if (!callback)
            fail("nanobind::detail::keep_alive(): callback creation failed!");
        PyObject *weakref = PyWeakref_NewRef(nurse, callback);
        if (!weakref)
            fail("nanobind::detail::keep_alive(): could not create a weak "
                 "reference! Likely, the 'nurse' argument you specified is not "
                 "a weak-referenceable type!");

        // Increase patient reference count, leak weak reference
        Py_INCREF(patient);
        Py_DECREF(callback);
    }
}

void keep_alive(PyObject *nurse, void *payload,
                void (*callback)(void *) noexcept) noexcept {
    if (!nurse)
        fail("nanobind::detail::keep_alive(): nurse==nullptr!");

    PyTypeObject *metaclass = Py_TYPE((PyObject *) Py_TYPE(nurse));

    nb_internals &internals = internals_get();

    if (metaclass == internals.nb_type || metaclass == internals.nb_enum) {
        keep_alive_set &keep_alive = internals.keep_alive[nurse];
        auto [it, success] = keep_alive.emplace(payload, callback);
        if (!success)
            raise("keep_alive(): the given 'payload' pointer was already registered!");
        ((nb_inst *) nurse)->clear_keep_alive = true;
    } else {
        PyObject *patient = capsule_new(payload, callback);
        keep_alive(nurse, patient);
        Py_DECREF(patient);
    }
}

PyObject *nb_type_put(const std::type_info *cpp_type, void *value,
                      rv_policy rvp, cleanup_list *cleanup,
                      bool *is_new) noexcept {
    // Convert nullptr -> None
    if (!value) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // Check if the instance is already registered with nanobind
    nb_internals &internals = internals_get();
    auto it = internals.inst_c2p.find(
        std::pair<void *, std::type_index>(value, *cpp_type));
    if (it != internals.inst_c2p.end()) {
        PyObject *result = (PyObject *) it->second;
        Py_INCREF(result);
        return result;
    } else if (rvp == rv_policy::none) {
        return nullptr;
    }

    // Look up the corresponding type
    auto it2 = internals.type_c2p.find(std::type_index(*cpp_type));
    if (it2 == internals.type_c2p.end())
        return nullptr;

    // The reference_internals RVP needs a self pointer, give up if unavailable
    if (rvp == rv_policy::reference_internal && (!cleanup || !cleanup->self()))
        return nullptr;

    type_data *t = it2->second;
    const bool intrusive = t->flags & (uint32_t) type_flags::intrusive_ptr;
    if (intrusive)
        rvp = rv_policy::take_ownership;

    const bool store_in_obj = rvp == rv_policy::copy || rvp == rv_policy::move;

    nb_inst *inst =
        (nb_inst *) inst_new_impl(t->type_py, store_in_obj ? nullptr : value);
    if (!inst)
        return nullptr;

    if (is_new)
        *is_new = true;

    void *new_value = inst_ptr(inst);
    if (rvp == rv_policy::move) {
        if (t->flags & (uint32_t) type_flags::is_move_constructible) {
            if (t->flags & (uint32_t) type_flags::has_move) {
                try {
                    t->move(new_value, value);
                } catch (...) {
                    Py_DECREF(inst);
                    return nullptr;
                }
            } else {
                memcpy(new_value, value, t->size);
                memset(value, 0, t->size);
            }
        } else {
            if (t->flags & (uint32_t) type_flags::is_copy_constructible) {
                rvp = rv_policy::copy;
            } else {
                fail("nanobind::detail::nb_type_put(\"%s\"): attempted to move "
                     "an instance that is neither copy- nor move-constructible!",
                     t->name);
            }
        }
    }

    if (rvp == rv_policy::copy) {
        if (t->flags & (uint32_t) type_flags::is_copy_constructible) {
            if (t->flags & (uint32_t) type_flags::has_copy) {
                try {
                    t->copy(new_value, value);
                } catch (...) {
                    Py_DECREF(inst);
                    return nullptr;
                }
            } else {
                memcpy(new_value, value, t->size);
            }
        } else {
            fail("nanobind::detail::nb_type_put(\"%s\"): attempted to copy "
                 "an instance that is not copy-constructible!", t->name);
        }
    }

    inst->destruct = rvp != rv_policy::reference && rvp != rv_policy::reference_internal;
    inst->cpp_delete = rvp == rv_policy::take_ownership;
    inst->ready = true;

    if (rvp == rv_policy::reference_internal)
        keep_alive((PyObject *) inst, cleanup->self());

    if (intrusive)
        t->set_self_py(new_value, (PyObject *) inst);

    return (PyObject *) inst;
}

PyObject *nb_type_put_unique(const std::type_info *cpp_type, void *value,
                             cleanup_list *cleanup, bool cpp_delete) noexcept {
    rv_policy policy = cpp_delete ? rv_policy::take_ownership : rv_policy::none;

    bool is_new = false;
    PyObject *o = nb_type_put(cpp_type, value, policy, cleanup, &is_new);
    if (!o)
        return nullptr;

    if (!cpp_delete && is_new)
        fail("nanobind::detail::nb_type_put_unique(type='%s', cpp_delete=%i): "
             "ownership status has become corrupted.",
             type_name(cpp_type), cpp_delete);

    nb_inst *inst = (nb_inst *) o;

    if (cpp_delete) {
        if (inst->ready != is_new || inst->destruct != is_new ||
            inst->cpp_delete != is_new)
            fail("nanobind::detail::nb_type_put_unique(type='%s', "
                 "cpp_delete=%i): unexpected status flags! (ready=%i, "
                 "destruct=%i, cpp_delete=%i)",
                 type_name(cpp_type), cpp_delete, inst->ready,
                 inst->destruct, inst->cpp_delete);

        inst->ready = inst->destruct = inst->cpp_delete = true;
    } else {
        if (inst->ready)
            fail("nanobind::detail::nb_type_put_unique('%s'): ownership "
                 "status has become corrupted.", type_name(cpp_type));
        inst->ready = true;
    }
    return o;
}

void nb_type_relinquish_ownership(PyObject *o, bool cpp_delete) {
    nb_inst *inst = (nb_inst *) o;

    // This function is called to indicate ownership *changes*
    if (!inst->ready)
        fail("nanobind::detail::nb_relinquish_ownership('%s'): ownership "
             "status has become corrupted.",
             PyUnicode_AsUTF8AndSize(nb_inst_name(o), nullptr));

    if (cpp_delete) {
        if (!inst->cpp_delete || !inst->destruct || inst->internal) {
            PyErr_WarnFormat(
                PyExc_RuntimeWarning, 1,
                "nanobind::detail::nb_relinquish_ownership(): could not "
                "transfer ownership of a Python instance of type '%s' to C++. "
                "This is only possible when the instance was previously "
                "constructed on the C++ side and is now owned by Python, which "
                "was not the case here. You could change the unique pointer "
                "signature to std::unique_ptr<T, nb::deleter<T>> to work around "
                "this issue.",
                PyUnicode_AsUTF8AndSize(nb_inst_name(o), nullptr));

            raise_next_overload();
        }

        inst->cpp_delete = false;
        inst->destruct = false;
    }

    inst->ready = false;
}

/// Special case to handle 'Class.property = value' assignments
int nb_type_setattro(PyObject* obj, PyObject* name, PyObject* value) {
    nb_internals &internals = internals_get();

    internals.nb_static_property_enabled = false;
    PyObject *cur = PyObject_GetAttr(obj, name);
    internals.nb_static_property_enabled = true;

    if (cur) {
        if (Py_TYPE(cur) == internals.nb_static_property) {
            int rv = nb_static_property_set(cur, obj, value);
            Py_DECREF(cur);
            return rv;
        }
        Py_DECREF(cur);
    } else {
        PyErr_Clear();
    }

    #if defined(Py_LIMITED_API)
        static setattrofunc tp_setattro =
            (setattrofunc) PyType_GetSlot(&PyType_Type, Py_tp_setattro);
    #else
        setattrofunc tp_setattro = PyType_Type.tp_setattro;
    #endif

    return tp_setattro(obj, name, value);
}

bool nb_type_isinstance(PyObject *o, const std::type_info *t) noexcept {
    nb_internals &internals = internals_get();
    auto it = internals.type_c2p.find(std::type_index(*t));
    if (it == internals.type_c2p.end())
        return false;
    return PyType_IsSubtype(Py_TYPE(o), it->second->type_py);
}

PyObject *nb_type_lookup(const std::type_info *t) noexcept {
    nb_internals &internals = internals_get();
    auto it = internals.type_c2p.find(std::type_index(*t));
    if (it != internals.type_c2p.end())
        return (PyObject *) it->second->type_py;
    return nullptr;
}

bool nb_type_check(PyObject *t) noexcept {
    nb_internals &internals = internals_get();
    PyTypeObject *metaclass = Py_TYPE(t);

    return metaclass == internals.nb_type ||
           metaclass == internals.nb_enum;
}

size_t nb_type_size(PyObject *t) noexcept {
    return nb_type_data((PyTypeObject *) t)->size;
}

size_t nb_type_align(PyObject *t) noexcept {
    return nb_type_data((PyTypeObject *) t)->align;
}

const std::type_info *nb_type_info(PyObject *t) noexcept {
    return nb_type_data((PyTypeObject *) t)->type;
}

void *nb_type_supplement(PyObject *t) noexcept {
    return nb_type_data((PyTypeObject *) t)->supplement;
}

PyObject *nb_inst_alloc(PyTypeObject *t) {
    PyObject *result = inst_new_impl(t, nullptr);
    if (!result)
        raise_python_error();
    return result;
}

void *nb_inst_ptr(PyObject *o) noexcept {
    return inst_ptr((nb_inst *) o);
}

void nb_inst_zero(PyObject *o) noexcept {
    nb_inst *nbi = (nb_inst *) o;
    type_data *t = nb_type_data(Py_TYPE(o));
    memset(inst_ptr(nbi), 0, t->size);
    nbi->ready = nbi->destruct = true;
}

void nb_inst_set_state(PyObject *o, bool ready, bool destruct) noexcept {
    nb_inst *nbi = (nb_inst *) o;
    nbi->ready = ready;
    nbi->destruct = destruct;
}

std::pair<bool, bool> nb_inst_state(PyObject *o) noexcept {
    nb_inst *nbi = (nb_inst *) o;
    return { (bool) nbi->ready, (bool) nbi->destruct };
}

void nb_inst_destruct(PyObject *o) noexcept {
    nb_inst *nbi = (nb_inst *) o;
    type_data *t = nb_type_data(Py_TYPE(o));

    if (nbi->destruct) {
        if (t->flags & (uint32_t) type_flags::is_destructible) {
            if (t->flags & (uint32_t) type_flags::has_destruct)
                t->destruct(inst_ptr(nbi));
        } else {
            fail("nanobind::detail::nb_inst_destruct(\"%s\"): attempted to call "
                 "the destructor of a non-destructible type!", t->name);
        }
        nbi->destruct = false;
    }

    nbi->ready = false;
}

void nb_inst_copy(PyObject *dst, const PyObject *src) noexcept {
    PyTypeObject *tp = Py_TYPE((PyObject *) src);
    type_data *t = nb_type_data(tp);

    if (tp != Py_TYPE(dst) ||
        (t->flags & (uint32_t) type_flags::is_copy_constructible) == 0)
        fail("nanobind::detail::nb_inst_copy(): invalid arguments!");

    nb_inst *nbi = (nb_inst *) dst;
    const void *src_data = inst_ptr((nb_inst *) src);
    void *dst_data = inst_ptr(nbi);

    if (t->flags & (uint32_t) type_flags::has_copy)
        t->copy(dst_data, src_data);
    else
        memcpy(dst_data, src_data, t->size);

    nbi->ready = nbi->destruct = true;
}

void nb_inst_move(PyObject *dst, const PyObject *src) noexcept {
    PyTypeObject *tp = Py_TYPE((PyObject *) src);
    type_data *t = nb_type_data(tp);

    if (tp != Py_TYPE(dst) ||
        (t->flags & (uint32_t) type_flags::is_move_constructible) == 0)
        fail("nanobind::detail::nb_inst_move(): invalid arguments!");

    nb_inst *nbi = (nb_inst *) dst;
    void *src_data = inst_ptr((nb_inst *) src);
    void *dst_data = inst_ptr(nbi);

    if (t->flags & (uint32_t) type_flags::has_move) {
        t->move(dst_data, src_data);
    } else {
        memcpy(dst_data, src_data, t->size);
        memset(src_data, 0, t->size);
    }

    nbi->ready = nbi->destruct = true;
}

#if defined(Py_LIMITED_API)
static size_t type_basicsize = 0;
type_data *nb_type_data_static(PyTypeObject *o) noexcept {
    if (type_basicsize == 0)
        type_basicsize = cast<size_t>(handle(&PyType_Type).attr("__basicsize__"));
    return (type_data *) (((char *) o) + type_basicsize);
}
#endif

/// Fetch the name of an instance as 'char *' (must be deallocated using 'free'!)
PyObject *nb_type_name(PyTypeObject *tp) noexcept {
    PyObject *name = PyObject_GetAttrString((PyObject *) tp, "__name__");

    if (PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE)) {
        PyObject *mod      = PyObject_GetAttrString((PyObject *) tp, "__module__"),
                 *combined = PyUnicode_FromFormat("%U.%U", mod, name);

        Py_DECREF(mod);
        Py_DECREF(name);
        name = combined;
    }

    return name;
}


NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
