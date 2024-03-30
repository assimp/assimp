/*
    nanobind/nb_types.h: nb::dict/str/list/..: C++ wrappers for Python types

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)

/// Macro defining functions/constructors for nanobind::handle subclasses
#define NB_OBJECT(Type, Parent, Str, Check)                                    \
public:                                                                        \
    static constexpr auto Name = detail::const_name(Str);                      \
    NB_INLINE Type(handle h, detail::borrow_t)                                 \
        : Parent(h, detail::borrow_t{}) {}                                     \
    NB_INLINE Type(handle h, detail::steal_t)                                  \
        : Parent(h, detail::steal_t{}) {}                                      \
    NB_INLINE static bool check_(handle h) {                                   \
        return Check(h.ptr());                                                 \
    }

/// Like NB_OBJECT but allow null-initialization
#define NB_OBJECT_DEFAULT(Type, Parent, Str, Check)                            \
    NB_OBJECT(Type, Parent, Str, Check)                                        \
    NB_INLINE Type() : Parent() {}

/// Helper macro to create detail::api comparison functions
#define NB_API_COMP(name, op)                                                  \
    template <typename T> NB_INLINE bool name(const api<T> &o) const {         \
        return detail::obj_comp(derived().ptr(), o.derived().ptr(), op);       \
    }

/// Helper macro to create detail::api unary operators
#define NB_API_OP_1(name, op)                                                  \
    NB_INLINE auto name() const {                                              \
        return steal(detail::obj_op_1(derived().ptr(), op));                   \
    }

/// Helper macro to create detail::api binary operators
#define NB_API_OP_2(name, op)                                                  \
    template <typename T> NB_INLINE auto name(const api<T> &o) const {         \
        return steal(                                                          \
            detail::obj_op_2(derived().ptr(), o.derived().ptr(), op));         \
    }

#define NB_API_OP_2_I(name, op)                                                \
    template <typename T> NB_INLINE auto name(const api<T> &o) {               \
        return steal(                                                          \
            detail::obj_op_2(derived().ptr(), o.derived().ptr(), op));         \
    }


// A few forward declarations
class object;
class handle;
class iterator;

template <typename T = object> T borrow(handle h);
template <typename T = object> T steal(handle h);

NAMESPACE_BEGIN(detail)

template <typename T, typename SFINAE = int> struct type_caster;
template <typename T> using make_caster = type_caster<intrinsic_t<T>>;

template <typename Impl> class accessor;
struct str_attr; struct obj_attr;
struct str_item; struct obj_item; struct num_item;
struct num_item_list; struct num_item_tuple;
class args_proxy; class kwargs_proxy;
struct borrow_t { };
struct steal_t { };
class api_tag { };
class dict_iterator;
struct fast_iterator;

// Standard operations provided by every nanobind object
template <typename Derived> class api : public api_tag {
public:
    Derived &derived() { return static_cast<Derived &>(*this); }
    const Derived &derived() const { return static_cast<const Derived &>(*this); }

    NB_INLINE bool is(handle value) const;
    NB_INLINE bool is_none() const { return derived().ptr() == Py_None; }
    NB_INLINE bool is_type() const { return PyType_Check(derived().ptr()); }
    NB_INLINE bool is_valid() const { return derived().ptr() != nullptr; }
    NB_INLINE handle inc_ref() const & noexcept;
    NB_INLINE handle dec_ref() const & noexcept;
    iterator begin() const;
    iterator end() const;

    NB_INLINE handle type() const;
    NB_INLINE operator handle() const;

    accessor<obj_attr> attr(handle key) const;
    accessor<str_attr> attr(const char *key) const;

    accessor<obj_item> operator[](handle key) const;
    accessor<str_item> operator[](const char *key) const;
    template <typename T, enable_if_t<std::is_arithmetic_v<T>> = 1>
    accessor<num_item> operator[](T key) const;
    args_proxy operator*() const;

    template <rv_policy policy = rv_policy::automatic_reference,
              typename... Args>
    object operator()(Args &&...args) const;

    NB_API_COMP(equal,      Py_EQ)
    NB_API_COMP(not_equal,  Py_NE)
    NB_API_COMP(operator<,  Py_LT)
    NB_API_COMP(operator<=, Py_LE)
    NB_API_COMP(operator>,  Py_GT)
    NB_API_COMP(operator>=, Py_GE)
    NB_API_OP_1(operator-,  PyNumber_Negative)
    NB_API_OP_1(operator!,  PyNumber_Invert)
    NB_API_OP_2(operator+,  PyNumber_Add)
    NB_API_OP_2(operator-,  PyNumber_Subtract)
    NB_API_OP_2(operator*,  PyNumber_Multiply)
    NB_API_OP_2(operator/,  PyNumber_TrueDivide)
    NB_API_OP_2(operator|,  PyNumber_Or)
    NB_API_OP_2(operator&,  PyNumber_And)
    NB_API_OP_2(operator^,  PyNumber_Xor)
    NB_API_OP_2(operator<<, PyNumber_Lshift)
    NB_API_OP_2(operator>>, PyNumber_Rshift)
    NB_API_OP_2(floor_div,  PyNumber_FloorDivide)
    NB_API_OP_2_I(operator+=, PyNumber_InPlaceAdd)
    NB_API_OP_2_I(operator-=, PyNumber_InPlaceSubtract)
    NB_API_OP_2_I(operator*=, PyNumber_InPlaceMultiply)
    NB_API_OP_2_I(operator/=, PyNumber_InPlaceTrueDivide)
    NB_API_OP_2_I(operator|=, PyNumber_InPlaceOr)
    NB_API_OP_2_I(operator&=, PyNumber_InPlaceAnd)
    NB_API_OP_2_I(operator^=, PyNumber_InPlaceXor)
    NB_API_OP_2_I(operator<<=,PyNumber_InPlaceLshift)
    NB_API_OP_2_I(operator>>=,PyNumber_InPlaceRshift)
};

NAMESPACE_END(detail)

class handle : public detail::api<handle> {
    friend class python_error;
    friend struct detail::str_attr;
    friend struct detail::obj_attr;
    friend struct detail::str_item;
    friend struct detail::obj_item;
    friend struct detail::num_item;
public:
    static constexpr auto Name = detail::const_name("object");

    handle() = default;
    handle(const handle &) = default;
    handle(handle &&) noexcept = default;
    handle &operator=(const handle &) = default;
    handle &operator=(handle &&) noexcept = default;
    NB_INLINE handle(const PyObject *ptr) : m_ptr((PyObject *) ptr) { }
    NB_INLINE handle(const PyTypeObject *ptr) : m_ptr((PyObject *) ptr) { }

    const handle& inc_ref() const & noexcept { Py_XINCREF(m_ptr); return *this; }
    const handle& dec_ref() const & noexcept { Py_XDECREF(m_ptr); return *this; }

    NB_INLINE operator bool() const { return m_ptr != nullptr; }
    NB_INLINE PyObject *ptr() const { return m_ptr; }
    NB_INLINE static bool check_(handle) { return true; }

protected:
    PyObject *m_ptr = nullptr;
};

class object : public handle {
public:
    static constexpr auto Name = detail::const_name("object");

    object() = default;
    object(const object &o) : handle(o) { inc_ref(); }
    object(object &&o) noexcept : handle(o) { o.m_ptr = nullptr; }
    ~object() { dec_ref(); }
    object(handle h, detail::borrow_t) : handle(h) { inc_ref(); }
    object(handle h, detail::steal_t) : handle(h) { }

    handle release() {
      handle temp(m_ptr);
      m_ptr = nullptr;
      return temp;
    }

    void clear() {
        dec_ref();
        m_ptr = nullptr;
    }

    object& operator=(const object &o) {
        handle temp(m_ptr);
        o.inc_ref();
        m_ptr = o.m_ptr;
        temp.dec_ref();
        return *this;
    }

    object& operator=(object &&o) noexcept {
        handle temp(m_ptr);
        m_ptr = o.m_ptr;
        o.m_ptr = nullptr;
        temp.dec_ref();
        return *this;
    }
};

template <typename T> NB_INLINE T borrow(handle h) {
    return { h, detail::borrow_t() };
}

template <typename T> NB_INLINE T steal(handle h) {
    return { h, detail::steal_t() };
}

inline bool hasattr(handle obj, const char *key) noexcept {
    return PyObject_HasAttrString(obj.ptr(), key);
}

inline bool hasattr(handle obj, handle key) noexcept {
    return PyObject_HasAttr(obj.ptr(), key.ptr());
}

inline object getattr(handle obj, const char *key) {
    return steal(detail::getattr(obj.ptr(), key));
}

inline object getattr(handle obj, handle key) {
    return steal(detail::getattr(obj.ptr(), key.ptr()));
}

inline object getattr(handle obj, const char *key, handle def) noexcept {
    return steal(detail::getattr(obj.ptr(), key, def.ptr()));
}

inline object getattr(handle obj, handle key, handle value) noexcept {
    return steal(detail::getattr(obj.ptr(), key.ptr(), value.ptr()));
}

inline void setattr(handle obj, const char *key, handle value) {
    detail::setattr(obj.ptr(), key, value.ptr());
}

inline void setattr(handle obj, handle key, handle value) {
    detail::setattr(obj.ptr(), key.ptr(), value.ptr());
}

class module_ : public object {
public:
    NB_OBJECT(module_, object, "module", PyModule_CheckExact);

    template <typename Func, typename... Extra>
    module_ &def(const char *name_, Func &&f, const Extra &...extra);

    /// Import and return a module or throws `python_error`.
    static NB_INLINE module_ import_(const char *name) {
        return steal<module_>(detail::module_import(name));
    }

    /// Import and return a module or throws `python_error`.
    NB_INLINE module_ def_submodule(const char *name,
                                    const char *doc = nullptr) {
        return borrow<module_>(detail::module_new_submodule(m_ptr, name, doc));
    }
};

class capsule : public object {
    NB_OBJECT_DEFAULT(capsule, object, "capsule", PyCapsule_CheckExact)

    capsule(const void *ptr, void (*free)(void *) noexcept = nullptr) {
        m_ptr = detail::capsule_new(ptr, free);
    }

    void *data() const { return PyCapsule_GetPointer(m_ptr, nullptr); }
};

class str : public object {
    NB_OBJECT_DEFAULT(str, object, "str", PyUnicode_Check)

    explicit str(handle h)
        : object(detail::str_from_obj(h.ptr()), detail::steal_t{}) { }

    explicit str(const char *c)
        : object(detail::str_from_cstr(c), detail::steal_t{}) { }

    explicit str(const char *c, size_t n)
        : object(detail::str_from_cstr_and_size(c, n), detail::steal_t{}) { }

    const char *c_str() { return PyUnicode_AsUTF8AndSize(m_ptr, nullptr); }
};

class tuple : public object {
    NB_OBJECT_DEFAULT(tuple, object, "tuple", PyTuple_Check)
    size_t size() const { return NB_TUPLE_GET_SIZE(m_ptr); }
    template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>> = 1>
    detail::accessor<detail::num_item_tuple> operator[](T key) const;
#if !defined(Py_LIMITED_API)
    detail::fast_iterator begin() const;
    detail::fast_iterator end() const;
#endif
};

class type_object : public object {
    NB_OBJECT_DEFAULT(type_object, object, "type", PyType_Check)
};

class list : public object {
    NB_OBJECT(list, object, "list", PyList_Check)
    list() : object(PyList_New(0), detail::steal_t()) { }
    size_t size() const { return NB_LIST_GET_SIZE(m_ptr); }

    template <typename T> void append(T &&value);

    template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>> = 1>
    detail::accessor<detail::num_item_list> operator[](T key) const;
#if !defined(Py_LIMITED_API)
    detail::fast_iterator begin() const;
    detail::fast_iterator end() const;
#endif
};

class dict : public object {
    NB_OBJECT(dict, object, "dict", PyDict_Check)
    dict() : object(PyDict_New(), detail::steal_t()) { }
    size_t size() const { return NB_DICT_GET_SIZE(m_ptr); }
    detail::dict_iterator begin() const;
    detail::dict_iterator end() const;
    list keys() const { return steal<list>(detail::obj_op_1(m_ptr, PyDict_Keys)); }
    list values() const { return steal<list>(detail::obj_op_1(m_ptr, PyDict_Values)); }
    list items() const { return steal<list>(detail::obj_op_1(m_ptr, PyDict_Items)); }
};

class sequence : public object {
    NB_OBJECT_DEFAULT(sequence, object, "Sequence", PySequence_Check)
};

class mapping : public object {
    NB_OBJECT_DEFAULT(mapping, object, "Mapping", PyMapping_Check)
    list keys() const { return steal<list>(detail::obj_op_1(m_ptr, PyMapping_Keys)); }
    list values() const { return steal<list>(detail::obj_op_1(m_ptr, PyMapping_Values)); }
    list items() const { return steal<list>(detail::obj_op_1(m_ptr, PyMapping_Items)); }
};

class args : public tuple {
    NB_OBJECT_DEFAULT(args, tuple, "tuple", PyTuple_Check)
};

class kwargs : public dict {
    NB_OBJECT_DEFAULT(kwargs, dict, "dict", PyDict_Check)
};

class iterator : public object {
public:
    using difference_type = Py_ssize_t;
    using value_type = handle;
    using reference = const handle;
    using pointer = const handle *;

    NB_OBJECT_DEFAULT(iterator, object, "iterator", PyIter_Check)

    iterator& operator++() {
        m_value = steal(detail::obj_iter_next(m_ptr));
        return *this;
    }

    iterator operator++(int) {
        iterator rv = *this;
        m_value = steal(detail::obj_iter_next(m_ptr));
        return rv;
    }

    handle operator*() const {
        if (is_valid() && !m_value.is_valid())
            m_value = steal(detail::obj_iter_next(m_ptr));
        return m_value;
    }

    pointer operator->() const { operator*(); return &m_value; }

    static iterator sentinel() { return {}; }

    friend bool operator==(const iterator &a, const iterator &b) { return a->ptr() == b->ptr(); }
    friend bool operator!=(const iterator &a, const iterator &b) { return a->ptr() != b->ptr(); }

private:
    mutable object m_value;
};

template <typename T>
NB_INLINE bool isinstance(handle obj) noexcept {
    if constexpr (std::is_base_of_v<handle, T>)
        return T::check_(obj);
    else
        return detail::nb_type_isinstance(obj.ptr(), &typeid(detail::intrinsic_t<T>));
}

NB_INLINE str repr(handle h) { return steal<str>(detail::obj_repr(h.ptr())); }
NB_INLINE size_t len(handle h) { return detail::obj_len(h.ptr()); }
NB_INLINE size_t len(const tuple &t) { return NB_TUPLE_GET_SIZE(t.ptr()); }
NB_INLINE size_t len(const list &t) { return NB_LIST_GET_SIZE(t.ptr()); }
NB_INLINE size_t len(const dict &t) { return NB_DICT_GET_SIZE(t.ptr()); }

inline void print(handle value, handle end = handle(), handle file = handle()) {
    detail::print(value.ptr(), end.ptr(), file.ptr());
}

inline void print(const char *str, handle end = handle(), handle file = handle()) {
    print(nanobind::str(str), end, file);
}

/// Retrieve the Python type object associated with a C++ class
template <typename T> handle type() {
    return detail::nb_type_lookup(&typeid(detail::intrinsic_t<T>));
}

inline object none() { return borrow(Py_None); }
inline dict builtins() { return borrow<dict>(PyEval_GetBuiltins()); }

inline iterator iter(handle h) {
    return steal<iterator>(detail::obj_iter(h.ptr()));
}

template <typename T> class handle_t : public handle {
public:
    static constexpr auto Name = detail::make_caster<T>::Name;

    using handle::handle;
    using handle::operator=;
    handle_t(const handle &h) : handle(h) { }

    static bool check_(handle h) { return isinstance<T>(h); }
};

template <typename T> class type_object_t : public type_object {
public:
    static constexpr auto Name = detail::const_name("type[") +
                                 detail::make_caster<T>::Name +
                                 detail::const_name("]");

    using type_object::type_object;
    using type_object::operator=;

    static bool check_(handle h) {
        return PyType_Check(h.ptr()) &&
               PyType_IsSubtype((PyTypeObject *) h.ptr(),
                                (PyTypeObject *) nanobind::type<T>().ptr());
    }
};


NAMESPACE_BEGIN(detail)
template <typename Derived> NB_INLINE api<Derived>::operator handle() const {
    return derived().ptr();
}

template <typename Derived> NB_INLINE handle api<Derived>::type() const {
    return (PyObject *) Py_TYPE(derived().ptr());
}

template <typename Derived>
NB_INLINE handle api<Derived>::inc_ref() const &noexcept {
    return operator handle().inc_ref();
}

template <typename Derived>
NB_INLINE bool api<Derived>::is(handle value) const {
    return derived().ptr() == value.ptr();
}

template <typename Derived> iterator api<Derived>::begin() const {
    return iter(*this);
}

template <typename Derived> iterator api<Derived>::end() const {
    return iterator::sentinel();
}

struct fast_iterator {
    using value_type = handle;
    using reference = const value_type;

    fast_iterator(PyObject **value) : value(value) { }

    fast_iterator& operator++() { value++; return *this; }
    fast_iterator operator++(int) { fast_iterator rv = *this; value++; return rv; }
    friend bool operator==(const fast_iterator &a, const fast_iterator &b) { return a.value == b.value; }
    friend bool operator!=(const fast_iterator &a, const fast_iterator &b) { return a.value != b.value; }

    handle operator*() const { return *value; }

    PyObject **value;
};

class dict_iterator {
public:
    using value_type = std::pair<handle, handle>;
    using reference = const value_type;

    dict_iterator() : obj(handle()), pos(-1) { }

    dict_iterator(handle obj) : obj(obj), pos(0) {
        increment();
    }

    dict_iterator& operator++() {
        increment();
        return *this;
    }

    dict_iterator operator++(int) {
        dict_iterator rv = *this;
        increment();
        return rv;
    }

    void increment() {
        if (PyDict_Next(obj.ptr(), &pos, &key, &value) == 0)
            pos = -1;
    }

    value_type operator*() const { return { key, value }; }

    friend bool operator==(const dict_iterator &a, const dict_iterator &b) { return a.pos == b.pos; }
    friend bool operator!=(const dict_iterator &a, const dict_iterator &b) { return a.pos != b.pos; }

private:
    handle obj;
    Py_ssize_t pos;
    PyObject *key = nullptr, *value = nullptr;
};


NAMESPACE_END(detail)

inline detail::dict_iterator dict::begin() const { return { *this }; }
inline detail::dict_iterator dict::end() const { return { }; }

#if !defined(Py_LIMITED_API)
inline detail::fast_iterator tuple::begin() const {
    return ((PyTupleObject *) m_ptr)->ob_item;
}
inline detail::fast_iterator tuple::end() const {
    PyTupleObject *v = (PyTupleObject *) m_ptr;
    return v->ob_item + v->ob_base.ob_size;
}
inline detail::fast_iterator list::begin() const {
    return ((PyListObject *) m_ptr)->ob_item;
}
inline detail::fast_iterator list::end() const {
    PyListObject *v = (PyListObject *) m_ptr;
    return v->ob_item + v->ob_base.ob_size;
}
#endif

NAMESPACE_END(NB_NAMESPACE)

#undef NB_API_COMP
#undef NB_API_OP_1
#undef NB_API_OP_2
#undef NB_API_OP_2_I
