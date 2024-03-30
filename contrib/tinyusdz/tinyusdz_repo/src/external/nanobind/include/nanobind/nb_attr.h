/*
    nanobind/nb_attr.h: Annotations for function and class declarations

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)

struct scope {
    PyObject *value;
    NB_INLINE scope(handle value) : value(value.ptr()) {}
};

struct name {
    const char *value;
    NB_INLINE name(const char *value) : value(value) {}
};

struct arg_v;
struct arg {
    NB_INLINE constexpr explicit arg(const char *name = nullptr) : name(name) {}
    template <typename T> NB_INLINE arg_v operator=(T &&value) const;
    NB_INLINE arg &noconvert(bool value = true) {
        convert_ = !value;
        return *this;
    }
    NB_INLINE arg &none(bool value = true) {
        none_ = value;
        return *this;
    }

    const char *name;
    uint8_t convert_{ true };
    bool none_{ false };
};

struct arg_v : arg {
    object value;
    NB_INLINE arg_v(const arg &base, object &&value)
        : arg(base), value(std::move(value)) {}
};

template <typename... Ts> struct call_guard {
    using type = detail::tuple<Ts...>;
};

struct dynamic_attr {};
struct is_method {};
struct is_implicit {};
struct is_operator {};
struct is_arithmetic {};
struct is_final {};
struct is_enum { bool is_signed; };

template <size_t /* Nurse */, size_t /* Patient */> struct keep_alive {};
template <typename T> struct supplement {};
template <typename T> struct intrusive_ptr {
    intrusive_ptr(void (*set_self_py)(T *, PyObject *))
        : set_self_py(set_self_py) {}
    void (*set_self_py)(T *, PyObject *);
};

struct type_callback {
    type_callback(void (*value)(PyType_Slot **) noexcept) : value(value) {}
    void (*value)(PyType_Slot **) noexcept;
};
struct raw_doc {
    const char *value;
    raw_doc(const char *doc) : value(doc) {}
};

NAMESPACE_BEGIN(literals)
constexpr arg operator"" _a(const char *name, size_t) { return arg(name); }
NAMESPACE_END(literals)

NAMESPACE_BEGIN(detail)

enum class func_flags : uint32_t {
    /* Low 3 bits reserved for return value policy */

    /// Did the user specify a name for this function, or is it anonymous?
    has_name = (1 << 4),
    /// Did the user specify a scope where this function should be installed?
    has_scope = (1 << 5),
    /// Did the user specify a docstring?
    has_doc = (1 << 6),
    /// Did the user specify nb::arg/arg_v annotations for all arguments?
    has_args = (1 << 7),
    /// Does the function signature contain an *args-style argument?
    has_var_args = (1 << 8),
    /// Does the function signature contain an *kwargs-style argument?
    has_var_kwargs = (1 << 9),
    /// Is this function a class method?
    is_method = (1 << 10),
    /// Is this function a method called __init__? (automatically generated)
    is_constructor = (1 << 11),
    /// Can this constructor be used to perform an implicit conversion?
    is_implicit = (1 << 12),
    /// Is this function an arithmetic operator?
    is_operator = (1 << 13),
    /// When the function is GCed, do we need to call func_data_prelim::free?
    has_free = (1 << 14),
    /// Should the func_new() call return a new reference?
    return_ref = (1 << 15),
    /// Does this overload specify a raw docstring that should take precedence?
    raw_doc = (1 << 16)
};

struct arg_data {
    const char *name;
    PyObject *name_py;
    PyObject *value;
    bool convert;
    bool none;
};

template <size_t Size> struct func_data_prelim {
    // A small amount of space to capture data used by the function/closure
    void *capture[3];

    // Callback to clean up the 'capture' field
    void (*free)(void *);

    /// Implementation of the function call
    PyObject *(*impl)(void *, PyObject **, uint8_t *, rv_policy,
                      cleanup_list *);

    /// Function signature description
    const char *descr;

    /// C++ types referenced by 'descr'
    const std::type_info **descr_types;

    /// Supplementary flags
    uint32_t flags;

    /// Total number of function call arguments
    uint32_t nargs;

    // ------- Extra fields -------

    const char *name;
    const char *doc;
    PyObject *scope;

#if defined(_MSC_VER)
    arg_data args[Size == 0 ? 1 : Size];
#else
    arg_data args[Size];
#endif
};

template <typename F>
NB_INLINE void func_extra_apply(F &f, const name &name, size_t &) {
    f.name = name.value;
    f.flags |= (uint32_t) func_flags::has_name;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const scope &scope, size_t &) {
    f.scope = scope.value;
    f.flags |= (uint32_t) func_flags::has_scope;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const raw_doc &d, size_t &) {
    f.flags |= (uint32_t) func_flags::has_doc | (uint32_t) func_flags::raw_doc;
    f.doc = d.value;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const char *doc, size_t &) {
    f.doc = doc;
    f.flags |= (uint32_t) func_flags::has_doc;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, is_method, size_t &) {
    f.flags |= (uint32_t) func_flags::is_method;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, is_implicit, size_t &) {
    f.flags |= (uint32_t) func_flags::is_implicit;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, is_operator, size_t &) {
    f.flags |= (uint32_t) func_flags::is_operator;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, rv_policy pol, size_t &) {
    f.flags = (f.flags & ~0b11) | (uint16_t) pol;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const arg &a, size_t &index) {
    arg_data &arg = f.args[index++];
    arg.name = a.name;
    arg.value = nullptr;
    arg.convert = a.convert_;
    arg.none = a.none_;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const arg_v &a, size_t &index) {
    arg_data &arg = f.args[index++];
    arg.name = a.name;
    arg.value = a.value.ptr();
    arg.convert = a.convert_;
    arg.none = a.none_;
}

template <typename F, typename... Ts>
NB_INLINE void func_extra_apply(F &, call_guard<Ts...>, size_t &) {}

template <typename F, size_t Nurse, size_t Patient>
NB_INLINE void func_extra_apply(F &, nanobind::keep_alive<Nurse, Patient>,
                                size_t &) {}

template <typename... Ts> struct extract_guard { using type = void; };

template <typename T, typename... Ts> struct extract_guard<T, Ts...> {
    using type = typename extract_guard<Ts...>::type;
};

template <typename... Cs, typename... Ts>
struct extract_guard<call_guard<Cs...>, Ts...> {
    static_assert(std::is_same_v<typename extract_guard<Ts...>::type, void>,
                  "call_guard<> can only be specified once!");
    using type = call_guard<Cs...>;
};

template <typename T>
NB_INLINE void process_keep_alive(PyObject **, PyObject *, T *) {}

template <size_t Nurse, size_t Patient>
NB_INLINE void
process_keep_alive(PyObject **args, PyObject *result,
                   nanobind::keep_alive<Nurse, Patient> *) noexcept {
    keep_alive(Nurse == 0 ? result : args[Nurse - 1],
               Patient == 0 ? result : args[Patient - 1]);
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
