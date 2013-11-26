#-*- coding: UTF-8 -*-

"""
PyAssimp

This is the main-module of PyAssimp.
"""

import sys
if sys.version_info < (2,6):
    raise 'pyassimp: need python 2.6 or newer'


import ctypes
import os
import numpy

import logging; logger = logging.getLogger("pyassimp")

# Attach a default, null handler, to the logger.
# applications can easily get log messages from pyassimp
# by calling for instance
# >>> logging.basicConfig(level=logging.DEBUG)
# before importing pyassimp
class NullHandler(logging.Handler):
    def emit(self, record):
        pass
h = NullHandler()
logger.addHandler(h)

from . import structs
from .errors import AssimpError
from . import helper


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

def make_tuple(ai_obj, type = None):
    res = None

    if isinstance(ai_obj, structs.Matrix4x4):
        res = numpy.array([getattr(ai_obj, e[0]) for e in ai_obj._fields_]).reshape((4,4))
        #import pdb;pdb.set_trace()
    elif isinstance(ai_obj, structs.Matrix3x3):
        res = numpy.array([getattr(ai_obj, e[0]) for e in ai_obj._fields_]).reshape((3,3))
    else:
        res = numpy.array([getattr(ai_obj, e[0]) for e in ai_obj._fields_])

    return res

# It is faster and more correct to have an init function for each assimp class
def _init_face(aiFace):
    aiFace.indices = [aiFace.mIndices[i] for i in range(aiFace.mNumIndices)]
    
assimp_struct_inits = \
    { structs.Face : _init_face }
    
def call_init(obj, caller = None):
    if helper.hasattr_silent(obj,'contents'): #pointer
        _init(obj.contents, obj, caller)
    else:
        _init(obj,parent=caller)

def _is_init_type(obj):
    if helper.hasattr_silent(obj,'contents'): #pointer
        return _is_init_type(obj[0])
    # null-pointer case that arises when we reach a mesh attribute
    # like mBitangents which use mNumVertices rather than mNumBitangents
    # so it breaks the 'is iterable' check.
    # Basically:
    # FIXME!
    elif not bool(obj): 
        return False
    tname = obj.__class__.__name__
    return not (tname[:2] == 'c_' or tname == 'Structure' \
            or tname == 'POINTER') and not isinstance(obj,int)
                    
