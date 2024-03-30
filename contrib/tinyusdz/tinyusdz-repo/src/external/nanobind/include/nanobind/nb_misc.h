/*
    nanobind/nb_misc.h: Miscellaneous bits (GIL, etc.)

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)

struct gil_scoped_acquire {
public:
    gil_scoped_acquire() noexcept : state(PyGILState_Ensure()) { }
    ~gil_scoped_acquire() { PyGILState_Release(state); }

private:
    const PyGILState_STATE state;
};

class gil_scoped_release {
public:
    gil_scoped_release() noexcept : state(PyEval_SaveThread()) { }
    ~gil_scoped_release() { PyEval_RestoreThread(state); }

private:
    PyThreadState *state;
};

// Deleter for std::unique_ptr<T> (handles ownership by both C++ and Python)
template <typename T> struct deleter {
    /// Instance should be cleared using a delete expression
    deleter()  = default;

    /// Instance owned by Python, reduce reference count upon deletion
    deleter(handle h) : o(h.ptr()) { }

    /// Does Python own storage of the underlying object
    bool owned_by_python() const { return o != nullptr; }

    /// Does C++ own storage of the underlying object
    bool owned_by_cpp() const { return o == nullptr; }

    /// Perform the requested deletion operation
    void operator()(void *p) noexcept {
        if (o) {
            gil_scoped_acquire guard;
            Py_DECREF(o);
        } else {
            delete (T *) p;
        }
    }

    PyObject *o{nullptr};
};

NAMESPACE_END(NB_NAMESPACE)
