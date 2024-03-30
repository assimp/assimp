/*
    nanobind/nb_defs.h: Preprocessor definitions used by the project

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#define NB_STRINGIFY(x) #x
#define NB_TOSTRING(x) NB_STRINGIFY(x)
#define NB_CONCAT(first, second) first##second
#define NB_NEXT_OVERLOAD ((PyObject *) 1) // special failure return code

#if !defined(NAMESPACE_BEGIN)
#  define NAMESPACE_BEGIN(name) namespace name {
#endif

#if !defined(NAMESPACE_END)
#  define NAMESPACE_END(name) }
#endif

#if defined(_WIN32)
#  define NB_EXPORT        __declspec(dllexport)
#  define NB_IMPORT        __declspec(dllimport)
#  define NB_INLINE        __forceinline
#  define NB_INLINE_LAMBDA
#  define NB_NOINLINE      __declspec(noinline)
# define  NB_STRDUP        _strdup
#else
#  define NB_EXPORT        __attribute__ ((visibility("default")))
#  define NB_IMPORT        __attribute__ ((visibility("default")))
#  define NB_INLINE        inline __attribute__((always_inline))
#  define NB_NOINLINE      __attribute__((noinline))
#if defined(__clang__)
#    define NB_INLINE_LAMBDA __attribute__((always_inline))
#else
#    define NB_INLINE_LAMBDA
#endif
#  define NB_STRDUP        strdup
#endif

#if defined(__GNUC__)
#  define NB_NAMESPACE nanobind __attribute__((visibility("hidden")))
#else
#  define NB_NAMESPACE nanobind
#endif

#if defined(NB_SHARED)
#  if defined(NB_BUILD)
#    define NB_CORE NB_EXPORT
#  else
#    define NB_CORE NB_IMPORT
#  endif
#else
#  if defined(_WIN32)
#    define NB_CORE
#  else
#    define NB_CORE NB_EXPORT
#  endif
#endif

#if defined(__cpp_lib_char8_t) && __cpp_lib_char8_t >= 201811L
#  define NB_HAS_U8STRING
#endif

#if defined(Py_TPFLAGS_HAVE_VECTORCALL)
#  define NB_VECTORCALL PyObject_Vectorcall
#  define NB_HAVE_VECTORCALL Py_TPFLAGS_HAVE_VECTORCALL
#elif defined(_Py_TPFLAGS_HAVE_VECTORCALL)
#  define NB_VECTORCALL _PyObject_Vectorcall
#  define NB_HAVE_VECTORCALL _Py_TPFLAGS_HAVE_VECTORCALL
#else
#  define NB_HAVE_VECTORCALL (1UL << 11)
#endif

#if defined(PY_VECTORCALL_ARGUMENTS_OFFSET)
#  define NB_VECTORCALL_ARGUMENTS_OFFSET PY_VECTORCALL_ARGUMENTS_OFFSET
#  define NB_VECTORCALL_NARGS PyVectorcall_NARGS
#else
#  define NB_VECTORCALL_ARGUMENTS_OFFSET ((size_t) 1 << (8 * sizeof(size_t) - 1))
#  define NB_VECTORCALL_NARGS(n) ((n) & ~NB_VECTORCALL_ARGUMENTS_OFFSET)
#endif

#if defined(Py_LIMITED_API)
#  define NB_TUPLE_GET_SIZE PyTuple_Size
#  define NB_TUPLE_GET_ITEM PyTuple_GetItem
#  define NB_TUPLE_SET_ITEM PyTuple_SetItem
#  define NB_LIST_GET_SIZE PyList_Size
#  define NB_LIST_GET_ITEM PyList_GetItem
#  define NB_LIST_SET_ITEM PyList_SetItem
#  define NB_DICT_GET_SIZE PyDict_Size
#else
#  define NB_TUPLE_GET_SIZE PyTuple_GET_SIZE
#  define NB_TUPLE_GET_ITEM PyTuple_GET_ITEM
#  define NB_TUPLE_SET_ITEM PyTuple_SET_ITEM
#  define NB_LIST_GET_SIZE PyList_GET_SIZE
#  define NB_LIST_GET_ITEM PyList_GET_ITEM
#  define NB_LIST_SET_ITEM PyList_SET_ITEM
#  define NB_DICT_GET_SIZE PyDict_GET_SIZE
#endif


#define NB_MODULE(name, variable)                                              \
    extern "C" [[maybe_unused]] NB_EXPORT PyObject *PyInit_##name();           \
    static PyModuleDef NB_CONCAT(nanobind_module_def_, name);                  \
    [[maybe_unused]] static void NB_CONCAT(nanobind_init_,                     \
                                           name)(::nanobind::module_ &);       \
    extern "C" NB_EXPORT PyObject *PyInit_##name() {                           \
        nanobind::module_ m =                                                  \
            nanobind::borrow<nanobind::module_>(nanobind::detail::module_new(  \
                NB_TOSTRING(name), &NB_CONCAT(nanobind_module_def_, name)));   \
        try {                                                                  \
            NB_CONCAT(nanobind_init_, name)(m);                                \
            return m.ptr();                                                    \
        } catch (const std::exception &e) {                                    \
            PyErr_SetString(PyExc_ImportError, e.what());                      \
            return nullptr;                                                    \
        }                                                                      \
    }                                                                          \
    void NB_CONCAT(nanobind_init_, name)(::nanobind::module_ & (variable))

