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



class AssimpLib(object):
    """
    Assimp-Singleton
    """
    load, release = helper.search_library()



class AssimpBase(object):
    """
    Base class for all Assimp-classes.
    """
    
    @staticmethod
    def _load_array(data, count, cons):
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
    
    
    @staticmethod
    def make_loader(function):
        """
        Creates a loader function for "_load_array".
        
        function - function to be applied to the content of an element
        """
        def loader(x):
            return function(x.contents)
        
        return loader


class Material(object):
    """
    A Material.
    """
    
    def __init__(self, material):
        """
        Converts the raw material data to a material.
        """
        self.properties = self._load_properties(material.mProperties,
                                                material.mNumProperties)
    
    
    def _load_properties(self, data, size):
        """
        Loads all properties of this mateiral.
        
        data - properties
        size - elements in properties
        """
        result = {}
        
        #read all properties
        for i in range(size):
            p = data[i].contents
            
            #the name
            key = p.mKey.data
            
            #the data
            value = p.mData[:p.mDataLength]
            
            result[key] = str(value)
        
        return result
    
    
    def __repr__(self):
        return repr(self.properties)
    
    
    def __str__(self):
        return str(self.properties)


class Matrix(AssimpBase):
    """
    Assimp 4x4-matrix
    """
    def __init__(self, matrix):
        """
        Copies matrix data to this structure.
        
        matrix - raw matrix data
        """
        m = matrix
        
        self.data = [
                     [m.a1, m.a2, m.a3, m.a4],
                     [m.b1, m.b2, m.b3, m.b4],
                     [m.c1, m.c2, m.c3, m.c4],
                     [m.d1, m.d2, m.d3, m.d4],
                     ]
    
    
    def __getitem__(self, index):
        """
        Returns an item out of the matrix data. Use (row, column) to access
        data directly or an natural number n to access the n-th row.
        
        index - matrix index
        
        result element or row
        """
        try:
            #tuple as index?
            x, y = index
            return data[x][y]
        except TypeError:
            #index as index
            return data[index]
    
    
    def __setitem__(self, index, value):
        """
        Sets an item of the matrix data. Use (row, column) to access
        data directly or an natural number n to access the n-th row.
        
        index - matrix index
        value - new value
        """
        try:
            #tuple as index?
            x, y = index
            data[x][y] = value
        except TypeError:
            #index as index
            data[index] = value


class VertexWeight(AssimpBase):
    """
    Weight for vertices.
    """
    
    def __init__(self, weight):
        """
        Copies vertex weights to this structure.
        
        weight - new weight
        """
        #corresponding vertex id
        self.vertex = weight.mVertexId
        
        #my weight
        self.weight = weight.mWeight


class Bone(AssimpBase):
    """
    Single bone of a mesh. A bone has a name by which it can be found 
    in the frame hierarchy and by which it can be addressed by animations.
    """
    
    def __init__(self, bone):
        """
        Converts an ASSIMP-bone to a PyAssimp-bone.
        """
        #the name is easy
        self.name = str(bone.mName)
        
        #matrix that transforms from mesh space to bone space in bind pose
        self.matrix = Matrix(bone.mOffsetMatrix)
        
        #and of course the weights!
        Bone._load_array(bone.mWeights,
                         bone.mNumWeights,
                         VertexWeight)


class Texture(AssimpBase):
    """
    Texture included in the model.
    """
    
    def __init__(self, texture):
        """
        Convertes the raw data to a texture.
        
        texture - raw data
        """
        #dimensions
        self.width = texture.mWidth
        self.height = texture.mHeight
        
        #format hint
        self.hint = texture.achFormatHint
        
        #load data
        self.data = self._load_data(texture)
    
    
    def _load_data(self, texture):
        """
        Loads the texture data.
        
        texture - the texture
        
        result texture data in (red, green, blue, alpha)
        """
        if self.height == 0:
            #compressed data
            size = self.width
        else:
            size = self.width * self.height
        
        #load!
        return Texture._load_array(texture.pcData,
                                   size,
                                   lambda x: (x.r, x.g, x.b, x.a))


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
        self.meshes = Scene._load_array(model.mMeshes,
                                        model.mNumMeshes,
                                        Scene.make_loader(Mesh))
        
        #load materials
        self.materials = Scene._load_array(model.mMaterials,
                                           model.mNumMaterials,
                                           Scene.make_loader(Material))
        
        #load textures
        self.textures = Scene._load_array(model.mTextures,
                                          model.mNumTextures,
                                          Scene.make_loader(Texture))
    
    
    def list_flags(self):
        """
        Returns a list of all used flags.
        
        result list of flags
        """
        return [name for (key, value) in Scene.FLAGS.iteritems()
                     if (key & self.flags)>0]


class Face(AssimpBase):
    """
    A single face in a mesh, referring to multiple vertices. 
    If the number of indices is 3, the face is a triangle, 
    for more than 3  it is a polygon.
    
    Point and line primitives are rarely used and are NOT supported. However,
    a load could pass them as degenerated triangles.
    """
    
    def __init__(self, face):
        """
        Loads a face from raw-data.
        """
        self.indices = [face.mIndices[i] for i in range(face.mNumIndices)]
    
    
    def __repr__(self):
        return str(self.indices)


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
        #load vertices
        self.vertices = Mesh._load_array(mesh.mVertices,
                                         mesh.mNumVertices,
                                         helper.vec2tuple)
        
        #load normals
        self.normals = Mesh._load_array(mesh.mNormals,
                                        mesh.mNumVertices,
                                        helper.vec2tuple)
        
        #load tangents
        self.tangents = Mesh._load_array(mesh.mTangents,
                                         mesh.mNumVertices,
                                         helper.vec2tuple)
        
        #load bitangents
        self.bitangents = Mesh._load_array(mesh.mBitangents,
                                           mesh.mNumVertices,
                                           helper.vec2tuple)
        
        #vertex color sets
        self.colors = self._load_colors(mesh)
        
        #number of coordinates per uv-channel
        self.uvsize = self._load_uv_component_count(mesh)
        
        #number of uv channels
        self.texcoords = self._load_texture_coords(mesh)
        
        #the used material
        self.material_index = int(mesh.mMaterialIndex)
        
        #faces
        self.faces = self._load_faces(mesh)
        
        #bones
        self.bones = self._load_bones(mesh)
    
    
    def _load_bones(self, mesh):
        """
        Loads bones of this mesh.
        
        mesh - mesh-data
        
        result bones
        """
        count = mesh.mNumBones
        
        if count==0:
            #no bones
            return []
        
        #read bones
        bones = mesh.mBones.contents
        return Mesh._load_array(bones,
                                count,
                                Bone)
    
    
    def _load_faces(self, mesh):
        """
        Loads all faces.
        
        mesh - mesh-data
        
        result faces
        """
        return [Face(mesh.mFaces[i]) for i in range(mesh.mNumFaces)]
    
    
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
            result.append(Mesh._load_array(mesh.mTextureCoords[i],
                                           mesh.mNumVertices,
                                           helper.vec2tuple))
                
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