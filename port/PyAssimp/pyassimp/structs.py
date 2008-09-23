#-*- coding: UTF-8 -*-

"""
All ASSIMP C-structures. See the Assimp-headers for all formats.
"""

from ctypes import POINTER, c_int, c_uint, c_char, c_float, Structure, c_char_p, c_double, c_ubyte



class STRING(Structure):
    """
    Represents a String in ASSIMP.
    """
    
    #Maximum length of a string. See "MAXLEN" in "aiTypes.h"
    MAXLEN = 1024
    
    _fields_ = [
            #length of the string
            ("length", c_uint),                                                 #OK
            
            #string data
            ("data", c_char*MAXLEN)                                             #OK
        ]



class MATRIX4X4(Structure):
    """
    ASSIMP's 4x4-matrix
    """
    
    _fields_ = [
        #all the fields
        ("a1", c_float), ("a2", c_float), ("a3", c_float), ("a4", c_float),     #OK
        ("b1", c_float), ("b2", c_float), ("b3", c_float), ("b4", c_float),     #OK
        ("c1", c_float), ("c2", c_float), ("c3", c_float), ("c4", c_float),     #OK
        ("d1", c_float), ("d2", c_float), ("d3", c_float), ("d4", c_float),     #OK
    ]



class NODE(Structure):
    """
    A node in the imported hierarchy.
    
    Each node has name, a parent node (except for the root node), 
    a transformation relative to its parent and possibly several child nodes.
    Simple file formats don't support hierarchical structures, for these formats 
    the imported scene does consist of only a single root node with no childs.
    """
    pass

NODE._fields_ = [
            #The name of the node
            ("mName", STRING),
            
            #The transformation relative to the node's parent.
            ("aiMatrix4x4", MATRIX4X4),
            
            #Parent node. NULL if this node is the root node.
            ("mParent", POINTER(NODE)),
            
            #The number of child nodes of this node.
            ("mNumChildren", c_uint),
            
            #The child nodes of this node. NULL if mNumChildren is 0.
            ("mChildren", POINTER(POINTER(NODE))),
            
            #The number of meshes of this node.
            ("mNumMeshes", c_uint),
            
            #The meshes of this node. Each entry is an index into the mesh.
            ("mMeshes", POINTER(c_int))
        ]


class VECTOR3D(Structure):
    """
    Represents a three-dimensional vector.
    """
    
    _fields_ = [
            ("x", c_float),
            ("y", c_float),
            ("z", c_float)
        ]


class COLOR4D(Structure):
    """
    Represents a color in Red-Green-Blue space including an alpha component.
    """
    
    _fields_ = [
            ("r", c_float),
            ("g", c_float),
            ("b", c_float),
            ("a", c_float)
        ]


class FACE(Structure):
    """
    A single face in a mesh, referring to multiple vertices. 
    If mNumIndices is 3, the face is a triangle, 
    for mNumIndices > 3 it's a polygon.
    Point and line primitives are rarely used and are NOT supported. However,
    a load could pass them as degenerated triangles.
    """
    
    _fields_ = [
            #Number of indices defining this face. 3 for a triangle, >3 for polygon
            ("mNumIndices", c_uint),                                            #OK
            
            #Pointer to the indices array. Size of the array is given in numIndices.
            ("mIndices", POINTER(c_uint))                                       #OK
        ]


class VERTEXWEIGHT(Structure):
    """
    A single influence of a bone on a vertex.
    """
    
    _fields_ = [
            #Index of the vertex which is influenced by the bone.
            ("mVertexId", c_uint),                                              #OK
        
            #The strength of the influence in the range (0...1).
            #The influence from all bones at one vertex amounts to 1.
            ("mWeight", c_float)                                                #OK
        ]


class BONE(Structure):
    """
    A single bone of a mesh. A bone has a name by which it can be found 
    in the frame hierarchy and by which it can be addressed by animations. 
    In addition it has a number of influences on vertices.
    """
    
    _fields_ = [
            #The name of the bone. 
            ("mName", STRING),                                                  #OK
        
            #The number of vertices affected by this bone
            ("mNumWeights", c_uint),                                            #OK
        
            #The vertices affected by this bone
            ("mWeights", POINTER(VERTEXWEIGHT)),                                #OK
        
            #Matrix that transforms from mesh space to bone space in bind pose
            ("mOffsetMatrix", MATRIX4X4)                                        #OK
        ]


