/*
    nanobind/nb_error.h: Python exception handling, binding of exceptions

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)

/// RAII wrapper that temporarily clears any Python error state
struct error_scope {
    PyObject *type, *value, *trace;
    error_scope() { PyErr_Fetch(&type, &value, &trace); }
    ~error_scope() { PyErr_Restore(type, value, trace); }
};

/// Wraps a Python error state as a C++ exception
class NB_EXPORT python_error : public std::exception {
public:
    python_error();
    python_error(const python_error &);
    python_error(python_error &&) noexcept;
    ~python_error() override;

    /// Move the error back into the Python domain
    void restore();

    const handle type() const { return m_type; }
    const handle value() const { return m_value; }
    const handle trace() const { return m_trace; }

    const char *what() const noexcept override;

private:
    object m_type, m_value, m_trace;
    mutable char *m_what = nullptr;
};

/// Throw from a bound method to skip to the next overload
class NB_EXPORT next_overload : public std::exception {
public:
    next_overload();
    ~next_overload() override;
};

// Base interface used to expose common Python exceptions in C++
class NB_EXPORT builtin_exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    virtual void set_error() const = 0;
};

#define NB_EXCEPTION(type)                                          \
    class NB_EXPORT type : public builtin_exception {               \
    public:                                                         \
        using builtin_exception::builtin_exception;                 \
        type();                                                     \
        void set_error() const override;                            \
    };

NB_EXCEPTION(stop_iteration)
NB_EXCEPTION(index_error)
NB_EXCEPTION(key_error)
NB_EXCEPTION(value_error)
NB_EXCEPTION(type_error)
NB_EXCEPTION(buffer_error)
NB_EXCEPTION(import_error)
NB_EXCEPTION(attribute_error)

#undef NB_EXCEPTION

NAMESPACE_END(NB_NAMESPACE)
