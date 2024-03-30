import copy
import ctypes
import ctinyusd
import sys

from typing import Optional, Union, List, Any

try:
    from typeguard import typechecked
    is_typegurad_available = True
except:
    is_typegurad_available = False

    # no-op 
    def typechecked(cls):
        return cls

try:
    import numpy as np
except:
    raise ModuleNotFoundError("Need numpy to test code.")

@typechecked
class Token(object):
    """Python wrapper for `token` type
    """

    def __init__(self, tok: str):
        #print("__init__")
        self._handle = ctinyusd.c_tinyusd_token_new(tok)
        #print("init", self._handle)

    def __copy__(self):
        # Duplicate instance
        return Token(str(self))

    def __deepcopy__(self, memo):
        return self.__copy__()

    def __del__(self):
        ret = ctinyusd.c_tinyusd_token_free(self._handle)
        assert ret == 1

    def __str__(self):
        btok = ctinyusd.c_tinyusd_token_str(self._handle)
        return btok.decode()


@typechecked
class String(object):
    """Python wrapper for `string` type
    """

    def __init__(self, s: str):
        
        self._handle = ctinyusd.c_tinyusd_string_new(s)

    def __copy__(self):
        # Duplicate instance
        return String(str(self))

    def __deepcopy__(self, memo):
        return self.__copy__()

    def __del__(self):
        ret = ctinyusd.c_tinyusd_token_free(self._handle)
        assert ret == 1

    def __str__(self):
        btok = ctinyusd.c_tinyusd_token_str(self._handle)
        return btok.decode()


@typechecked
class Attribute(object):
    def __init__(self):
        pass


@typechecked
class Relationship(object):
    def __init__(self, target: Union[str, List[str], None]):
        self.target = target
        print("Relationship", self.target)

    def __str__(self):
        if isinstance(self.target, List):
            raise
        else:
            return str(self.target)
        


@typechecked
class Property(object):
    def __init__(self, prop: Union[Attribute, Relationship]):
        pass


@typechecked
class Property(object):
    def __init__(self, name: str, value: Any = None):
        self.name = name
        self.value = value

@typechecked
class Attribute(Property):
    def __init__(self, name: str, value: Any = None):
        super().__init__(self, name, value)


@typechecked
class Relationship(Property):
    def __init__(self, name: str, target: Union[str, List[str], None] = None):
        super().__init__(self, name, value)



@typechecked
class Value(object):
    _np_conv_table = {
        "float32": [ctinyusd.c_tinyusd_value_new_float, ctinyusd.c_tinyusd_value_new_array_float]
    }

    def __init__(self, value):

        # CTinyUSDValue*
        self._handle = None

        if isinstance(value, np.ndarray):
            if str(value.dtype) in self._np_conv_table:
                fun_table = self._np_conv_table[str(value.dtype)]
                print(fun_table)

                if value.ndim == 0:
                    # scalar
                    self._handle = ctinyusd.c_tinyusd_value_new_float(value.ctypes.data_as(ctypes.c_float))
                elif value.ndim == 1:
                    # 1D array

                    c_type = ctypes.POINTER(ctypes.c_float)

                    #self._handle = ctinyusd.c_tinyusd_value_new_array_float(value.size, value.ctypes.data_as(ctypes.POINTER(ctypes.c_float)))
                    self._handle = ctinyusd.c_tinyusd_value_new_array_float(value.size, value.ctypes.data_as(c_type))
                else:
                    raise RuntimeError("2D or multi dim array is not supported: ndim = {}".format(value.ndim)) 
                    # [0] = scalr, [1] = array
            else:
                raise RuntimeError("Unsupported dtype {}".format(value.dtype))
                
        elif isinstance(value, np.generic):
            if str(value.dtype) in self._np_conv_table:
                fun_table = self._np_conv_table[str(value.dtype)]
                print(fun_table)

                self._handle = ctinyusd.c_tinyusd_value_new_float(np.ctypeslib.as_ctypes(value))
            else:
                raise RuntimeError("Unsupported dtype {}".format(value.dtype))
            

        else:
            raise "TODO"


    def __del__(self):
        if self._handle is not None:
            ret = ctinyusd.c_tinyusd_value_free(self._handle)
            assert ret == 1

    def __str__(self):
        if self._handle is not None:
            cstrbuf = ctinyusd.c_tinyusd_string_new_empty() 
            ret = ctinyusd.c_tinyusd_value_to_string(self._handle, cstrbuf)
            assert ret == 1

            cstr = ctinyusd.c_tinyusd_string_str(cstrbuf)

            s = cstr.decode() # str
            
            ctinyusd.c_tinyusd_string_free(cstrbuf)

            return s
        

    def __repr__(self):
        return self.__str__()

        