def _init(self, target = None, parent = None):
    """
    Custom initialize() for C structs, adds safely accessable member functionality.

    :param target: set the object which receive the added methods. Useful when manipulating
    pointers, to skip the intermediate 'contents' deferencing.
    """
    if not target:
        target = self
    
    dirself = dir(self) 
    for m in dirself:

        if m.startswith("_"):
            continue

        if m.startswith('mNum'):
            if 'm' + m[4:] in dirself:
                continue # will be processed later on
            else:
                name = m[1:].lower()

                obj = getattr(self, m)
                setattr(target, name, obj)
                continue

        if m == 'mName':
            obj = self.mName
            target.name = str(obj.data.decode("utf-8"))
            target.__class__.__repr__ = lambda x: str(x.__class__) + "(" + x.name + ")"
            target.__class__.__str__ = lambda x: x.name
            continue
            
        name = m[1:].lower()

        obj = getattr(self, m)

        # Create tuples
        if isinstance(obj, assimp_structs_as_tuple):
            setattr(target, name, make_tuple(obj))
            logger.debug(str(self) + ": Added array " + str(getattr(target, name)) +  " as self." + name.lower())
            continue

        if m.startswith('m'):

            if name == "parent":
                setattr(target, name, parent)
                logger.debug("Added a parent as self." + name)
                continue

            if helper.hasattr_silent(self, 'mNum' + m[1:]):

                length =  getattr(self, 'mNum' + m[1:])
    
                # -> special case: properties are
                # stored as a dict.
                if m == 'mProperties':
                    setattr(target, name, _get_properties(obj, length))
                    continue


                if not length: # empty!
                    setattr(target, name, [])
                    logger.debug(str(self) + ": " + name + " is an empty list.")
                    continue
                

                try:
                    if obj._type_ in assimp_structs_as_tuple:
                        setattr(target, name, numpy.array([make_tuple(obj[i]) for i in range(length)], dtype=numpy.float32))

                        logger.debug(str(self) + ": Added an array of numpy arrays (type "+ str(type(obj)) + ") as self." + name)

                    else:
                        setattr(target, name, [obj[i] for i in range(length)]) #TODO: maybe not necessary to recreate an array?

                        logger.debug(str(self) + ": Added list of " + str(obj) + " " + name + " as self." + name + " (type: " + str(type(obj)) + ")")

                        # initialize array elements
                        try:
                            init = assimp_struct_inits[type(obj[0])]
                        except KeyError:
                            if _is_init_type(obj[0]):
                                for e in getattr(target, name):
                                    call_init(e, target)
                        else:
                            for e in getattr(target, name):
                                init(e)


                except IndexError:
                    logger.error("in " + str(self) +" : mismatch between mNum" + name + " and the actual amount of data in m" + name + ". This may be due to version mismatch between libassimp and pyassimp. Quitting now.")
                    sys.exit(1)

                except ValueError as e:
                    
                    logger.error("In " + str(self) +  "->" + name + ": " + str(e) + ". Quitting now.")
                    if "setting an array element with a sequence" in str(e):
                        logger.error("Note that pyassimp does not currently "
                                     "support meshes with mixed triangles "
                                     "and quads. Try to load your mesh with"
                                     " a post-processing to triangulate your"
                                     " faces.")
                    sys.exit(1)


            else: # starts with 'm' but not iterable

                setattr(target, name, obj)
                logger.debug("Added " + name + " as self." + name + " (type: " + str(type(obj)) + ")")
        
                if _is_init_type(obj):
                    call_init(obj, target)




    if isinstance(self, structs.Mesh):
        _finalize_mesh(self, target)

    if isinstance(self, structs.Texture):
        _finalize_texture(self, target)


    return self

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
     - ADDTRANSFORMATION: add a reference to an object's transformation taken from their associated node.

    :param type: the type of modification to operate (cf above)
    :param obj: the input object to modify
    :param scene: a reference to the whole scene
    """

    if type == "MESH":
        meshes = []
        for i in obj:
            meshes.append(scene.meshes[i])
        return meshes

    if type == "ADDTRANSFORMATION":

        def getnode(node, name):
            if node.name == name: return node
            for child in node.children:
                n = getnode(child, name)
                if n: return n

        node = getnode(scene.rootnode, obj.name)
        if not node:
            raise AssimpError("Object " + str(obj) + " has no associated node!")
        setattr(obj, "transformation", node.transformation)


def recur_pythonize(node, scene):
    """ Recursively call pythonize_assimp on
    nodes tree to apply several post-processing to
    pythonize the assimp datastructures.
    """

    node.meshes = pythonize_assimp("MESH", node.meshes, scene)

    for mesh in node.meshes:
        mesh.material = scene.materials[mesh.materialindex]

    for cam in scene.cameras:
        pythonize_assimp("ADDTRANSFORMATION", cam, scene)

    #for light in scene.lights:
    #    pythonize_assimp("ADDTRANSFORMATION", light, scene)

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
    #from ctypes import c_char_p, c_uint
    #model = _assimp_lib.load(c_char_p(filename), c_uint(processing))
    model = _assimp_lib.load(filename.encode("ascii"), processing)
    if not model:
        #Uhhh, something went wrong!
        raise AssimpError("could not import file: %s" % filename)

    scene = _init(model.contents)

    recur_pythonize(scene.rootnode, scene)

    return scene

def release(scene):
    from ctypes import pointer
    _assimp_lib.release(pointer(scene))

def _finalize_texture(tex, target):
    setattr(target, "achformathint", tex.achFormatHint)
    data = numpy.array([make_tuple(getattr(tex, "pcData")[i]) for i in range(tex.mWidth * tex.mHeight)])
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
            data = numpy.array([make_tuple(getattr(mesh, name)[i]) for i in range(nb_vertices)], dtype=numpy.float32)
            setattr(target, name[1:].lower(), data)
        else:
            setattr(target, name[1:].lower(), numpy.array([], dtype="float32"))

    def fillarray(name):
        mAttr = getattr(mesh, name)

        data = []
        for index, mSubAttr in enumerate(mAttr):
            if mSubAttr:
                data.append([make_tuple(getattr(mesh, name)[index][i]) for i in range(nb_vertices)])

        setattr(target, name[1:].lower(), numpy.array(data, dtype=numpy.float32))

    fill("mNormals")
    fill("mTangents")
    fill("mBitangents")

    fillarray("mColors")
    fillarray("mTextureCoords")
    
    # prepare faces
    faces = numpy.array([f.indices for f in target.faces], dtype=numpy.int32)
    setattr(target, 'faces', faces)


class PropertyGetter(dict):
    def __getitem__(self, key):
        semantic = 0
        if isinstance(key, tuple):
            key, semantic = key

        return dict.__getitem__(self, (key, semantic))

    def keys(self):
        for k in dict.keys(self):
            yield k[0]

    def __iter__(self):
        return self.keys()

    def items(self):
        for k, v in dict.items(self):
            yield k[0], v


def _get_properties(properties, length): 
    """
    Convenience Function to get the material properties as a dict
    and values in a python format.
    """
    result = {}
    #read all properties
    for p in [properties[i] for i in range(length)]:
        #the name
        p = p.contents
        key = (str(p.mKey.data.decode("utf-8")).split('.')[1], p.mSemantic)

        #the data
        from ctypes import POINTER, cast, c_int, c_float, sizeof
        if p.mType == 1:
            arr = cast(p.mData, POINTER(c_float * int(p.mDataLength/sizeof(c_float)) )).contents
            value = [x for x in arr]
        elif p.mType == 3: #string can't be an array
            value = cast(p.mData, POINTER(structs.MaterialPropertyString)).contents.data.decode("utf-8")
        elif p.mType == 4:
            arr = cast(p.mData, POINTER(c_int * int(p.mDataLength/sizeof(c_int)) )).contents
            value = [x for x in arr]
        else:
            value = p.mData[:p.mDataLength]

        if len(value) == 1:
            [value] = value

        result[key] = value

    return PropertyGetter(result)

def decompose_matrix(matrix):
    if not isinstance(matrix, structs.Matrix4x4):
        raise AssimpError("pyassimp.decompose_matrix failed: Not a Matrix4x4!")
    
    scaling = structs.Vector3D()
    rotation = structs.Quaternion()
    position = structs.Vector3D()
    
    from ctypes import byref, pointer
    _assimp_lib.dll.aiDecomposeMatrix(pointer(matrix), byref(scaling), byref(rotation), byref(position))
    return scaling._init(), rotation._init(), position._init()
