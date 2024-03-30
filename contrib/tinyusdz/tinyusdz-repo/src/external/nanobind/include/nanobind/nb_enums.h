/*
    nanobind/nb_enum.h: enumerations used in nanobind (just rv_policy atm.)

    Copyright (c) 2022 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)

// Approach used to cast a previously unknown C++ instance into a Python object
enum class rv_policy {
    /** This is the default return value policy, which falls back to the policy
        rv_policy::take_ownership when the return value is a pointer.
        Otherwise, it uses return_value::move or return_value::copy for rvalue
        and lvalue references, respectively. See below for a description of what
        all of these different policies do. */
    automatic = 0,

    /** As above, but use policy rv_policy::reference when the return
        value is a pointer. This is the default conversion policy for function
        arguments when calling Python functions manually from C++ code (i.e. via
        handle::operator()). You probably won't need to use this. */
    automatic_reference = 1,

    /** Reference an existing object (i.e. do not create a new copy) and take
        ownership. Python will call the destructor and delete operator when the
        object’s reference count reaches zero. Undefined behavior ensues when
        the C++ side does the same.. */
    take_ownership = 2,

    /** Create a new copy of the returned object, which will be owned by
        Python. This policy is comparably safe because the lifetimes of the two
        instances are decoupled. */
    copy = 3,

    /** Use std::move to move the return value contents into a new instance
        that will be owned by Python. This policy is comparably safe because the
        lifetimes of the two instances (move source and destination) are
        decoupled. */
    move = 4,

    /** Reference an existing object, but do not take ownership. The C++ side
        is responsible for managing the object’s lifetime and deallocating it
        when it is no longer used. Warning: undefined behavior will ensue when
        the C++ side deletes an object that is still referenced and used by
        Python. */
    reference = 5,

    /** This policy only applies to methods and properties. It references the
        object without taking ownership similar to the above
        rv_policy::reference policy. In contrast to that policy, the
        function or property’s implicit this argument (called the parent) is
        considered to be the the owner of the return value (the child).
        nanobind then couples the lifetime of the parent to the child via a
        reference relationship that ensures that the parent cannot be garbage
        collected while Python is still using the child. More advanced
        variations of this scheme are also possible using combinations of
        rv_policy::reference and the keep_alive call policy */
    reference_internal = 6,

    /** Conservative policy: only succeed if the object is already
       registered with nanobind. */
    none = 7

    /* Note to self: nb_func.h assumes that this value fits into 3 bits,
       hence no further policies can be added. */
};

NAMESPACE_END(NB_NAMESPACE)
