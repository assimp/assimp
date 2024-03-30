#include "Reflect.h"

namespace reflect {

//--------------------------------------------------------
// A type descriptor for int
//--------------------------------------------------------

struct TypeDescriptor_Int : TypeDescriptor {
    TypeDescriptor_Int() : TypeDescriptor{"int", sizeof(int)} {
    }
    virtual void dump(const void* obj, int /* unused */) const override {
        std::cout << "int{" << *(const int*) obj << "}";
    }
};

template <>
TypeDescriptor* getPrimitiveDescriptor<int>() {
    static TypeDescriptor_Int typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// A type descriptor for float
//--------------------------------------------------------

struct TypeDescriptor_Float : TypeDescriptor {
    TypeDescriptor_Float() : TypeDescriptor{"float", sizeof(float)} {
    }
    virtual void dump(const void* obj, int /* unused */) const override {
        std::cout << "float{" << *(const float*) obj << "}";
    }
};

template <>
TypeDescriptor* getPrimitiveDescriptor<float>() {
    static TypeDescriptor_Float typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// A type descriptor for std::string
//--------------------------------------------------------

struct TypeDescriptor_StdString : TypeDescriptor {
    TypeDescriptor_StdString() : TypeDescriptor{"std::string", sizeof(std::string)} {
    }
    virtual void dump(const void* obj, int /* unused */) const override {
        std::cout << "std::string{\"" << *(const std::string*) obj << "\"}";
    }
};

template <>
TypeDescriptor* getPrimitiveDescriptor<std::string>() {
    static TypeDescriptor_StdString typeDesc;
    return &typeDesc;
}

#define REGISTER_TYPE_DESCRIPTOR(suffix, type) \
struct TypeDescriptor_##suffix : TypeDescriptor { \
    TypeDescriptor_##suffix() : TypeDescriptor{#type, sizeof(type)} { \
    } \
    virtual void dump(const void* obj, int /* unused */) const override {\
        std::cout << #type "{\"" << *(const type*) obj << "\"}"; \
    } \
}; \
template <> \
TypeDescriptor* getPrimitiveDescriptor<type>() { \
    static TypeDescriptor_##suffix typeDesc; \
    return &typeDesc; \
}

REGISTER_TYPE_DESCRIPTOR(Double, double);

} // namespace reflect
