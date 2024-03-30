/*
    nanobind/nb_python.h: Include CPython headers while temporarily disabling
    certain warnings. Also, disable dangerous preprocessor definitions.

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

/// Include Python header, disable linking to pythonX_d.lib on Windows in debug mode

#if defined(_MSC_VER)
#  pragma warning(push)
#  if defined(_DEBUG) && !defined(Py_DEBUG)
#    define NB_DEBUG_MARKER
#    undef _DEBUG
#  endif
#endif

#include <Python.h>
#include <frameobject.h>
#include <pythread.h>
#include <structmember.h>

/* Python #defines overrides on all sorts of core functions, which
   tends to weak havok in C++ codebases that expect these to work
   like regular functions (potentially with several overloads) */
#if defined(isalnum)
#  undef isalnum
#  undef isalpha
#  undef islower
#  undef isspace
#  undef isupper
#  undef tolower
#  undef toupper
#endif

#if defined(copysign)
#  undef copysign
#endif

#if defined(setter)
#  undef setter
#endif

#if defined(getter)
#  undef getter
#endif

#if defined(_MSC_VER)
#  if defined(NB_DEBUG_MARKER)
#    define _DEBUG
#    undef NB_DEBUG_MARKER
#  endif
#  pragma warning(pop)
#endif

#if PY_VERSION_HEX < 0x03080000
#  error The nanobind library requires Python 3.8 (or newer)
#endif
