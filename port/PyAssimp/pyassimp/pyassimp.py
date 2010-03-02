#-*- coding: UTF-8 -*-

"""
PyAssimp

This is the main-module of PyAssimp.
"""

import structs
import ctypes
import os
import helper
from errors import AssimpError

class aiArray:
    """
    A python class to 'safely' access C arrays.
    For m<Name> and mNum<Name> assimp class members.
    """
    def __init__(self, instance, dataName, sizeName, i=None):
        self.instance = instance
        self.dataName = dataName
        self.sizeName = sizeName
        self.i = i
        self.count = 0
    
    def _GetSize(self):
        return getattr(self.instance, self.sizeName)
        
    def _GetData(self, index):
        if self.i != None:
            if not bool(getattr(self.instance, self.dataName)[self.i]):
                return None
            item = getattr(self.instance, self.dataName)[self.i][index]
        else:
            item = getattr(self.instance, self.dataName)[index]
        if hasattr(item, 'contents'):
            return item.contents._init()
        elif hasattr(item, '_init'):
            return item._init()
        else:
            return item
        
    def next(self):
        if self.count >= self._GetSize():
            self.count = 0
            raise StopIteration
        else:
            c = self.count
            self.count += 1
            return self._GetData(c)
            
    def __getitem__(self, index):
        if isinstance(index, slice):
            indices = index.indices(len(self))
            return [self.__getitem__(i) for i in range(*indices)]

        if index < 0 or index >= self._GetSize():
            raise IndexError("aiArray index out of range")
        return self._GetData(index)
        
    def __iter__(self):
        return self
        
    def __len__(self):
        return int(self._GetSize())
    
    def __str__(self):
        return str([x for x in self])
        
    def __repr__(self):
        return str([x for x in self])
        
class aiTuple:
    """
    A python class to 'safely' access C structs in a python tuple fashion.
    For C structs like vectors, matrices, colors, ...
    """
    def __init__(self, instance):
        self.instance = instance
        self.count = 0
    
    def _GetSize(self):
        return len(self.instance._fields_)
     
    def _GetData(self, index):
        return getattr(self.instance, self.instance._fields_[index][0])
        
    def next(self):
        if self.count >= self._GetSize():
            self.count = 0
            raise StopIteration
        else:
            c = self.count
            self.count += 1
            return self._GetData(c)
            
    def __getitem__(self, index):
        if isinstance(index, slice):
            indices = index.indices(len(self))
            return [self.__getitem__(i) for i in range(*indices)]
            
        if index < 0 or index >= self._GetSize():
            raise IndexError("aiTuple index out of range")
        return self._GetData(index)
        
    def __iter__(self):
        return self
        
    def __len__(self):
        return int(self._GetSize())
        
    def __str__(self):
        return str([x for x in self])
            
    def __repr__(self):
        return str([x for x in self])
        
def _init(self):
    """
    Custom initialize() for C structs, adds safely accessable member functionality.
    """
    if hasattr(self, '_is_init'):
        return self
    self._is_init = True

    if str(self.__class__.__name__) == "MaterialProperty":
        self.mKey._init()
        
    for m in self.__class__.__dict__.keys():
        if m.startswith('mNum'):
            name = m.split('mNum')[1]
            if 'm'+name in self.__class__.__dict__.keys():
                setattr(self.__class__, name.lower(), aiArray(self, 'm'+name , m))
                if name.lower() == "vertices":
                    setattr(self.__class__, "normals", aiArray(self, 'mNormals' , m))
                    setattr(self.__class__, "tangents", aiArray(self, 'mTangents' , m))
                    setattr(self.__class__, "bitangets", aiArray(self, 'mBitangents' , m))
                    setattr(self.__class__, "colors", [aiArray(self, 'mColors' , m, o) for o in xrange(len(self.mColors))])
                    setattr(self.__class__, "texcoords", [aiArray(self, 'mTextureCoords' , m, o) for o in xrange(len(self.mColors))])
                
        elif m == "x" or m == "a1" or m == "b": # Vector, matrix, quat, color
            self._tuple = aiTuple(self)
            setattr(self.__class__, '__getitem__', lambda x, y: x._tuple.__getitem__(y))
            setattr(self.__class__, '__iter__', lambda x: x._tuple)
            setattr(self.__class__, 'next', lambda x: x._tuple.next)
            setattr(self.__class__, '__repr__', lambda x: str([c for c in x]))
            break
        elif m == "data": #String
            setattr(self.__class__, '__repr__', lambda x: str(x.data))
            setattr(self.__class__, '__str__', lambda x: str(x.data))
            break
            
        if hasattr(getattr(self, m), '_init'):
            getattr(self, m)._init()
        
    return self