class MESH(Structure):
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
    
    #Maximum number of texture coord sets (UV(W) channels) per mesh.
    #See "AI_MAX_NUMBER_OF_TEXTURECOORDS" in "aiMesh.h" for further
    #information.
    AI_MAX_NUMBER_OF_TEXTURECOORDS = 4
    
    
    #Maximum number of vertex color sets per mesh.
    #Normally: Diffuse, specular, ambient and emissive
    #However one could use the vertex color sets for any other purpose, too.
    AI_MAX_NUMBER_OF_COLOR_SETS = 4
    
    
    _fields_ = [
            #The number of vertices in this mesh. 
            #This is also the size of all of the per-vertex data arrays
            ("mNumVertices", c_uint),                                           #OK
        
            #The number of primitives (triangles, polygones, lines) in this mesh. 
            #This is also the size of the mFaces array 
            ("mNumFaces", c_uint),                                              #OK
        
            #Vertex positions. 
            #This array is always present in a mesh. The array is 
            #mNumVertices in size. 
            ("mVertices", POINTER(VECTOR3D)),                                   #OK
        
            #Vertex normals. 
            #The array contains normalized vectors, NULL if not present. 
            #The array is mNumVertices in size. 
            ("mNormals", POINTER(VECTOR3D)),                                    #OK
        
            #Vertex tangents. 
            #The tangent of a vertex points in the direction of the positive 
            #X texture axis. The array contains normalized vectors, NULL if
            #not present. The array is mNumVertices in size. 
            #@note If the mesh contains tangents, it automatically also 
            #contains bitangents. 
            ("mTangents", POINTER(VECTOR3D)),                                   #OK
        
            #Vertex bitangents. 
            #The bitangent of a vertex points in the direction of the positive 
            #Y texture axis. The array contains normalized vectors, NULL if not
            #present. The array is mNumVertices in size. 
            #@note If the mesh contains tangents, it automatically also contains
            #bitangents. 
            ("mBitangents", POINTER(VECTOR3D)),                                 #OK
        
            #Vertex color sets. 
            #A mesh may contain 0 to #AI_MAX_NUMBER_OF_COLOR_SETS vertex 
            #colors per vertex. NULL if not present. Each array is
            #mNumVertices in size if present.
            ("mColors", POINTER(COLOR4D)*AI_MAX_NUMBER_OF_COLOR_SETS),          #OK
        
            #Vertex texture coords, also known as UV channels.
            #A mesh may contain 0 to AI_MAX_NUMBER_OF_TEXTURECOORDS per
            #vertex. NULL if not present. The array is mNumVertices in size. 
            ("mTextureCoords", POINTER(VECTOR3D)*AI_MAX_NUMBER_OF_TEXTURECOORDS),#OK
        
            #Specifies the number of components for a given UV channel.
            #Up to three channels are supported (UVW, for accessing volume
            #or cube maps). If the value is 2 for a given channel n, the
            #component p.z of mTextureCoords[n][p] is set to 0.0f.
            #If the value is 1 for a given channel, p.y is set to 0.0f, too.
            #@note 4D coords are not supported
            ("mNumUVComponents", c_uint*AI_MAX_NUMBER_OF_TEXTURECOORDS),        #OK
        
            #The faces the mesh is contstructed from. 
            #Each face referres to a number of vertices by their indices. 
            #This array is always present in a mesh, its size is given 
            #in mNumFaces.
            ("mFaces", POINTER(FACE)),                                          #OK
        
            #The number of bones this mesh contains. 
            #Can be 0, in which case the mBones array is NULL. 
            ("mNumBones", c_uint),                                              #OK
        
            #The bones of this mesh. 
            #A bone consists of a name by which it can be found in the
            #frame hierarchy and a set of vertex weights.
            ("mBones", POINTER(POINTER(BONE))),                                 #OK
        
            #The material used by this mesh. 
            #A mesh does use only a single material. If an imported model uses
            #multiple materials, the import splits up the mesh. Use this value 
            #as index into the scene's material list.
            ("mMaterialIndex", c_uint)                                          #OK
        ]


