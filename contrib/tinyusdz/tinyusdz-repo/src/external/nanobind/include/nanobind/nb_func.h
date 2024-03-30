/*
    nanobind/nb_func.h: Functionality for binding C++ functions/methods

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <bool ReturnRef, bool CheckGuard, typename Func, typename Return,
          typename... Args, size_t... Is, typename... Extra>
NB_INLINE PyObject *func_create(Func &&func, Return (*)(Args...),
                                std::index_sequence<Is...> is,
                                const Extra &...extra) {
    using Guard = typename extract_guard<Extra...>::type;

    if constexpr (CheckGuard && !std::is_same_v<Guard, void>) {
        return func_create<ReturnRef, false>(
            [func = (forward_t<Func>) func](Args... args) NB_INLINE_LAMBDA {
                typename Guard::type g;
                (void) g;
                return func(args...);
            },
            (Return(*)(Args...)) nullptr, is, extra...);
    }

    (void) is;

    // Detect locations of nb::args / nb::kwargs (if exists)
    static constexpr size_t
        args_pos_1 = index_1_v<std::is_same_v<intrinsic_t<Args>, args>...>,
        args_pos_n = index_n_v<std::is_same_v<intrinsic_t<Args>, args>...>,
        kwargs_pos_1 = index_1_v<std::is_same_v<intrinsic_t<Args>, kwargs>...>,
        kwargs_pos_n = index_n_v<std::is_same_v<intrinsic_t<Args>, kwargs>...>,
        nargs = sizeof...(Args);

    // Determine the number of nb::arg/nb::arg_v annotations
    constexpr size_t nargs_provided =
        ((std::is_same_v<arg, Extra> + std::is_same_v<arg_v, Extra>) + ... + 0);
    constexpr bool is_method_det =
        (std::is_same_v<is_method, Extra> + ... + 0) != 0;

    /// A few compile-time consistency checks
    static_assert(args_pos_1 == args_pos_n && kwargs_pos_1 == kwargs_pos_n,
        "Repeated use of nb::kwargs or nb::args in the function signature!");
    static_assert(nargs_provided == 0 || nargs_provided + is_method_det == nargs,
        "The number of nb::arg annotations must match the argument count!");
    static_assert(kwargs_pos_1 == nargs || kwargs_pos_1 + 1 == nargs,
        "nb::kwargs must be the last element of the function signature!");
    static_assert(args_pos_1 == nargs || args_pos_1 + 1 == kwargs_pos_1,
        "nb::args must follow positional arguments and precede nb::kwargs!");

    // Collect function signature information for the docstring
    using cast_out = make_caster<
        std::conditional_t<std::is_void_v<Return>, void_type, Return>>;
    constexpr auto descr =
        const_name("(") + concat(type_descr(make_caster<Args>::Name)...) +
        const_name(") -> ") + cast_out::Name;
    const std::type_info* descr_types[descr.type_count() + 1];
    descr.put_types(descr_types);

    // Auxiliary data structure to capture the provided function/closure
    struct capture {
        std::remove_reference_t<Func> func;
    };

    // The following temporary record will describe the function in detail
    func_data_prelim<nargs_provided> f;
    f.flags = (args_pos_1   < nargs ? (uint32_t) func_flags::has_var_args   : 0) |
              (kwargs_pos_1 < nargs ? (uint32_t) func_flags::has_var_kwargs : 0) |
              (nargs_provided       ? (uint32_t) func_flags::has_args       : 0) |
              (ReturnRef            ? (uint32_t) func_flags::return_ref     : 0);

    /* Store captured function inside 'func_data_prelim' if there is space. Issues
       with aliasing are resolved via separate compilation of libnanobind. */
    if constexpr (sizeof(capture) <= sizeof(f.capture)) {
        capture *cap = (capture *) f.capture;
        new (cap) capture{ (forward_t<Func>) func };

        if constexpr (!std::is_trivially_destructible_v<capture>) {
            f.flags |= (uint32_t) func_flags::has_free;
            f.free = [](void *p) {
                ((capture *) p)->~capture();
            };
        }
    } else {
        void **cap = (void **) f.capture;
        cap[0] = new capture{ (forward_t<Func>) func };

        f.flags |= (uint32_t) func_flags::has_free;
        f.free = [](void *p) {
            delete (capture *) ((void **) p)[0];
        };
    }

    f.impl = [](void *p, PyObject **args, uint8_t *args_flags, rv_policy policy,
                cleanup_list *cleanup) -> PyObject * {
        (void)p; (void)args; (void)args_flags; (void)policy; (void)cleanup;

        const capture *cap;
        if constexpr (sizeof(capture) <= sizeof(f.capture))
            cap = (capture *) p;
        else
            cap = (capture *) ((void **) p)[0];

        tuple<make_caster<Args>...> in;
        (void) in;

        if ((!in.template get<Is>().from_python(args[Is], args_flags[Is],
                                                cleanup) || ...))
            return NB_NEXT_OVERLOAD;

        PyObject *result;
        if constexpr (std::is_void_v<Return>) {
            cap->func(((make_caster<Args>&&) in.template get<Is>()).operator cast_t<Args>()...);
            Py_INCREF(Py_None);
            result = Py_None;
        } else {
            result = cast_out::from_cpp(
                       cap->func(((make_caster<Args> &&) in.template get<Is>())
                                     .operator cast_t<Args>()...),
                       policy, cleanup).ptr();
        }

        (process_keep_alive(args, result, (Extra *) nullptr), ...);

        return result;
    };

    f.descr = descr.text;
    f.descr_types = descr_types;
    f.nargs = (uint16_t) nargs;

    // Fill remaining fields of 'f'
    size_t arg_index = 0;
    (void) arg_index;
    (func_extra_apply(f, extra, arg_index), ...);

    return nb_func_new((const void *) &f);
}

