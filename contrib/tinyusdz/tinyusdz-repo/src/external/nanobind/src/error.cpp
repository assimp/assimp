/*
    src/error.cpp: libnanobind functionality for exceptions

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include <nanobind/nanobind.h>
#include "buffer.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

Buffer buf(128);

NAMESPACE_END(detail)

python_error::python_error() {
    PyErr_Fetch(&m_type.m_ptr, &m_value.m_ptr, &m_trace.m_ptr);
}

python_error::~python_error() {
    free(m_what);
}

python_error::python_error(const python_error &e) : std::exception(e),
    m_type{e.m_type},
    m_value{e.m_value},
    m_trace{e.m_trace} {
    if (e.m_what)
        m_what = NB_STRDUP(e.m_what);
}

python_error::python_error(python_error &&e) noexcept : std::exception(e),
    m_type{std::move(e.m_type)},
    m_value{std::move(e.m_value)},
    m_trace{std::move(e.m_trace)} {
    std::swap(m_what, e.m_what);
}

const char *python_error::what() const noexcept {
    if (m_what)
        return m_what;

    using detail::buf;

    buf.clear();

    if (m_type.is_valid()) {
        object name = m_type.attr("__name__");
        buf.put_dstr(borrow<str>(name).c_str());
        buf.put(": ");
    }

    if (m_value.is_valid())
        buf.put_dstr(str(m_value).c_str());

#if !defined(Py_LIMITED_API)
    if (m_trace.is_valid()) {
        PyTracebackObject *to = (PyTracebackObject *) m_trace.ptr();

        // Get the deepest trace possible
        while (to->tb_next)
            to = to->tb_next;

        PyFrameObject *frame = to->tb_frame;
#if PY_VERSION_HEX >= 0x03090000
        Py_XINCREF(frame);
#endif

        buf.put("\n\nAt:\n");
        while (frame) {
#if PY_VERSION_HEX >= 0x03090000
            PyCodeObject *f_code = PyFrame_GetCode(frame);
#else
            PyCodeObject *f_code = frame->f_code;
            Py_INCREF(f_code);
#endif
            buf.put_dstr(borrow<str>(f_code->co_filename).c_str());
            buf.put('(');
            buf.put_uint32(PyFrame_GetLineNumber(frame));
            buf.put("): ");
            buf.put_dstr(borrow<str>(f_code->co_name).c_str());
            buf.put('\n');

#if PY_VERSION_HEX >= 0x03090000
            PyFrameObject *frame_new = PyFrame_GetBack(frame);
            Py_DECREF(frame);
            frame = frame_new;
#else
            frame = frame->f_back;
#endif
           Py_DECREF(f_code);
        }
    }
#endif

    m_what = buf.copy();
    return m_what;
}

void python_error::restore() {
    PyErr_Restore(m_type.release().ptr(), m_value.release().ptr(),
                  m_trace.release().ptr());
}

next_overload::next_overload() : std::exception() { }
next_overload::~next_overload() = default;

#define NB_EXCEPTION(name, type)                                               \
    name::name() : builtin_exception("") { }                                   \
    void name::set_error() const { PyErr_SetString(type, what()); }

NB_EXCEPTION(stop_iteration, PyExc_StopIteration)
NB_EXCEPTION(index_error, PyExc_IndexError)
NB_EXCEPTION(key_error, PyExc_KeyError)
NB_EXCEPTION(value_error, PyExc_ValueError)
NB_EXCEPTION(type_error, PyExc_TypeError)
NB_EXCEPTION(buffer_error, PyExc_BufferError)
NB_EXCEPTION(import_error, PyExc_ImportError)
NB_EXCEPTION(attribute_error, PyExc_AttributeError)

#undef NB_EXCEPTION

NAMESPACE_END(NB_NAMESPACE)