class MATERIALPROPERTY(Structure):
    """
    Data structure for a single property inside a material.
    """
    
    _fields_ = [
            #Specifies the name of the property (key)
            #Keys are case insensitive.
            ("mKey", STRING),                                                   #OK
        
            #Size of the buffer mData is pointing to, in bytes
            #This value may not be 0.
            ("mDataLength", c_uint),                                            #OK
        
            #Type information for the property.
            #Defines the data layout inside the
            #data buffer. This is used by the library
            #internally to perform debug checks.
            #THIS IS AN ENUM!
            ("mType", c_int),                                                   #IGNORED
        
            #Binary buffer to hold the property's value
            #The buffer has no terminal character. However,
            #if a string is stored inside it may use 0 as terminal,
            #but it would be contained in mDataLength. This member
            #is never 0
            ("mData", c_char_p)                                                 #OK
        ]


class MATERIAL(Structure):
    """
    Data structure for a material.
    
    Material data is stored using a key-value structure, called property
    (to guarant that the system is maximally flexible).
    The library defines a set of standard keys (AI_MATKEY) which should be 
    enough for nearly all purposes.
    """
    
    _fields_ = [
            #List of all material properties loaded.
            ("mProperties", POINTER(POINTER(MATERIALPROPERTY))),                #OK
        
            #Number of properties loaded
            ("mNumProperties", c_uint),                                         #OK
            ("mNumAllocated", c_uint)                                           #IGNORED
        ]
    

class VECTORKEY(Structure):
    """
    A time-value pair specifying a certain 3D vector for the given time.
    """
    
    _fields_ = [
            #The time of this key
            ("mTime", c_double),
            
            #The value of this key
            ("mValue", VECTOR3D)
        ]


class QUATERNION(Structure):
    """
    Represents a quaternion in a 4D vector.
    """
    
    _fields = [
               #the components
               ("w", c_float),
               ("x", c_float),
               ("y", c_float),
               ("z", c_float)
           ]


class QUATKEY(Structure):
    """
    A time-value pair specifying a rotation for the given time. For joint
    animations the rotation is usually expressed using a quaternion.
    """
    
    _fields_ = [
            #The time of this key
            ("mTime", c_double),
            
            #The value of this key
            ("mValue", QUATERNION)
        ]


class BONEANIM(Structure):
    """
    Describes the animation of a single node. The name specifies the bone/node
    which is affected by this animation channel. The keyframes are given in
    three separate series of values, one each for position, rotation and
    scaling.
    
    NOTE: The name "BoneAnim" is misleading. This structure is also used to
    describe the animation of regular nodes on the node graph. They needn't be
    nodes.
    """
    
    _fields_ = [
            #The name of the bone affected by this animation.
            ("mBoneName", STRING),
        
            #The number of position keys
            ("mNumPositionKeys", c_uint),
            
            #The position keys of this animation channel. Positions are
            #specified as 3D vector. The array is mNumPositionKeys in size.
            ("mPositionKeys", POINTER(VECTORKEY)),
        
            #The number of rotation keys
            ("mNumRotationKeys", c_uint),
            
            #The rotation keys of this animation channel. Rotations are given as
            #quaternions, which are 4D vectors. The array is mNumRotationKeys in
            #size.
            ("mRotationKeys", POINTER(QUATKEY)),
        
            #The number of scaling keys
            ("mNumScalingKeys", c_uint),
            
            #The scaling keys of this animation channel. Scalings are specified
            #as 3D vector. The array is mNumScalingKeys in size.
            ("mScalingKeys", POINTER(VECTORKEY))
        ]


