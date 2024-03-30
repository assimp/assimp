#include <nanobind/tensor.h>
#include <atomic>
#include "nb_internals.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

// ========================================================================

struct managed_tensor {
    dlpack::tensor dl_tensor;
    void *manager_ctx;
    void (*deleter)(managed_tensor *);
};

struct tensor_handle {
    managed_tensor *tensor;
    std::atomic<size_t> refcount;
    PyObject *owner;
    bool free_shape;
    bool free_strides;
    bool call_deleter;
};

PyObject *nb_tensor_new(PyTypeObject *tp, PyObject *args,
                        PyObject *kwargs) {

    allocfunc tp_alloc;
#if defined(Py_LIMITED_API)
    tp_alloc = (allocfunc) PyType_GetSlot(tp, Py_tp_alloc);
#else
    tp_alloc = tp->tp_alloc;
#endif

    PyObject* result = tp_alloc(tp, 0);
    if (NB_TUPLE_GET_SIZE(args) != 1 || kwargs)
        fail("nanobind::detail::nb_tensor_new(): internal error!");

    PyObject *capsule = NB_TUPLE_GET_ITEM(args, 0);
    ((nb_tensor *) result)->capsule = capsule;
    Py_INCREF(capsule);
    return result;
}

void nb_tensor_dealloc(PyObject *self) {
    Py_DECREF(((nb_tensor *) self)->capsule);

    freefunc tp_free;
#if defined(Py_LIMITED_API)
    tp_free = (freefunc) PyType_GetSlot(Py_TYPE(self), Py_tp_free);
#else
    tp_free = Py_TYPE(self)->tp_free;
#endif

    tp_free(self);
}

PyObject *nb_tensor_get(PyObject *self, PyObject *) {
    PyObject *result = ((nb_tensor *) self)->capsule;
    Py_INCREF(result);
    return result;
}

int nb_tensor_getbuffer(PyObject *exporter, Py_buffer *view, int) {
    nb_tensor *self = (nb_tensor *) exporter;

    void *ptr = PyCapsule_GetPointer(self->capsule, "dltensor");
    if (!ptr)
        fail("nanobind::tensor::nb_tensor_getbuffer(): internal error!");

    dlpack::tensor &t = ((managed_tensor *) ptr)->dl_tensor;

    if (t.device.device_type != device::cpu::value) {
        PyErr_SetString(PyExc_BufferError, "Only CPU-allocated tensors can be "
                                           "accessed via the buffer protocol!");
        return -1;
    }

    const char *format = nullptr;
    switch ((dlpack::dtype_code) t.dtype.code) {
        case dlpack::dtype_code::Int:
            switch (t.dtype.bits) {
                case 8: format = "b"; break;
                case 16: format = "h"; break;
                case 32: format = "i"; break;
                case 64: format = "q"; break;
            }
            break;

        case dlpack::dtype_code::UInt:
            switch (t.dtype.bits) {
                case 8: format = "B"; break;
                case 16: format = "H"; break;
                case 32: format = "I"; break;
                case 64: format = "Q"; break;
            }
            break;

        case dlpack::dtype_code::Float:
            switch (t.dtype.bits) {
                case 16: format = "e"; break;
                case 32: format = "f"; break;
                case 64: format = "d"; break;
            }
            break;

        default:
            break;
    }

    if (!format || t.dtype.lanes != 1) {
        PyErr_SetString(
            PyExc_BufferError,
            "Don't know how to convert DLPack dtype into buffer protocol format!");
        return -1;
    }

    view->format = (char *) format;
    view->itemsize = t.dtype.bits / 8;
    view->buf = (void *) ((uintptr_t) t.data + t.byte_offset);
    view->obj = exporter;
    Py_INCREF(exporter);

    Py_ssize_t len = view->itemsize;
    scoped_pymalloc<Py_ssize_t> strides(t.ndim), shape(t.ndim);
    for (int32_t i = 0; i < t.ndim; ++i) {
        len *= (Py_ssize_t) t.shape[i];
        strides[i] = (Py_ssize_t) t.strides[i] * view->itemsize;
        shape[i] = (Py_ssize_t) t.shape[i];
    }

    view->ndim = t.ndim;
    view->len = len;
    view->readonly = false;
    view->suboffsets = nullptr;
    view->internal = nullptr;
    view->strides = strides.release();
    view->shape = shape.release();

    return 0;
}

void nb_tensor_releasebuffer(PyObject *, Py_buffer *view) {
    PyMem_Free(view->shape);
    PyMem_Free(view->strides);
}