@typechecked
class Prim(object):
    
    _builtin_types: List[str] = [
        "Model", # Generic Prim type
        "Scope",
        "Xform",
        "Mesh",
        "GeomSubset",
        "Camera",
        "Material",
        "Shader",
        "SphereLight",
        "DistantLight",
        "RectLight",
        # TODO: More Prim types...
        ]
    
    # TODO: better init constructor
    def __init__(self, prim_type:str = "Model", name: Optional[str] = None, from_handle = None):

        print("Prim ctor")
        if from_handle is not None:
            # Create a Python Prim instance with handle.
            # (Reference to existing C object. No copies of C object)

            print("Create Prim from handle.")
            self._handle = from_handle 
            #assert self._handle 

            assert name == None
            self._prim_type = ctinyusd.c_tinyusd_prim_type(self._handle)
            self._name = ctinyusd.c_tinyusd_prim_element_name()

            self._is_handle_reference = True

        else:
            # New Prim with prim_type
            if prim_type not in self._builtin_types:
                raise RuntimeError("Unsupported/unimplemented Prim type: ", prim_type)

            self._prim_type = prim_type
            err = ctinyusd.c_tinyusd_string_new_empty()
            self._handle = ctinyusd.c_tinyusd_prim_new(prim_type, err)

            if self._handle is False:
                raise RuntimeError("Failed to new Prim:" + ctinyusd.c_tinyusd_string_str(err))

            ctinyusd.c_tinyusd_string_free(err)

            self._is_handle_reference = False

    def __copy__(self):
        raise RuntimeError("Copying Prim in Python side is not supported at the moment.")

    def __deepcopy__(self, memo):
        print("deep copy")
        raise RuntimeError("Deep copying Prim in Python side is not supported at the moment.")

    def __del__(self):
        if not self._is_handle_reference:
            ret = ctinyusd.c_tinyusd_prim_free(self._handle)
            print("del", ret)

    #def __getattr__(self, name):
    #    if name == 'prim_type':
    #        return self._prim_type   
    #    else:
    #        raise RuntimeError("Unknown Python attribute name:", name)

    def name(self):
        return self._name

    def prim_type(self):
        return self._prim_type
            
    def children(self):
        # Return list
        # TODO: Consider use generator?(but len() is not available)

        child_list = []

        n = ctinyusd.c_tinyusd_prim_num_children(self._handle)
        for i in range(n):
            child_ptr = ctypes.POINTER(ctinyusd.CTinyUSDPrim)()

            ret = ctinyusd.c_tinyusd_prim_get_child(self._handle, i, ctypes.byref(child_ptr))
            assert ret

            child_prim = Prim(from_handle=child_ptr)

            child_list.append(child_prim)

        return child_list

    def add_child(self, child_prim):
        assert isinstance(child_prim, Prim)

        print("self:", self._handle)
        print("child:", child_prim._handle)
        ret = ctinyusd.c_tinyusd_prim_append_child(self._handle, child_prim._handle)
        print("append child:", ret)
        assert ret == 1


@typechecked
class Xform(Prim):
    def __init__(self, name: str):
        super().__init__(prim_type="Xform", name=name)

    def __del__(self):
        super().__del__()


@typechecked
class PrimChildIterator:
    def __init__(self, handle):
        self._handle = handle
        self._num_children = ctinyusd.c_tinyusd_prim_num_children(self._handle)
        self._current_index = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self._current_index < self._num_children:
            # Just a reference.
            child_ptr = ctypes.POINTER(ctinyusd.CTinyUSDPrim)
            ret = ctinyusd.c_tinyusd_prim_get_child(self._handle, ctypes.byref(child_ptr))
            assert ret == 1

            child_prim = Prim(from_handle=child_ptr)

        raise StopIteration

xform = Xform("xform0")

rel = Relationship("bora")

tok = Token("hello")
print(tok)

svalue = np.float32(1.3)
print(type(svalue))
print(isinstance(svalue, np.generic))

print("svalue.ndim", svalue.ndim)
tsval = Value(svalue)
print("tsval", tsval)


value = np.zeros(100, dtype=np.float32)
print(value)

tval = Value(value)

t = Token("mytok")
print(t)

#dt = copy.copy(t)
#del t
#print(dt)

root = Prim("Xform")
print("root", root.prim_type())

xform_prim = Prim("Xform")
print("child", xform_prim.prim_type())

root.add_child(xform_prim)
print("added child.")

print("# of child = ", len(root.children()))
for child in root.children():
    print(child)

#cp = Prim(from_handle=root._handle)
#print(cp)

#a = copy.deepcopy(p)

#for child in p.children():
#    print(child)

#print("del p")
#del p
