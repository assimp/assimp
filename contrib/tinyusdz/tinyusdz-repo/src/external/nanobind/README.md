# nanobind — Seamless operability between C++17 and Python

[![Continuous Integration](https://github.com/wjakob/nanobind/actions/workflows/ci.yml/badge.svg)](https://github.com/wjakob/nanobind/actions/workflows/ci.yml)
[![](https://img.shields.io/pypi/v/nanobind.svg)](https://pypi.org/pypi/nanobind/)
![](https://img.shields.io/pypi/l/nanobind.svg)
[![](https://img.shields.io/badge/Example_project-Link-green)](https://github.com/wjakob/nanobind_example)

_nanobind_ is a small binding library that exposes C++ types in Python and vice
versa. It is reminiscent of
_[Boost.Python](https://www.boost.org/doc/libs/1_64_0/libs/python/doc/html)_
and _[pybind11](http://github.com/pybind/pybind11)_ and uses near-identical
syntax. In contrast to these existing tools, _nanobind_ is more _efficient_:
bindings compile in a shorter amount of time, producing smaller binaries with
better runtime performance.

## Why _yet another_ binding library?

I started the _[pybind11](http://github.com/pybind/pybind11)_ project back in
2015 to generate better and more efficient C++/Python bindings. Thanks to many
amazing contributions by others, _pybind11_ has become a core dependency of
software across the world including flagship projects like PyTorch and
Tensorflow. Every day, the repository is cloned more than 100.000 times.
Hundreds of contributed extensions and generalizations address use cases of
this diverse audience. However, all of this success also came with _costs_: the
complexity of the library grew tremendously, which had a negative impact on
efficiency.

Ironically, the situation today feels like 2015 all over again: binding
generation with existing tools (_Boost.Python_, _pybind11_) is slow and
produces enormous binaries with overheads on runtime performance. At the same
time, key improvements in C++17 and Python 3.8 provide opportunities for
drastic simplifications. Therefore, I am starting _another_ binding project..
This time, the scope is intentionally limited so that this doesn't turn into an
endless cycle.

## Performance

> **TLDR**: _nanobind_ bindings compile ~2-3× faster, producing
~3× smaller binaries, with up to ~8× lower overheads on runtime performance
(when comparing to _pybind11_ with `-Os` size optimizations).

The following experiments analyze the performance of a very large
function-heavy (`func`) and class-heavy (`class`) binding microbenchmark
compiled using _Boost.Python_, _pybind11_, and _nanobind_ in both `debug` and
size-optimized (`opt`) modes.
A comparison with [cppyy](https://cppyy.readthedocs.io/en/latest/) (which uses
dynamic compilation) is also shown later.
Details on the experimental setup can be found
[here](https://github.com/wjakob/nanobind/blob/master/docs/benchmark.md).

The first plot contrasts the **compilation time**, where "_number_ ×"
annotations denote the amount of time spent relative to _nanobind_. As shown
below, _nanobind_ achieves a consistent ~**2-3× improvement** compared to
_pybind11_.
<p align="center">
<img src="https://github.com/wjakob/nanobind/raw/master/docs/images/times.svg" alt="Compilation time benchmark" width="850"/>
</p>

_nanobind_ also greatly reduces the **binary size** of the compiled bindings.
There is a roughly **3× improvement** compared to _pybind11_ and a **8-9×
improvement** compared to _Boost.Python_ (both with size optimizations).
<p align="center">
<img src="https://github.com/wjakob/nanobind/raw/master/docs/images/sizes.svg" alt="Binary size benchmark" width="850"/>
</p>

The last experiment compares the **runtime performance overheads** by calling
one of the bound functions many times in a loop. Here, it is also interesting
to compare against [cppyy](https://cppyy.readthedocs.io/en/latest/) (gray bar)
and a pure Python implementation that runs bytecode without binding overheads
(hatched red bar).

<p align="center">
<img src="https://github.com/wjakob/nanobind/raw/master/docs/images/perf.svg" alt="Runtime performance benchmark" width="850"/>
</p>

This data shows that the overhead of calling a _nanobind_ function is lower
than that of an equivalent function call done within CPython. The functions
benchmarked here don't perform CPU-intensive work, so this this mainly measures
the overheads of performing a function call, boxing/unboxing arguments and
return values, etc.

The difference to _pybind11_ is _significant_: a ~**2× improvement** for simple
functions, and an **~8× improvement** when classes are being passed around.
Complexities in _pybind11_ related to overload resolution, multiple
inheritance, and holders are the main reasons for this difference. Those
features were either simplified or completely removed in _nanobind_.

Finally, there is a **~1.4× improvement** in both experiments compared
to _cppyy_ (please ignore the two `[debug]` columns—I did not feel
comfortable adjusting the JIT compilation flags; all _cppyy_ bindings
are therefore optimized.)

## What are technical differences between _nanobind_ and _cppyy_?

_cppyy_ is based on dynamic parsing of C++ code and *just-in-time* (JIT)
compilation of bindings via the LLVM compiler infrastructure. The authors of
_cppyy_ report that their tool produces bindings with much lower overheads
compared to _pybind11_, and the above plots show that this is indeed true.
However, _nanobind_ retakes the performance lead in these experiments.

With speed gone as the main differentiating factor, other qualitative
differences make these two tools appropriate to different audiences: _cppyy_
has its origin in CERN's ROOT mega-project and must be highly dynamic to work
with that codebase: it can parse header files to generate bindings as needed.
_cppyy_ works particularly well together with PyPy and can avoid
boxing/unboxing overheads with this combination. The main downside of _cppyy_
is that it depends on big and complex machinery (Cling/Clang/LLVM) that must
be deployed on the user's side and then run there. There isn't a way of
pre-generating bindings and then shipping just the output of this process.

_nanobind_ is relatively static in comparison: you must tell it which functions
to expose via binding declarations. These declarations offer a high degree of
flexibility that users will typically use to create bindings that feel
_pythonic_. At compile-time, those declarations turn into a sequence of CPython
API calls, which produces self-contained bindings that are easy to redistribute
via [PyPI](https://pypi.org) or elsewhere. Tools like
[cibuildwheel](https://cibuildwheel.readthedocs.io/en/stable/) and
[scikit-build](https://scikit-build.readthedocs.io/en/latest/index.html) can
fully automate the process of generating _Python wheels_ for each target
platform. A minimal [example
project](https://github.com/wjakob/nanobind_example) shows how to do this
automatically via [GitHub Actions](https://github.com/features/actions).

## What are technical differences between _nanobind_ and _pybind11_?

_nanobind_ and _pybind11_ are the most similar of all of the binding tools
compared above.

The main difference is between them is a change in philosophy: _pybind11_ must
deal with *all of C++* to bind complex legacy codebases, while _nanobind_
targets a smaller C++ subset. **The codebase has to adapt to the binding tool
and not the other way around!**, which allows _nanobind_ to be simpler and
faster. Pull requests with extensions and generalizations were welcomed in
_pybind11_, but they will likely be rejected in this project.

An overview of removed features is provided in a [separate
document](https://github.com/wjakob/nanobind/blob/master/docs/removed.md).
Besides feature removal, the rewrite was also an opportunity to address
long-standing performance issues in _pybind11_:

- C++ objects are now co-located with the Python object whenever possible (less
  pointer chasing compared to _pybind11_). The per-instance overhead for
  wrapping a C++ type into a Python object shrinks by 2.3x. (_pybind11_: 56
  bytes, _nanobind_: 24 bytes.)

- C++ function binding information is now co-located with the Python function
  object (less pointer chasing).

- C++ type binding information is now co-located with the Python type object
  (less pointer chasing, fewer hashtable lookups).

- _nanobind_ internally replaces `std::unordered_map` with a more efficient
  hash table ([tsl::robin_map](https://github.com/Tessil/robin-map), which is
  included as a git submodule).

- function calls from/to Python are realized using [PEP 590 vector
  calls](https://www.python.org/dev/peps/pep-0590), which gives a nice speed
  boost. The main function dispatch loop no longer allocates heap memory.

- _pybind11_ was designed as a header-only library, which is generally a good
  thing because it simplifies the compilation workflow. However, one major
  downside of this is that a large amount of redundant code has to be compiled
  in each binding file (e.g., the function dispatch loop and all of the related
  internal data structures). _nanobind_ compiles a separate shared or static
  support library (`libnanobind`) and links it against the binding code to
  avoid redundant compilation. When using the CMake `nanobind_add_module()`
  function, this all happens transparently.

- `#include <pybind11/pybind11.h>` pulls in a large portion of the STL (about
  2.1 MiB of headers with Clang and libc++). _nanobind_ minimizes STL usage to
  avoid this problem. Type casters even for for basic types like `std::string`
  require an explicit opt-in by including an extra header file (e.g. `#include
  <nanobind/stl/string.h>`).

- _pybind11_ is dependent on *link time optimization* (LTO) to produce
  reasonably-sized bindings, which makes linking a build time bottleneck. With
  _nanobind_'s split into a precompiled core library and minimal
  metatemplating, LTO is no longer important.

- _nanobind_ maintains efficient internal data structures for lifetime
  management (needed for `nb::keep_alive`, `nb::rv_policy::reference_internal`,
  the `std::shared_ptr` interface, etc.). With these changes, it is no longer
  necessary that bound types are weak-referenceable, which saves a pointer per
  instance.

### Other improvements

Besides performance improvements, _nanobind_ includes a quality-of-live
improvements for developers:

- _nanobind_ has [greatly
  improved](https://github.com/wjakob/nanobind/blob/master/docs/tensor.md)
  support for exchanging CPU/GPU/TPU/.. tensor data structures with modern
  array programming frameworks.

- _nanobind_ can target Python's [stable ABI
  interface](https://docs.python.org/3/c-api/stable.html) starting with Python
  3.12. This means that extension modules will eventually be compatible with
  any future version of Python without having to compile separate binaries per
  version. That vision is still far out, however: it will require Python 3.12+
  to be widely deployed.

- When the python interpreter shuts down, _nanobind_ reports instance, type,
  and function leaks related to bindings, which is useful for tracking down
  reference counting issues.

- _nanobind_ deletes its internal data structures when the Python interpreter
  terminates, which avoids memory leak reports in tools like _valgrind_.

- In _pybind11_, function docstrings are pre-rendered while the binding code
  runs (`.def(...)`). This can create confusing signatures containing C++ types
  when the binding code of those C++ types hasn't yet run. _nanobind_ does not
  pre-render function docstrings: they are created on the fly when queried.

- _nanobind_ docstrings have improved out-of-the-box compatibility with tools
  like [Sphinx](https://www.sphinx-doc.org/en/master/).

### Dependencies

_nanobind_ depends on recent versions of everything:

- **C++17**: The `if constexpr` feature was crucial to simplify the internal
  meta-templating of this library.
- **Python 3.8+**: _nanobind_ heavily relies on [PEP 590 vector
  calls](https://www.python.org/dev/peps/pep-0590) that were introduced in
  version 3.8.
- **CMake 3.15+**: Recent CMake versions include important improvements to
  `FindPython` that this project depends on.
- **Supported compilers**: Clang 7, GCC 8, MSVC2019 (or newer) are officially
  supported.

  Other compilers like MinGW, Intel (icpc, oneAPI), NVIDIA (PGI, nvcc) may or
  may not work but aren't officially supported. Pull requests to work around
  bugs in these compilers will not be accepted, as similar changes introduced
  significant complexity in _pybind11_. Instead, please file bugs with the
  vendors so that they will fix their compilers.

### CMake interface

_nanobind_ integrates with CMake to simplify binding compilation. Please see
the [separate
writeup](https://github.com/wjakob/nanobind/blob/master/docs/cmake.md) for
details.

The easiest way to get started is by cloning
[`nanobind_example`](https://github.com/wjakob/nanobind_example), which is a
minimal project with _nanobind_-based bindings compiled via CMake and
[`scikit-build`](https://scikit-build.readthedocs.io/en/latest/). It also shows
how to use GitHub Actions to deploy binary wheels for a variety of platforms.

### API differences

_nanobind_ mostly follows the _pybind11_ API, hence the [pybind11
documentation](https://pybind11.readthedocs.io/en/stable) is the main source of
documentation for this project. A number of simplifications and noteworthy
changes are detailed below.

- **Namespace**. _nanobind_ types and functions are located in the `nanobind` namespace. The
  `namespace nb = nanobind;` shorthand alias is recommended.

- **Macros**. The `PYBIND11_*` macros (e.g., `PYBIND11_OVERRIDE(..)`) were
  renamed to `NB_*` (e.g., `NB_OVERRIDE(..)`).

- **Shared pointers and holders**. _nanobind_ removes the concept of a _holder
  type_, which caused inefficiencies and introduced complexity in _pybind11_.
  This has implications on object ownership, shared ownership, and interactions
  with C++ shared/unique pointers.

  Please see the following [separate page](docs/ownership.md) for the
  nitty-gritty details on shared and unique pointers. Classes with _intrusive_
  reference counting also continue to be supported, please see the [linked
  page](docs/intrusive.md) for details.

  The gist is that use of shared/unique pointers requires one or both of the
  following optional header files:

  - [`nanobind/stl/unique_ptr.h`](https://github.com/wjakob/nanobind/blob/master/include/nanobind/stl/unique_ptr.h)
  - [`nanobind/stl/shared_ptr.h`](https://github.com/wjakob/nanobind/blob/master/include/nanobind/stl/shared_ptr.h)

  Binding functions that take ``std::unique_ptr<T>`` arguments involves some
  limitations that can be avoided by changing their signatures to use
  ``std::unique_ptr<T, nb::deleter<T>>`` instead. Usage of
  ``std::enable_shared_from_this<T>`` is prohibited and will raise a
  compile-time assertion. This is consistent with the philosophy of this
  library: _the codebase has to adapt to the binding tool and not the other way
  around_.

  It is no longer necessary to specify holder types in the type declaration:

  _pybind11_:
  ```cpp
  py::class_<MyType, std::shared_ptr<MyType>>(m, "MyType")
    ...
  ```

  _nanobind_:
  ```cpp
  nb::class_<MyType>(m, "MyType")
    ...
  ```

- **Null pointers**. In contrast to _pybind11_, _nanobind_ by default does
  _not_ permit ``None``-valued arguments during overload resolution. They need
  to be enabled explicitly using the ``.none()`` member of an argument
  annotation.

  ```cpp
      .def("func", &func, "arg"_a.none());
  ```

  It is also possible to set a ``None`` default value as follows:

  ```cpp
      .def("func", &func, "arg"_a.none() = nb::none());
  ```

- **Implicit type conversions**. In _pybind11_, implicit conversions were
  specified using a follow-up function call. In _nanobind_, they are specified
  within the constructor declarations:

  _pybind11_:
  ```cpp
  py::class_<MyType>(m, "MyType")
      .def(py::init<MyOtherType>());

  py::implicitly_convertible<MyOtherType, MyType>();
  ```

  _nanobind_:
  ```cpp
  nb::class_<MyType>(m, "MyType")
      .def(nb::init_implicit<MyOtherType>());
  ```

- **Custom constructors**: In _pybind11_, custom constructors (i.e. ones that
  do not already exist in the C++ class) could be specified as lambda function
  returning an instance of the desired type.

  ```
  nb::class_<MyType>(m, "MyType")
      .def(nb::init([](int) { return MyType(...); }));

  ```

  Unfortunately, the implementation of this feature was quite complex and often
  required involved further internal calls to the move or copy constructor.
  *nanobind* instead reverts to how pybind11 originally implemented this
  feature using in-place construction ("[placement
  new](https://en.wikipedia.org/wiki/Placement_syntax)"):

  ```
  nb::class_<MyType>(m, "MyType")
      .def("__init__", [](MyType *t) { new (t) MyType(...); });
  ```

  The provided lambda function will be called with a pointer to uninitialized
  memory that has already been allocated (this memory region is co-located with
  the Python object for reasons of efficiency). The lambda function can then
  either run an in-place constructor and return normally (in which case the
  instance is assumed to be correctly constructed) or fail by raising an
  exception.

- **Trampoline classes.** Trampolines, i.e., polymorphic class implementations
  that forward virtual function calls to Python, now require an extra
  `NB_TRAMPOLINE(parent, size)` declaration, where `parent` refers to the
  parent class and `size` is at least as big as the number of `NB_OVERRIDE_*()`
  calls. _nanobind_ caches information to enable efficient function dispatch,
  for which it must know the number of trampoline "slots". Example:

  ```cpp
  struct PyAnimal : Animal {
      NB_TRAMPOLINE(Animal, 1);

      std::string name() const override {
          NB_OVERRIDE(std::string, Animal, name);
      }
  };
  ```

  Trampoline declarations with an insufficient size may eventually trigger a
  Python `RuntimeError` exception with a descriptive label, e.g.
  `nanobind::detail::get_trampoline('PyAnimal::what()'): the trampoline ran out
  of slots (you will need to increase the value provided to the NB_TRAMPOLINE()
  macro)!`.

- **Type casters.** The API of custom type casters has changed _significantly_.
  In a nutshell, the following changes are needed:

  - `load()` was renamed to `from_python()`. The function now takes an extra
    `uint8_t flags` (instead `bool convert`, which is now represented by the
    flag `nanobind::detail::cast_flags::convert`). A `cleanup_list *` pointer
    keeps track of Python temporaries that are created by the conversion, and
    which need to be deallocated after a function call has taken place. `flags`
    and `cleanup` should be passed to any recursive usage of
    `type_caster::from_python()`.

  - `cast()` was renamed to `from_cpp()`. The function takes a return value
    policy (as before) and a `cleanup_list *` pointer.

  Both functions must be marked as `noexcept`. In contrast to _pybind11_,
  errors during type casting are only propagated using status codes. If a
  severe error condition arises that should be reported, use Python warning API
  calls for this, e.g. `PyErr_WarnFormat()`.

  Note that the cleanup list is only available when `from_python()` or
  `from_cpp()` are called as part of function dispatch, while usage by
  `nanobind::cast()` sets `cleanup` to `nullptr`. This case should be handled
  gracefully by refusing the conversion if the cleanup list is absolutely required.

  The [std::pair type
  caster](https://github.com/wjakob/nanobind/blob/master/include/nanobind/stl/pair.h)
  may be useful as a reference for these changes.

- The following types and functions were renamed:

  | _pybind11_           | _nanobind_      |
  | -------------------- | --------------- |
  | `error_already_set`  | `python_error`  |
  | `type::of<T>`        | `type<T>`       |
  | `type`               | `type_object`   |
  | `reinterpret_borrow` | `borrow`        |
  | `reinterpret_steal`  | `steal`         |
  | `custom_type_setup`  | `type_callback` |

- **New features.**

  - **Unified DLPack/Buffer protocol integration**: _nanobind_ can retrieve and
    return tensors using two standard protocols:
    [DLPack](https://github.com/dmlc/dlpack), and the the [buffer
    protocol](https://docs.python.org/3/c-api/buffer.html). This enables
    zero-copy data exchange of CPU and GPU tensors with array programming
    frameworks including [NumPy](https://numpy.org),
    [PyTorch](https://pytorch.org), [TensorFlow](https://www.tensorflow.org),
    [JAX](https://jax.readthedocs.io), etc.

    Details on using this feature can be found
    [here](https://github.com/wjakob/nanobind/blob/master/docs/tensor.md).


  - **Supplemental type data**: _nanobind_ can store supplemental data along
    with registered types. An example use of this fairly advanced feature are
    libraries that register large numbers of different types (e.g. flavors of
    tensors). A single generically implemented function can then query this
    supplemental information to handle each type slightly differently.

    ```cpp
    struct Supplement {
        ... // should be a POD (plain old data) type
    };

    // Register a new type Test, and reserve space for sizeof(Supplement)
    nb::class_<Test> cls(m, "Test", nb::supplement<Supplement>(), nb::is_final())

    /// Mutable reference to 'Supplement' portion in Python type object
    Supplement &supplement = nb::type_supplement<Supplement>(cls);
    ```

    The supplement is not propagated to subclasses created within Python.
    Such types should therefore be created with `nb::is_final()`.

  - **Low-level interface**: _nanobind_ exposes a low-level interface to
    provide fine-grained control over the sequence of steps that instantiates a
    Python object wrapping a C++ instance. Like the above point, this is useful
    when writing generic binding code that manipulates _nanobind_-based objects
    of various types.

    Details on using this feature can be found
    [here](https://github.com/wjakob/nanobind/blob/master/docs/lowlevel.md).

  - **Python type wrappers**: The `nb::handle_t<T>` type behaves just like the
    `nb::handle` class and wraps a `PyObject *` pointer. However, when binding
    a function that takes such an argument, _nanobind_ will only call the
    associated function overload when the underlying Python object wraps a C++
    instance of type `T`.

    Siimlarly, the `nb::type_object_t<T>` type behaves just like the
    `nb::type_object` class and wraps a `PyTypeObject *` pointer. However, when
    binding a function that takes such an argument, _nanobind_ will only call
    the associated function overload when the underlying Python type object is
    a subtype of the C++ type `T`.

  - **Raw docstrings**: In cases where absolute control over docstrings is
    required (for example, so that complex cases can be parsed by a tool like
    [Sphinx](https://www.sphinx-doc.org)), the ``nb::raw_doc`` attribute can be
    specified to functions. In this case, _nanobind_ will _skip_ generation of
    a combined docstring that enumerates overloads along with type information.

    Example:

    ```cpp
    m.def("identity", [](float arg) { return arg; });
    m.def("identity", [](int arg) { return arg; },
          nb::raw_doc(
              "identity(arg)\n"
              "An identity function for integers and floats\n"
              "\n"
              "Args:\n"
              "    arg (float | int): Input value\n"
              "\n"
              "Returns:\n"
              "    float | int: Result of the identity operation"));
    ```

    Writing detailed docstrings in this way is rather tedious. In practice,
    they would usually be extracted from C++ heades using a tool like
    [pybind11_mkdoc](https://github.com/pybind/pybind11_mkdoc).

## How to cite this project?

Please use the following BibTeX template to cite nanobind in scientific
discourse:

```bibtex
@misc{nanobind,
   author = {Wenzel Jakob},
   year = {2022},
   note = {https://github.com/wjakob/nanobind},
   title = {nanobind -- Seamless operability between C++17 and Python}
}
```
