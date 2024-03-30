#pragma once

#include <nanobind/nanobind.h>
#include <functional>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

struct function_handle {
    object f;
    explicit function_handle(handle h): f(borrow(h)) { }
    function_handle(function_handle &&h) noexcept : f(std::move(h.f)) { }
    function_handle(const function_handle &h) {
        gil_scoped_acquire acq;
        f = h.f;
    }

    ~function_handle() {
        if (f.is_valid()) {
            gil_scoped_acquire acq;
            f.release().dec_ref();
        }
    }
};

template <typename Return, typename... Args>
struct type_caster<std::function<Return(Args...)>> {
    NB_TYPE_CASTER(std::function <Return(Args...)>,
                   const_name("Callable[[") +
                       concat(make_caster<Args>::Name...) + const_name("], ") +
                       make_caster<Return>::Name + const_name("]"));

    bool from_python(handle src, uint8_t flags, cleanup_list *) noexcept {
        if (src.is_none())
            return flags & cast_flags::convert;

        if (!PyCallable_Check(src.ptr()))
            return false;

        value = [f = function_handle(src)](Args... args) -> Return {
            gil_scoped_acquire acq;
            return cast<Return>(f.f((forward_t<Args>) args...));
        };

        return true;
    }

    static handle from_cpp(const Value &value, rv_policy,
                           cleanup_list *) noexcept {
        if (!value)
            return none().inc_ref();
        return cpp_function(value).release();
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
