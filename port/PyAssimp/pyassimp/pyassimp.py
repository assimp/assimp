#-*- coding: UTF-8 -*-

"""
PyAssimp

This is the main-module of PyAssimp.
"""

import structs
import ctypes
import os
from ctypes import POINTER, c_int, c_uint, c_double, c_char, c_float


#get the assimp path
LIBRARY = os.path.join(os.path.dirname(__file__), "assimp.so")


class AssimpError(BaseException):
    """
    If ann internal error occures.
    """
    pass



class AssimpLib(object):
    #open library
    _dll = ctypes.cdll.LoadLibrary(LIBRARY)
    
    #get functions
    load = _dll.aiImportFile
    load.restype = POINTER(structs.SCENE)
    
    release = _dll.aiReleaseImport



class AssimpBase(object):
    """
    Base class for all Assimp-classes.
    """
    
    def _load_array(self, data, count, cons):
        """
        Loads a whole array out of data, and constructs a new object. If data
        is NULL, an empty list will be returned.
        
        data - pointer to array
        count - size of the array
        cons - constructor
        
        result array data
        """
        if data:
            return [cons(data[i]) for i in range(count)]
        else:
            return []


class Scene(AssimpBase):
    """
    The root structure of the imported data.
    Everything that was imported from the given file can be accessed from here.
    """
    
    #possible flags
    FLAGS = {1 : "AI_SCENE_FLAGS_ANIM_SKELETON_ONLY"}
    
    
    def __init__(self, model):
        """
        Converts the model-data to a real scene
        
        model - the raw model-data
        """
        #process data
        self._load(model)
    
    
    def _load(self, model):
        """
        Converts model from raw-data to fancy data!
        
        model - pointer to data
        """
        #store scene flags
        self.flags = model.flags
        
        #load mesh-data
        self.meshes = self._load_array(model.mMeshes,
                                       model.mNumMeshes,
                                       lambda x: Mesh(x.contents))
    
    
    def list_flags(self):
        """
        Returns a list of all used flags.
        
        result list of flags
        """
        return [name for (key, value) in Scene.FLAGS.iteritems()
                     if (key & self.flags)>0]


class Mesh(AssimpBase):
    """
    A mesh represents a geometry or model with a single material. 
    It usually consists of a number of vertices and a series of primitives/faces 
    referencing the vertices. In addition there might be a series of bones, each 
    of them addressing a number of vertices with a certain weight. Vertex data 
    is presented in channels with each channel containing a single per-vertex 
    information such as a set of texture coords or a normal vector.
    If a data pointer is non-null, the corresponding data stream is present.
    
    A Mesh uses only a single material which is referenced by a material ID.
    """
    
    def __init__(self, mesh):
        """
        Loads mesh from raw-data.
        """
        #process data
        self._load(mesh)
    
    
    def _load(self, mesh):
        """
        Loads mesh-data from raw data
        
        mesh - raw mesh-data
        """
        #converts a VECTOR3D-struct to a tuple
        vec2tuple = lambda x: (x.x, x.y, x.z)
        
        #load vertices
        self.vertices = self._load_array(mesh.mVertices,
                                         mesh.mNumVertices,
                                         vec2tuple)
        
        #load normals
        self.normals = self._load_array(mesh.mNormals,
                                        mesh.mNumVertices,
                                        vec2tuple)
        
        #load tangents
        self.tangents = self._load_array(mesh.mTangents,
                                         mesh.mNumVertices,
                                         vec2tuple)
        
        #load bitangents
        self.bitangents = self._load_array(mesh.mBitangents,
                                           mesh.mNumVertices,
                                           vec2tuple)
        
        #vertex color sets
        self.colors = self._load_colors(mesh)
        
        #number of coordinates per uv-channel
        self.uvsize = self._load_uv_component_count(mesh)
        
        #number of uv channels
        self.texcoords = self._load_texture_coords(mesh)
        
        #the used material
        self.material_index = mesh.mMaterialIndex
    
    
    def _load_uv_component_count(self, mesh):
        """
        Loads the number of components for a given UV channel.
        
        mesh - mesh-data
        
        result (count channel 1, count channel 2, ...)
        """
        return tuple(mesh.mNumUVComponents[i]
                     for i in range(structs.MESH.AI_MAX_NUMBER_OF_TEXTURECOORDS))
    
    
    def _load_texture_coords(self, mesh):
        """
        Loads texture coordinates.
        
        mesh - mesh-data
        
        result texture coordinates
        """
        result = []
        
        for i in range(structs.MESH.AI_MAX_NUMBER_OF_TEXTURECOORDS):
            result.append(self._load_array(mesh.mTextureCoords[i],
                                           mesh.mNumVertices,
                                           lambda x: (x.x, x.y, x.z)))
                
        return result
    
    
    def _load_colors(self, mesh):
        """
        Loads color sets.
        
        mesh - mesh with color sets
        
        result all color sets
        """
        result = []
        
        #for all possible sets
        for i in range(structs.MESH.AI_MAX_NUMBER_OF_COLOR_SETS):
            #try this set
            x = mesh.mColors[i]
            
            if x:
                channel = []
                
                #read data for al vertices!
                for j in range(mesh.mNumVertices):
                    c = x[j]
                    channel.append((c.r, c.g, c.b, c.a))
                
                result.append(channel)
                
        
        return result



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
    
    try:
        #create scene
        return Scene(model.contents)
    finally:
        #forget raw data
        _assimp_lib.release(model)