class ANIMATION(Structure):
    """
    An animation consists of keyframe data for a number of bones.
    For each bone affected by the animation a separate series of data is given.
    """
    
    _fields_ = [
            #The name of the animation. If the modelling package this data was
            #exported from does support only a single animation channel, this
            #name is usually empty (length is zero).
            ("mName", STRING),
        
            #Duration of the animation in ticks.
            ("mDuration", c_double),
            
            #Ticks per second. 0 if not specified in the imported file
            ("mTicksPerSecond", c_double),
        
            #The number of bone animation channels. Each channel affects a
            #single bone.
            ("mNumBones", c_uint),
            
            #The bone animation channels. Each channel affects a single bone.
            #The array is mNumBones in size.
            ("mBones", POINTER(POINTER(BONEANIM)))
        ]


class TEXEL(Structure):
    """
    Helper structure to represent a texel in ARGB8888 format
    """
    
    _fields_ = [
            ("b", c_ubyte),
            ("g", c_ubyte),
            ("r", c_ubyte),
            ("a", c_ubyte)
        ]


class TEXTURE(Structure):
    """
    Helper structure to describe an embedded texture
    
    Normally textures are contained in external files but some file formats
    do embedd them.
    """
    
    _fields_ = [
            #Width of the texture, in pixels
            #If mHeight is zero the texture is compressed in a format
            #like JPEG. In this case mWidth specifies the size of the
            #memory area pcData is pointing to, in bytes.
            ("mWidth", c_uint),                                                 #OK
        
            #Height of the texture, in pixels
            #If this value is zero, pcData points to an compressed texture
            #in an unknown format (e.g. JPEG).
            ("mHeight", c_uint),                                                #OK
        
            #A hint from the loader to make it easier for applications
            #to determine the type of embedded compressed textures.
            #If mHeight != 0 this member is undefined. Otherwise it
            #will be set to '\0\0\0\0' if the loader has no additional
            #information about the texture file format used OR the
            #file extension of the format without a leading dot.
            #E.g. 'dds\0', 'pcx\0'.  All characters are lower-case.
            ("achFormatHint", c_char*4),                                        #OK
        
            #Data of the texture.
            #Points to an array of mWidth * mHeight aiTexel's.
            #The format of the texture data is always ARGB8888 to
            #make the implementation for user of the library as easy
            #as possible. If mHeight = 0 this is a pointer to a memory
            #buffer of size mWidth containing the compressed texture
            #data. Good luck, have fun!
            ("pcData", POINTER(TEXEL))                                          #OK
        ]


class SCENE(Structure):
    """
    The root structure of the imported data.
    Everything that was imported from the given file can be accessed from here.
    """
    
    _fields_ = [
            #Any combination of the AI_SCENE_FLAGS_XXX flags
            ("flags", c_uint),                                                  #OK
            
            #The root node of the hierarchy.
            #There will always be at least the root node if the import
            #was successful. Presence of further nodes depends on the 
            #format and content of the imported file.
            ("mRootNode", POINTER(NODE)),
            
            #The number of meshes in the scene.
            ("mNumMeshes", c_uint),                                             #OK
            
            #The array of meshes.
            #Use the indices given in the aiNode structure to access 
            #this array. The array is mNumMeshes in size.
            ("mMeshes", POINTER(POINTER(MESH))),                                #OK
            
            #The number of materials in the scene.
            ("mNumMaterials", c_uint),                                          #OK
            
            #The array of materials.
            #Use the index given in each aiMesh structure to access this
            #array. The array is mNumMaterials in size.
            ("mMaterials", POINTER(POINTER(MATERIAL))),                         #OK
            
            #The number of animations in the scene.
            ("mNumAnimations", c_uint),
            
            #The array of animations.
            #All animations imported from the given file are listed here.
            #The array is mNumAnimations in size.
            ("mAnimations", POINTER(POINTER(ANIMATION))),
            
            #The number of textures embedded into the file
            ("mNumTextures", c_uint),                                           #OK
            
            #The array of embedded textures.
            #Not many file formats embedd their textures into the file.
            #An example is Quake's MDL format (which is also used by
            #some GameStudio(TM) versions)
            ("mTextures", POINTER(POINTER(TEXTURE)))                            #OK
        ]