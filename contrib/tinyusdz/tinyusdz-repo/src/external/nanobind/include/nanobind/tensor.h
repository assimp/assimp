/*
    nanobind/tensor.h: functionality to input/output tensors via DLPack

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.

    The API below is based on the DLPack project
    (https://github.com/dmlc/dlpack/blob/main/include/dlpack/dlpack.h)
*/
#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)

NAMESPACE_BEGIN(device)
#define NB_DEVICE(enum_name, enum_value)                              \
    struct enum_name {                                                \
        static constexpr auto name = detail::const_name(#enum_name);  \
        static constexpr int32_t value = enum_value;                  \
        static constexpr bool is_device = true;                       \
    }
NB_DEVICE(none, 0); NB_DEVICE(cpu, 1); NB_DEVICE(cuda, 2);
NB_DEVICE(cuda_host, 3); NB_DEVICE(opencl, 4); NB_DEVICE(vulkan, 7);
NB_DEVICE(metal, 8); NB_DEVICE(rocm, 10); NB_DEVICE(rocm_host, 11);
NB_DEVICE(cuda_managed, 13); NB_DEVICE(oneapi, 14);
#undef NB_DEVICE
NAMESPACE_END(device)

NAMESPACE_BEGIN(dlpack)

enum class dtype_code : uint8_t {
    Int = 0, UInt = 1, Float = 2, Bfloat = 4, Complex = 5
};

struct device {
    int32_t device_type = 0;
    int32_t device_id = 0;
};

struct dtype {
    uint8_t code = 0;
    uint8_t bits = 0;
    uint16_t lanes = 0;

    bool operator==(const dtype &o) const {
        return code == o.code && bits == o.bits && lanes == o.lanes;
    }
    bool operator!=(const dtype &o) const { return !operator==(o); }
};

struct tensor {
    void *data = nullptr;
    nanobind::dlpack::device device;
    int32_t ndim = 0;
    nanobind::dlpack::dtype dtype;
    int64_t *shape = nullptr;
    int64_t *strides = nullptr;
    uint64_t byte_offset = 0;
};

NAMESPACE_END(dlpack)

constexpr size_t any = (size_t) -1;

template <size_t... Is> struct shape {
    static constexpr size_t size = sizeof...(Is);
};

struct c_contig { };
struct f_contig { };
struct numpy { };
struct tensorflow { };
struct pytorch { };
struct jax { };

template <typename T> constexpr dlpack::dtype dtype() {
    static_assert(
        std::is_floating_point_v<T> || std::is_integral_v<T>,
        "nanobind::dtype<T>: T must be a floating point or integer variable!"
    );

    dlpack::dtype result;

    if constexpr (std::is_floating_point_v<T>)
        result.code = (uint8_t) dlpack::dtype_code::Float;
    else if constexpr (std::is_signed_v<T>)
        result.code = (uint8_t) dlpack::dtype_code::Int;
    else
        result.code = (uint8_t) dlpack::dtype_code::UInt;

    result.bits = sizeof(T) * 8;
    result.lanes = 1;

    return result;
}


NAMESPACE_BEGIN(detail)

enum class tensor_framework : int { none, numpy, tensorflow, pytorch, jax };

struct tensor_req {
    dlpack::dtype dtype;
    uint32_t ndim = 0;
    size_t *shape = nullptr;
    bool req_shape = false;
    bool req_dtype = false;
    char req_order = '\0';
    uint8_t req_device = 0;
};

template <typename T, typename = int> struct tensor_arg {
    static constexpr size_t size = 0;
    static constexpr auto name = descr<0>{ };
    static void apply(tensor_req &) { }
};

template <typename T> struct tensor_arg<T, enable_if_t<std::is_floating_point_v<T>>> {
    static constexpr size_t size = 0;

    static constexpr auto name =
        const_name("dtype=float") + const_name<sizeof(T) * 8>();

    static void apply(tensor_req &tr) {
        tr.dtype = dtype<T>();
        tr.req_dtype = true;
    }
};

template <typename T> struct tensor_arg<T, enable_if_t<std::is_integral_v<T>>> {
    static constexpr size_t size = 0;

    static constexpr auto name =
        const_name("dtype=") + const_name<std::is_unsigned_v<T>>("u", "") +
        const_name("int") + const_name<sizeof(T) * 8>();

    static void apply(tensor_req &tr) {
        tr.dtype = dtype<T>();
        tr.req_dtype = true;
    }
};

template <size_t... Is> struct tensor_arg<shape<Is...>> {
    static constexpr size_t size = sizeof...(Is);
    static constexpr auto name =
        const_name("shape=(") +
        concat(const_name<Is == any>(const_name("*"), const_name<Is>())...) +
        const_name(")");

    static void apply(tensor_req &tr) {
        size_t i = 0;
        ((tr.shape[i++] = Is), ...);
        tr.ndim = (uint32_t) sizeof...(Is);
        tr.req_shape = true;
    }
};

template <> struct tensor_arg<c_contig> {
    static constexpr size_t size = 0;
    static constexpr auto name = const_name("order='C'");
    static void apply(tensor_req &tr) { tr.req_order = 'C'; }
};

template <> struct tensor_arg<f_contig> {
    static constexpr size_t size = 0;
    static constexpr auto name = const_name("order='F'");
    static void apply(tensor_req &tr) { tr.req_order = 'F'; }
};

template <typename T> struct tensor_arg<T, enable_if_t<T::is_device>> {
    static constexpr size_t size = 0;
    static constexpr auto name = const_name("device='") + T::name + const_name('\'');
    static void apply(tensor_req &tr) { tr.req_device = (uint8_t) T::value; }
};

template <typename... Ts> struct tensor_info {
    using scalar_type = void;
    using shape_type = void;
    constexpr static auto name = const_name("tensor");
    constexpr static tensor_framework framework = tensor_framework::none;
};

template <typename T, typename... Ts> struct tensor_info<T, Ts...>  : tensor_info<Ts...> {
    using scalar_type =
        std::conditional_t<std::is_scalar_v<T>, T,
                           typename tensor_info<Ts...>::scalar_type>;
};

template <size_t... Is, typename... Ts> struct tensor_info<shape<Is...>, Ts...> : tensor_info<Ts...> {
    using shape_type = shape<Is...>;
};

template <typename... Ts> struct tensor_info<numpy, Ts...> : tensor_info<Ts...> {
    constexpr static auto name = const_name("numpy.ndarray");
    constexpr static tensor_framework framework = tensor_framework::numpy;
};

template <typename... Ts> struct tensor_info<pytorch, Ts...> : tensor_info<Ts...> {
    constexpr static auto name = const_name("torch.Tensor");
    constexpr static tensor_framework framework = tensor_framework::pytorch;
};

template <typename... Ts> struct tensor_info<tensorflow, Ts...> : tensor_info<Ts...> {
    constexpr static auto name = const_name("tensorflow.python.framework.ops.EagerTensor");
    constexpr static tensor_framework framework = tensor_framework::tensorflow;
};

template <typename... Ts> struct tensor_info<jax, Ts...> : tensor_info<Ts...> {
    constexpr static auto name = const_name("jaxlib.xla_extension.DeviceArray");
    constexpr static tensor_framework framework = tensor_framework::jax;
};

NAMESPACE_END(detail)

template <typename... Args> class tensor {
public:
    using Info = detail::tensor_info<Args...>;
    using Scalar = typename Info::scalar_type;

    tensor() = default;

    explicit tensor(detail::tensor_handle *handle) : m_handle(handle) {
        if (handle)
            m_tensor = *detail::tensor_inc_ref(handle);
    }

    tensor(void *value,
           size_t ndim,
           const size_t *shape,
           handle owner = nanobind::handle(),
           const int64_t *strides = nullptr,
           dlpack::dtype dtype = nanobind::dtype<Scalar>(),
           int32_t device_type = device::cpu::value,
           int32_t device_id = 0) {
        m_handle =
            detail::tensor_create(value, ndim, shape, owner.ptr(), strides,
                                  &dtype, device_type, device_id);
        m_tensor = *detail::tensor_inc_ref(m_handle);
    }

    ~tensor() {
        detail::tensor_dec_ref(m_handle);
    }

    tensor(const tensor &t) : m_handle(t.m_handle), m_tensor(t.m_tensor) {
        detail::tensor_inc_ref(m_handle);
    }

    tensor(tensor &&t) noexcept : m_handle(t.m_handle), m_tensor(t.m_tensor) {
        t.m_handle = nullptr;
        t.m_tensor = dlpack::tensor();
    }

    tensor &operator=(tensor &&t) noexcept {
        detail::tensor_dec_ref(m_handle);
        m_handle = t.m_handle;
        m_tensor = t.m_tensor;
        t.m_handle = nullptr;
        t.m_tensor = dlpack::tensor();
        return *this;
    }

    tensor &operator=(const tensor &t) {
        detail::tensor_inc_ref(t.m_handle);
        detail::tensor_dec_ref(m_handle);
        m_handle = t.m_handle;
        m_tensor = t.m_tensor;
        return *this;
    }

    dlpack::dtype dtype() const { return m_tensor.dtype; }
    size_t ndim() const { return m_tensor.ndim; }
    size_t shape(size_t i) const { return m_tensor.shape[i]; }
    int64_t stride(size_t i) const { return m_tensor.strides[i]; }
    bool is_valid() const { return m_handle != nullptr; }
    int32_t device_type() const { return m_tensor.device.device_type; }
    int32_t device_id() const { return m_tensor.device.device_id; }
    detail::tensor_handle *handle() const { return m_handle; }

    const void *data() const {
        return (const uint8_t *) m_tensor.data + m_tensor.byte_offset;
    }
    void *data() { return (uint8_t *) m_tensor.data + m_tensor.byte_offset; }

    template <typename... Ts>
    NB_INLINE auto& operator()(Ts... indices) {
        static_assert(
            !std::is_same_v<typename Info::scalar_type, void>,
            "To use nb::tensor::operator(), you must add a scalar type "
            "annotation (e.g. 'float') to the tensor template parameters.");
        static_assert(
            !std::is_same_v<typename Info::shape_type, void>,
            "To use nb::tensor::operator(), you must add a nb::shape<> "
            "annotation to the tensor template parameters.");
        static_assert(sizeof...(Ts) == Info::shape_type::size,
                      "nb::tensor::operator(): invalid number of arguments");

        int64_t counter = 0, index = 0;
        ((index += int64_t(indices) * m_tensor.strides[counter++]), ...);
        return (typename Info::scalar_type &) *(
            (uint8_t *) m_tensor.data + m_tensor.byte_offset +
            index * sizeof(typename Info::scalar_type));
    }

private:
    detail::tensor_handle *m_handle = nullptr;
    dlpack::tensor m_tensor;
};

NAMESPACE_BEGIN(detail)

template <typename... Args> struct type_caster<tensor<Args...>> {
    NB_TYPE_CASTER(tensor<Args...>, Value::Info::name + const_name("[") +
                                        concat_maybe(detail::tensor_arg<Args>::name...) +
                                        const_name("]"));

    bool from_python(handle src, uint8_t flags, cleanup_list *) noexcept {
        constexpr size_t size = (0 + ... + detail::tensor_arg<Args>::size);
        size_t shape[size + 1];
        detail::tensor_req req;
        req.shape = shape;
        (detail::tensor_arg<Args>::apply(req), ...);
        value = tensor<Args...>(tensor_import(
            src.ptr(), &req, flags & (uint8_t) cast_flags::convert));
        return value.is_valid();
    }

    static handle from_cpp(const tensor<Args...> &tensor, rv_policy,
                           cleanup_list *) noexcept {
        return tensor_wrap(tensor.handle(), int(Value::Info::framework));
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