static PyObject *dlpack_from_buffer_protocol(PyObject *o) {
    scoped_pymalloc<Py_buffer> view;
    scoped_pymalloc<managed_tensor> mt;

    if (PyObject_GetBuffer(o, view.get(), PyBUF_RECORDS)) {
        PyErr_Clear();
        return nullptr;
    }

    char format = 'B';
    const char *format_str = view->format;
    if (format_str)
        format = *format_str;

    bool skip_first = format == '@' || format == '=';

    int32_t num = 1;
    if(*(uint8_t *) &num == 1) {
        if (format == '<')
            skip_first = true;
    } else {
        if (format == '!' || format == '>')
            skip_first = true;
    }

    if (skip_first && format_str)
        format = *++format_str;

    dlpack::dtype dt { };
    bool fail = format_str && format_str[1] != '\0';

    if (!fail) {
        switch (format) {
            case 'c':
            case 'b':
            case 'h':
            case 'i':
            case 'l':
            case 'q':
            case 'n': dt.code = (uint8_t) dlpack::dtype_code::Int; break;

            case 'B':
            case 'H':
            case 'I':
            case 'L':
            case 'Q':
            case 'N': dt.code = (uint8_t) dlpack::dtype_code::UInt; break;

            case 'e':
            case 'f':
            case 'd': dt.code = (uint8_t) dlpack::dtype_code::Float; break;

            default:
                fail = true;
        }
        dt.lanes = 1;
        dt.bits = (uint8_t) (view->itemsize * 8);
    }

    if (fail) {
        PyBuffer_Release(view.get());
        return nullptr;
    }

    mt->deleter = [](managed_tensor *mt2) {
        gil_scoped_acquire guard;
        Py_buffer *buf = (Py_buffer *) mt2->manager_ctx;
        PyBuffer_Release(buf);
        PyMem_Free(mt2->dl_tensor.shape);
        PyMem_Free(mt2->dl_tensor.strides);
        PyMem_Free(mt2);
    };

    /* DLPack mandates 256-byte alignment of the 'DLTensor::data' field, but
       PyTorch unfortunately ignores the 'byte_offset' value.. :-( */
#if 0
    uintptr_t value_int = (uintptr_t) view->buf,
              value_rounded = (value_int / 256) * 256;
#else
    uintptr_t value_int = (uintptr_t) view->buf,
              value_rounded = value_int;
#endif

    mt->dl_tensor.data = (void *) value_rounded;
    mt->dl_tensor.device = { device::cpu::value, 0 };
    mt->dl_tensor.ndim = view->ndim;
    mt->dl_tensor.dtype = dt;
    mt->dl_tensor.byte_offset = value_int - value_rounded;

    scoped_pymalloc<int64_t> strides(view->ndim);
    scoped_pymalloc<int64_t> shape(view->ndim);
    for (size_t i = 0; i < (size_t) view->ndim; ++i) {
        strides[i] = (int64_t) (view->strides[i] / view->itemsize);
        shape[i] = (int64_t) view->shape[i];
    }

    mt->manager_ctx = view.release();
    mt->dl_tensor.shape = shape.release();
    mt->dl_tensor.strides = strides.release();

    return PyCapsule_New(mt.release(), "dltensor", [](PyObject *o) {
        error_scope scope; // temporarily save any existing errors
        managed_tensor *mt =
            (managed_tensor *) PyCapsule_GetPointer(o, "dltensor");
        if (mt) {
            if (mt->deleter)
                mt->deleter(mt);
        } else {
            PyErr_Clear();
        }
    });
}

