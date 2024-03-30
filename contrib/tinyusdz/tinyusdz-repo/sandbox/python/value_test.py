import ctypes
import struct

import ctinyusdz
import array

arr = array.array('i', [1, 2, 3, 4])
print(arr)

int1d = (ctypes.c_int * 10)()
print(int1d)

fltval = ctypes.c_float(1.3)
print(fltval)

pvar = ctinyusdz.PrimVar()
pvar.set_buf(arr)
pvar.set_buf(fltval)

pvar.set_obj(fltval)
pvar.set_obj([1.3]) # fltval)
#pvar.set(fltval)
#pvar.set_array(int1d)

#gv = pvar.get_array()
# numpy.array, but we can use it without numpy module.
#print(gv.size)
#print(gv)

#ctinyiusdz.
#
