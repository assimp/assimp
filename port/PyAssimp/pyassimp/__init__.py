#-*- coding: UTF-8 -*-

"""
PyAssimp

This is the main-module of PyAssimp.
"""

import sys
if sys.version_info < (2,5):
	raise 'pyassimp: need python 2.5 or newer'


import ctypes
import os
import helper
from errors import AssimpError

import structs

import logging; logger = logging.getLogger("pyassimp")

#logging.basicConfig(level=logging.DEBUG)
logging.basicConfig()

assimp_structs_as_tuple = (
        structs.Matrix4x4, 
        structs.Matrix3x3, 
        structs.Vector2D, 
        structs.Vector3D, 
        structs.Color3D, 
        structs.Color4D, 
        structs.Quaternion, 
        structs.Plane, 
        structs.Texel)

def make_tuple(ai_obj):
    return tuple([getattr(ai_obj, e[0]) for e in ai_obj._fields_])

def call_init(obj, caller = None):
        # init children
        if hasattr(obj, '_init'):
            logger.debug("Init of children: ")
            obj._init()
            logger.debug("Back to " + str(caller))

        # pointers
        elif hasattr(obj, 'contents'):
            if hasattr(obj.contents, '_init'):
                logger.debug("Init of children (pointer): ")
                obj.contents._init(target = obj)
                logger.debug("Back to " + str(caller))



def _init(self, target = None):
    """
    Custom initialize() for C structs, adds safely accessable member functionality.

    :param target: set the object which receive the added methods. Useful when manipulating
    pointers, to skip the intermediate 'contents' deferencing.
    """
    if hasattr(self, '_is_init'):
        logger.debug("" + str(self.__class__) + " already initialized.")
        return self
    self._is_init = True
    
    if not target:
        target = self

    #for m in self.__class__.__dict__.keys():


    for m in dir(self):

        name = m[1:].lower()
 
        #if 'normals' in name : import pdb;pdb.set_trace()
        
        if m.startswith("_"):
            continue

        obj = getattr(self, m)

        if m.startswith('mNum'):
            if 'm' + m[4:] in dir(self):
                continue # will be processed later on
            else:
                setattr(target, name, obj)



        # Create tuples
        if isinstance(obj, assimp_structs_as_tuple):
            setattr(target, name, make_tuple(obj))
            logger.debug(str(self) + ": Added tuple " + str(getattr(target, name)) +  " as self." + name.lower())
            continue


        if isinstance(obj, structs.String):
            setattr(target, 'name', str(obj.data))
            setattr(target.__class__, '__repr__', lambda x: str(x.__class__) + "(" + x.name + ")")
            setattr(target.__class__, '__str__', lambda x: x.name)
            continue
        

        if m.startswith('m'):


            if hasattr(self, 'mNum' + m[1:]):

                length =  getattr(self, 'mNum' + m[1:])
    
                # -> special case: properties are
                # stored as a dict.
                if m == 'mProperties':
                    setattr(target, name, _get_properties(obj, length))
                    continue


                #import pdb;pdb.set_trace()
                if not obj: # empty!
                    setattr(target, name, [])
                    logger.debug(str(self) + ": " + name + " is an empty list.")
                    continue
                

                try:
                    if obj._type_ in assimp_structs_as_tuple:
                        setattr(target, name, [make_tuple(obj[i]) for i in xrange(length)])

                        logger.debug(str(self) + ": Added a list data (type "+ str(type(obj)) + ") as self." + name)

                    else:
                        setattr(target, name, [obj[i] for i in xrange(length)]) #TODO: maybe not necessary to recreate an array?

                        logger.debug(str(self) + ": Added list of " + str(obj) + " " + name + " as self." + name + " (type: " + str(type(obj)) + ")")

                        # initialize array elements
                        for e in getattr(target, name):
                            call_init(e, caller = self)


                except IndexError:
                    logger.warning("in " + str(self) +" : mismatch between mNum" + name + " and the actual amount of data in m" + name + ". This may be due to version mismatch between libassimp and pyassimp. Skipping this field.")
                except ValueError as e:
                    logger.warning(str(e))
                    logger.warning("in " + str(self) +" : table of " + name + " not initialized (NULL pointer). Skipping this field.")


            else: # starts with 'm' but not iterable

                setattr(target, name, obj)
                logger.debug("Added " + name + " as self." + name + " (type: " + str(type(obj)) + ")")

                if name not in ["parent"]: # do not re-initialize my parent
                    call_init(obj, caller = self)



    if isinstance(self, structs.Mesh):
        _finalize_mesh(self, target)

    if isinstance(self, structs.Texture):
        _finalize_texture(self, target)


    return self