tensor_handle *tensor_import(PyObject *o, const tensor_req *req,
                             bool convert) noexcept {
    object capsule;

    // If this is not a capsule, try calling o.__dlpack__()
    if (!PyCapsule_CheckExact(o)) {
        capsule = steal(PyObject_CallMethod(o, "__dlpack__", nullptr));

        if (!capsule.is_valid()) {
            PyErr_Clear();
            PyTypeObject *tp = Py_TYPE(o);

            try {
                const char *module_name =
                    borrow<str>(handle(tp).attr("__module__")).c_str();

                object package;
                if (strncmp(module_name, "tensorflow.", 11) == 0)
                    package = module_::import_("tensorflow.experimental.dlpack");
                else if (strcmp(module_name, "torch") == 0)
                    package = module_::import_("torch.utils.dlpack");
                else if (strncmp(module_name, "jaxlib", 6) == 0)
                    package = module_::import_("jax.dlpack");

                if (package.is_valid())
                    capsule = package.attr("to_dlpack")(handle(o));
            } catch (...) {
                capsule.clear();
            }
        }

        // Try creating a tensor via the buffer protocol
        if (!capsule.is_valid())
            capsule = steal(dlpack_from_buffer_protocol(o));

        if (!capsule.is_valid())
            return nullptr;
    } else {
        capsule = borrow(o);
    }

    // Extract the pointer underlying the capsule
    void *ptr = PyCapsule_GetPointer(capsule.ptr(), "dltensor");
    if (!ptr) {
        PyErr_Clear();
        return nullptr;
    }

    // Check if the tensor satisfies the requirements
    dlpack::tensor &t = ((managed_tensor *) ptr)->dl_tensor;

    bool pass_dtype = true, pass_device = true,
         pass_shape = true, pass_order = true;

    if (req->req_dtype)
        pass_dtype = t.dtype == req->dtype;

    if (req->req_device)
        pass_device = t.device.device_type == req->req_device;

    if (req->req_shape) {
        pass_shape &= req->ndim == (uint32_t) t.ndim;

        if (pass_shape) {
            for (uint32_t i = 0; i < req->ndim; ++i) {
                if (req->shape[i] != (size_t) t.shape[i] &&
                    req->shape[i] != nanobind::any) {
                    pass_shape = false;
                    break;
                }
            }
        }
    }

    scoped_pymalloc<int64_t> strides(t.ndim);
    if ((req->req_order || t.strides == nullptr) && t.ndim > 0) {
        size_t accum = 1;

        if (req->req_order == 'C' || t.strides == nullptr) {
            for (uint32_t i = (uint32_t) (t.ndim - 1);;) {
                strides[i] = accum;
                accum *= t.shape[i];
                if (i == 0)
                    break;
                --i;
            }
        } else if (req->req_order == 'F') {
            pass_order &= t.strides != nullptr;

            for (uint32_t i = 0; i < (uint32_t) t.ndim; ++i) {
                strides[i] = accum;
                accum *= t.shape[i];
            }
        } else {
            pass_order = false;
        }

        if (t.strides) {
            for (uint32_t i = 0; i < (uint32_t) t.ndim; ++i) {
                if (strides[i] != t.strides[i]) {
                    pass_order = false;
                    break;
                }
            }
        }
    }

    // Support implicit conversion of 'dtype' and order
    if (pass_device && pass_shape && (!pass_dtype || !pass_order) && convert &&
        capsule.ptr() != o) {
        PyTypeObject *tp = Py_TYPE(o);
        str module_name_o = borrow<str>(handle(tp).attr("__module__"));
        const char *module_name = module_name_o.c_str();

        char order = 'K';
        if (req->req_order != '\0')
            order = req->req_order;

        if (req->dtype.lanes != 1)
            return nullptr;

        const char *prefix = nullptr;
        char dtype[8];
        switch (req->dtype.code) {
            case (uint8_t) dlpack::dtype_code::Int: prefix = "int"; break;
            case (uint8_t) dlpack::dtype_code::UInt: prefix = "uint"; break;
            case (uint8_t) dlpack::dtype_code::Float: prefix = "float"; break;
            default:
                return nullptr;
        }
        snprintf(dtype, sizeof(dtype), "%s%u", prefix, req->dtype.bits);

        object converted;
        try {
            if (strcmp(module_name, "numpy") == 0) {
                converted = handle(o).attr("astype")(dtype, order);
            } else if (strcmp(module_name, "torch") == 0) {
                converted = handle(o).attr("to")(
                    arg("dtype") = module_::import_("torch").attr(dtype),
                    arg("copy") = true
                );
            } else if (strncmp(module_name, "tensorflow.", 11) == 0) {
                converted = module_::import_("tensorflow")
                                .attr("cast")(handle(o), dtype);
            } else if (strncmp(module_name, "jaxlib", 6) == 0) {
                converted = handle(o).attr("astype")(dtype);
            }
        } catch (...) { converted.clear(); }

        // Potentially try again recursively
        if (!converted.is_valid())
            return nullptr;
        else
            return tensor_import(converted.ptr(), req, false);
    }

    if (!pass_dtype || !pass_device || !pass_shape || !pass_order)
        return nullptr;

    // Create a reference-counted wrapper
    scoped_pymalloc<tensor_handle> result;
    result->tensor = (managed_tensor *) ptr;
    result->refcount = 0;
    result->owner = nullptr;
    result->free_shape = false;
    result->call_deleter = true;

    // Ensure that the strides member is always initialized
    if (t.strides) {
        result->free_strides = false;
    } else {
        result->free_strides = true;
        t.strides = strides.release();
    }

    // Mark the dltensor capsule as "consumed"
    if (PyCapsule_SetName(capsule.ptr(), "used_dltensor") ||
        PyCapsule_SetDestructor(capsule.ptr(), nullptr))
        fail("nanobind::detail::tensor_import(): could not mark dltensor "
             "capsule as consumed!");

    return result.release();
}

dlpack::tensor *tensor_inc_ref(tensor_handle *th) noexcept {
    if (!th)
        return nullptr;
    ++th->refcount;
    return &th->tensor->dl_tensor;
}