"""
Python magic to add the _init() function to all C struct classes.
"""
for struct in dir(structs):
    if not (struct.startswith('_') or struct.startswith('c_') or struct == "Structure" or struct == "POINTER"):
        setattr(getattr(structs, struct), '_init', _init)


class AssimpLib(object):
    """
    Assimp-Singleton
    """
    load, release, dll = helper.search_library()

#the loader as singleton
_assimp_lib = AssimpLib()


def load(filename, processing=0):
    """
    Loads the model with some specific processing parameters.
    
    filename - file to load model from
    processing - processing parameters
    
    result Scene-object with model-data
    
    throws AssimpError - could not open file
    """
    #read pure data
    model = _assimp_lib.load(filename, processing)
    if not model:
        #Uhhh, something went wrong!
        raise AssimpError, ("could not import file: %s" % filename)

    return model.contents._init()

def release(scene):
    from ctypes import pointer
    _assimp_lib.release(pointer(scene))
    
def aiGetMaterialFloatArray(material, key):
    AI_SUCCESS = 0
    from ctypes import byref, pointer, cast, c_float, POINTER, sizeof, c_uint
    out = structs.Color4D()
    max = c_uint(sizeof(structs.Color4D))
    r=_assimp_lib.dll.aiGetMaterialFloatArray(pointer(material), 
                                            key[0], 
                                            key[1], 
                                            key[2], 
                                            byref(out), 
                                            byref(max))
                                            
    if (r != AI_SUCCESS):    
        raise AssimpError("aiGetMaterialFloatArray failed!")
      
    out._init()
    return [out[i] for i in xrange(max.value)]
    
def aiGetMaterialString(material, key):
    AI_SUCCESS = 0
    from ctypes import byref, pointer, cast, c_float, POINTER, sizeof, c_uint
    out = structs.String()
    r=_assimp_lib.dll.aiGetMaterialString(pointer(material), 
                                            key[0], 
                                            key[1], 
                                            key[2], 
                                            byref(out))
                                            
    if (r != AI_SUCCESS):    
        raise AssimpError("aiGetMaterialString failed!")
        
    return str(out.data)
 
def GetMaterialProperties(material): 
    """
    Convenience Function to get the material properties as a dict
    and values in a python format.
    """
    result = {}
    #read all properties
    for p in material.properties:
        #the name
        key = p.mKey.data

        #the data
        from ctypes import POINTER, cast, c_int, c_float, sizeof
        if p.mType == 1:
            arr = cast(p.mData, POINTER(c_float*(p.mDataLength/sizeof(c_float)) )).contents
            value = [x for x in arr]
        elif p.mType == 3: #string can't be an array
            value = cast(p.mData, POINTER(structs.String)).contents.data
        elif p.mType == 4:
            arr = cast(p.mData, POINTER(c_int*(p.mDataLength/sizeof(c_int)) )).contents
            value = [x for x in arr]
        else:
            value = p.mData[:p.mDataLength]

        result[key] = value

    return result
    
    
def aiDecomposeMatrix(matrix):
    if not isinstance(matrix, structs.Matrix4x4):
        raise AssimpError("aiDecomposeMatrix failed: Not a aiMatrix4x4!")
    
    scaling = structs.Vector3D()
    rotation = structs.Quaternion()
    position = structs.Vector3D()
    
    from ctypes import byref, pointer
    _assimp_lib.dll.aiDecomposeMatrix(pointer(matrix), byref(scaling), byref(rotation), byref(position))
    return scaling._init(), rotation._init(), position._init()
