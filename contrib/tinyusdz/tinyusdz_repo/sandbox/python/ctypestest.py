import ctypes

class CPrim(ctypes.Structure):
    _fields_ = [ ("data", ctypes.c_void_p) ]

class CString(ctypes.Structure):
    _fields_ = [ ("data", ctypes.c_void_p) ]

lib = ctypes.CDLL("../../build/libc-tinyusd.so")
print(lib)

s = CString()
#ret = lib.c_tinyusd_string_new_empty(ctypes.byref(s))
ret = lib.c_tinyusd_string_new(ctypes.byref(s), b"bora")
print(ret)

lib.c_tinyusd_string_str.restype = ctypes.c_char_p
lib.c_tinyusd_string_str.argstypes = [ctypes.POINTER(CString)]
print(lib.c_tinyusd_string_str.restype)
msg = lib.c_tinyusd_string_str(ctypes.byref(s))
print(msg.decode())

ret = lib.c_tinyusd_string_free(ctypes.byref(s))
print(ret)

prim = CPrim()
print(prim)

lib.c_tinyusd_prim_new.restype = ctypes.c_int
lib.c_tinyusd_prim_new.argtypes = [ctypes.c_char_p, ctypes.POINTER(CPrim)]

prim_type = "Xform"
ret = lib.c_tinyusd_prim_new(prim_type.encode(), ctypes.byref(prim))
print(ret)

lib.c_tinyusd_prim_free.restype = ctypes.c_int
lib.c_tinyusd_prim_free.argtypes = [ctypes.POINTER(CPrim)]
ret = lib.c_tinyusd_prim_free(ctypes.byref(prim))
print(ret) # => 1

# double-free
ret = lib.c_tinyusd_prim_free(ctypes.byref(prim))
print(ret) # => 0