void tensor_dec_ref(tensor_handle *th) noexcept {
    if (!th)
        return;
    size_t rc_value = th->refcount--;

    if (rc_value == 0) {
        fail("tensor_dec_ref(): reference count became negative!");
    } else if (rc_value == 1) {
        Py_XDECREF(th->owner);
        managed_tensor *mt = th->tensor;
        if (th->free_shape) {
            PyMem_Free(mt->dl_tensor.shape);
            mt->dl_tensor.shape = nullptr;
        }
        if (th->free_strides) {
            PyMem_Free(mt->dl_tensor.strides);
            mt->dl_tensor.strides = nullptr;
        }
        if (th->call_deleter) {
            if (mt->deleter)
                mt->deleter(mt);
        } else {
            PyMem_Free(mt);
        }
        PyMem_Free(th);
    }
}

tensor_handle *tensor_create(void *value, size_t ndim, const size_t *shape_in,
                            PyObject *owner, const int64_t *strides_in,
                            dlpack::dtype *dtype, int32_t device_type,
                            int32_t device_id) {
    /* DLPack mandates 256-byte alignment of the 'DLTensor::data' field, but
       PyTorch unfortunately ignores the 'byte_offset' value.. :-( */
#if 0
    uintptr_t value_int = (uintptr_t) value,
              value_rounded = (value_int / 256) * 256;
#else
    uintptr_t value_int = (uintptr_t) value,
              value_rounded = value_int;
#endif


    scoped_pymalloc<managed_tensor> tensor;
    scoped_pymalloc<tensor_handle> result;
    scoped_pymalloc<int64_t> shape(ndim), strides(ndim);

    auto deleter = [](managed_tensor *mt) {
        gil_scoped_acquire guard;
        tensor_handle *th = (tensor_handle *) mt->manager_ctx;
        tensor_dec_ref(th);
    };

    for (size_t i = 0; i < ndim; ++i)
        shape[i] = (int64_t) shape_in[i];

    int64_t prod = 1;
    for (size_t i = ndim - 1; ;) {
        if (strides_in) {
            strides[i] = strides_in[i];
        } else {
            strides[i] = prod;
            prod *= (int64_t) shape_in[i];
        }
        if (i == 0)
            break;
        --i;
    }

    tensor->dl_tensor.data = (void *) value_rounded;
    tensor->dl_tensor.device.device_type = device_type;
    tensor->dl_tensor.device.device_id = device_id;
    tensor->dl_tensor.ndim = (int32_t) ndim;
    tensor->dl_tensor.dtype = *dtype;
    tensor->dl_tensor.byte_offset = value_int - value_rounded;
    tensor->dl_tensor.shape = shape.release();
    tensor->dl_tensor.strides = strides.release();
    tensor->manager_ctx = result.get();
    tensor->deleter = deleter;
    result->tensor = (managed_tensor *) tensor.release();
    result->refcount = 0;
    result->owner = owner;
    result->free_shape = true;
    result->free_strides = true;
    result->call_deleter = false;
    Py_XINCREF(owner);
    return result.release();
}

static void tensor_capsule_destructor(PyObject *o) {
    error_scope scope; // temporarily save any existing errors
    managed_tensor *mt =
        (managed_tensor *) PyCapsule_GetPointer(o, "dltensor");
    if (mt)
        tensor_dec_ref((tensor_handle *) mt->manager_ctx);
    else
        PyErr_Clear();
}

PyObject *tensor_wrap(tensor_handle *th, int framework) noexcept {
    tensor_inc_ref(th);
    object o = steal(PyCapsule_New(th->tensor, "dltensor", tensor_capsule_destructor)),
           package;

    switch ((tensor_framework) framework) {
        case tensor_framework::none:
            break;

        case tensor_framework::numpy:
            package = module_::import_("numpy");
            o = handle(internals_get().nb_tensor)(o);
            break;

        case tensor_framework::pytorch:
            package = module_::import_("torch.utils.dlpack");
            break;


        case tensor_framework::tensorflow:
            package = module_::import_("tensorflow.experimental.dlpack");
            break;

        case tensor_framework::jax:
            package = module_::import_("jax.dlpack");
            break;


        default:
            fail("nanobind::detail::tensor_wrap(): unknown framework "
                 "specified!");
    }


    if (package.is_valid()) {
        try {
            o = package.attr("from_dlpack")(o);
        } catch (...) {
            if ((tensor_framework) framework == tensor_framework::numpy) {
                try {
                    // Older numpy versions
                    o = package.attr("_from_dlpack")(o);
                } catch (...) {
                    try {
                        // Yet older numpy versions
                        o = package.attr("asarray")(o);
                    } catch (...) {
                        return nullptr;
                    }
                }
            } else {
                return nullptr;
            }
        }
    }

    return o.release().ptr();
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