"""
Python magic to add the _init() function to all C struct classes.
"""
for struct in dir(structs):
    if not (struct.startswith('_') or struct.startswith('c_') or struct == "Structure" or struct == "POINTER") and not isinstance(getattr(structs, struct),int):

        logger.debug("Adding _init to class " +  struct)
        setattr(getattr(structs, struct), '_init', _init)


class AssimpLib(object):
    """
    Assimp-Singleton
    """
    load, release, dll = helper.search_library()

#the loader as singleton
_assimp_lib = AssimpLib()

def pythonize_assimp(type, obj, scene):
    """ This method modify the Assimp data structures
    to make them easier to work with in Python.

    Supported operations:
     - MESH: replace a list of mesh IDs by reference to these meshes

    :param type: the type of modification to operate (cf above)
    :param obj: the input object to modify
    :param scene: a reference to the whole scene
    """

    if type == "MESH":
        meshes = []
        for i in obj:
            meshes.append(scene.meshes[i])
        return meshes


def recur_pythonize(node, scene):
    """ Recursively call pythonize_assimp on
    nodes tree to apply several post-processing to
    pythonize the assimp datastructures.
    """

    node.meshes = pythonize_assimp("MESH", node.meshes, scene)

    for c in node.children:
        recur_pythonize(c, scene)


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

    #logger.debug("Initializing recursively tuples")
    #_init_tuples(model.contents)

    logger.debug("Initializing recursively objects")
    scene = model.contents._init()

    recur_pythonize(scene.rootnode, scene)
    
    return scene

def release(scene):
    from ctypes import pointer
    _assimp_lib.release(pointer(scene))

def _finalize_texture(tex, target):
    setattr(target, "achformathint", tex.achFormatHint)
    data = [make_tuple(getattr(tex, pcData)[i]) for i in xrange(tex.mWidth * tex.mHeight)]
    setattr(target, "data", data)



def _finalize_mesh(mesh, target):
    """ Building of meshes is a bit specific.

    We override here the various datasets that can
    not be process as regular fields.

    For instance, the length of the normals array is
    mNumVertices (no mNumNormals is available)
    """
    nb_vertices = getattr(mesh, "mNumVertices")

    def fill(name):
        mAttr = getattr(mesh, name)
        if mAttr:
            data = [make_tuple(getattr(mesh, name)[i]) for i in xrange(nb_vertices)]
            setattr(target, name[1:].lower(), data)
        else:
            setattr(target, name[1:].lower(), [])

    def fillarray(name):
        mAttr = getattr(mesh, name)

        data = []
        for index, mSubAttr in enumerate(mAttr):
            if mSubAttr:
                data.append([make_tuple(getattr(mesh, name)[index][i]) for i in xrange(nb_vertices)])

        setattr(target, name[1:].lower(), data)

    fill("mNormals")
    fill("mTangents")
    fill("mBitangents")

    fillarray("mColors")
    fillarray("mTextureCoords")

def _get_properties(properties, length): 
    """
    Convenience Function to get the material properties as a dict
    and values in a python format.
    """
    result = {}
    #read all properties
    for p in [properties[i] for i in xrange(length)]:
        #the name
        p = p.contents
        key = str(p.mKey.data)

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

    
    
def decompose_matrix(matrix):
    if not isinstance(matrix, structs.Matrix4x4):
        raise AssimpError("pyassimp.decompose_matrix failed: Not a Matrix4x4!")
    
    scaling = structs.Vector3D()
    rotation = structs.Quaternion()
    position = structs.Vector3D()
    
    from ctypes import byref, pointer
    _assimp_lib.dll.aiDecomposeMatrix(pointer(matrix), byref(scaling), byref(rotation), byref(position))
    return scaling._init(), rotation._init(), position._init()