NAMESPACE_END(detail)

template <typename Return, typename... Args, typename... Extra>
NB_INLINE object cpp_function(Return (*f)(Args...), const Extra&... extra) {
    return steal(detail::func_create<true, true>(
        f, f, std::make_index_sequence<sizeof...(Args)>(), extra...));
}

template <typename Return, typename... Args, typename... Extra>
NB_INLINE void cpp_function_def(Return (*f)(Args...), const Extra&... extra) {
    detail::func_create<false, true>(
        f, f, std::make_index_sequence<sizeof...(Args)>(), extra...);
}

/// Construct a cpp_function from a lambda function (pot. with internal state)
template <
    typename Func, typename... Extra,
    detail::enable_if_t<detail::is_lambda_v<std::remove_reference_t<Func>>> = 0>
NB_INLINE object cpp_function(Func &&f, const Extra &...extra) {
    using am = detail::analyze_method<decltype(&std::remove_reference_t<Func>::operator())>;
    return steal(detail::func_create<true, true>(
        (detail::forward_t<Func>) f, (typename am::func *) nullptr,
        std::make_index_sequence<am::argc>(), extra...));
}

template <
    typename Func, typename... Extra,
    detail::enable_if_t<detail::is_lambda_v<std::remove_reference_t<Func>>> = 0>
NB_INLINE void cpp_function_def(Func &&f, const Extra &...extra) {
    using am = detail::analyze_method<decltype(&std::remove_reference_t<Func>::operator())>;
    detail::func_create<false, true>(
        (detail::forward_t<Func>) f, (typename am::func *) nullptr,
        std::make_index_sequence<am::argc>(), extra...);
}

/// Construct a cpp_function from a class method (non-const)
template <typename Return, typename Class, typename... Args, typename... Extra>
NB_INLINE object cpp_function(Return (Class::*f)(Args...), const Extra &...extra) {
    return steal(detail::func_create<true, true>(
        [f](Class *c, Args... args) NB_INLINE_LAMBDA -> Return {
            return (c->*f)((detail::forward_t<Args>) args...);
        },
        (Return(*)(Class *, Args...)) nullptr,
        std::make_index_sequence<sizeof...(Args) + 1>(), extra...));
}

template <typename Return, typename Class, typename... Args, typename... Extra>
NB_INLINE void cpp_function_def(Return (Class::*f)(Args...), const Extra &...extra) {
    detail::func_create<false, true>(
        [f](Class *c, Args... args) NB_INLINE_LAMBDA -> Return {
            return (c->*f)((detail::forward_t<Args>) args...);
        },
        (Return(*)(Class *, Args...)) nullptr,
        std::make_index_sequence<sizeof...(Args) + 1>(), extra...);
}

/// Construct a cpp_function from a class method (const)
template <typename Return, typename Class, typename... Args, typename... Extra>
NB_INLINE object cpp_function(Return (Class::*f)(Args...) const, const Extra &...extra) {
    return steal(detail::func_create<true, true>(
        [f](const Class *c, Args... args) NB_INLINE_LAMBDA -> Return {
            return (c->*f)((detail::forward_t<Args>) args...);
        },
        (Return(*)(const Class *, Args...)) nullptr,
        std::make_index_sequence<sizeof...(Args) + 1>(), extra...));
}

template <typename Return, typename Class, typename... Args, typename... Extra>
NB_INLINE void cpp_function_def(Return (Class::*f)(Args...) const, const Extra &...extra) {
    detail::func_create<false, true>(
        [f](const Class *c, Args... args) NB_INLINE_LAMBDA -> Return {
            return (c->*f)((detail::forward_t<Args>) args...);
        },
        (Return(*)(const Class *, Args...)) nullptr,
        std::make_index_sequence<sizeof...(Args) + 1>(), extra...);
}

template <typename Func, typename... Extra>
module_ &module_::def(const char *name_, Func &&f, const Extra &...extra) {
    cpp_function_def((detail::forward_t<Func>) f, scope(*this),
                     name(name_), extra...);
    return *this;
}

NAMESPACE_END(NB_NAMESPACE)
