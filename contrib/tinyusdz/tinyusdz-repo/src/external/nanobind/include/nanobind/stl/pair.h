#pragma once

#include <nanobind/nanobind.h>
#include <utility>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename T1, typename T2> struct type_caster<std::pair<T1, T2>> {
    using Value = std::pair<T1, T2>;

    // Sub type casters
    using Caster1 = make_caster<T1>;
    using Caster2 = make_caster<T2>;

    /**
     * \brief Target type for extracting the result of a `from_python` cast
     * using the C++ cast operator (see operator Value() below)
     *
     * The std::pair type caster is slightly restricted: to support pairs
     * containing references, the pair is constructed on the fly in the cast
     * operator. Other classes will typically specify the more general
     *
     * ```
     * template <typename T> using Cast = default_cast_t<T_>;
     * // or, even better:
     * template <typename T> using Cast = movable_cast_t<T_>;
     * ```
     *
     * which can also cast to pointers, or lvalue/rvalue references.
     */
    template <typename T> using Cast = Value;

    // Value name for docstring generation
    static constexpr auto Name =
        const_name("tuple[") + concat(Caster1::Name, Caster2::Name) + const_name("]");

    /// Python -> C++ caster, populates `caster1` and `caster2` upon success
    bool from_python(handle src, uint8_t flags,
                     cleanup_list *cleanup) noexcept {
        PyObject *temp; // always initialized by the following line
        PyObject **o = seq_get_with_size(src.ptr(), 2, &temp);

        bool success = o &&
                       caster1.from_python(o[0], flags, cleanup) &&
                       caster2.from_python(o[1], flags, cleanup);

        Py_XDECREF(temp);

        return success;
    }

    template <typename T>
    static handle from_cpp(T *value, rv_policy policy, cleanup_list *cleanup) {
        if (!value)
            return none().release();
        return from_cpp(*value, policy, cleanup);
    }

    template <typename T>
    static handle from_cpp(T &&value, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        object o1 = steal(
            Caster1::from_cpp(forward_like<T>(value.first), policy, cleanup));
        if (!o1.is_valid())
            return {};

        object o2 = steal(
            Caster2::from_cpp(forward_like<T>(value.second), policy, cleanup));
        if (!o2.is_valid())
            return {};

        PyObject *r = PyTuple_New(2);
        NB_TUPLE_SET_ITEM(r, 0, o1.release().ptr());
        NB_TUPLE_SET_ITEM(r, 1, o2.release().ptr());
        return r;
    }

    /// Return the constructed tuple by copying from the sub-casters
    explicit operator Value() & {
        return Value(caster1.operator cast_t<T1>(),
                     caster2.operator cast_t<T2>());
    }

    // Return the constructed type by moving from the sub-casters
    explicit operator Value() && {
        return Value(((Caster1 &&) caster1).operator cast_t<T1>(),
                     ((Caster2 &&) caster2).operator cast_t<T2>());
    }

    Caster1 caster1;
    Caster2 caster2;
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
