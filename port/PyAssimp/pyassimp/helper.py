#-*- coding: UTF-8 -*-

"""
Some fancy helper functions.
"""

import os
import ctypes
from ctypes import POINTER
import operator
import numpy
from numpy import linalg

import logging;logger = logging.getLogger("pyassimp")

from .errors import AssimpError

additional_dirs, ext_whitelist = [],[]

# populate search directories and lists of allowed file extensions
# depending on the platform we're running on.
if os.name=='posix':
    additional_dirs.append('/usr/lib/')
    additional_dirs.append('/usr/local/lib/')

    # note - this won't catch libassimp.so.N.n, but 
    # currently there's always a symlink called
    # libassimp.so in /usr/local/lib.
    ext_whitelist.append('.so')
    # libassimp.dylib in /usr/local/lib
    ext_whitelist.append('.dylib')

elif os.name=='nt':
    ext_whitelist.append('.dll')
    path_dirs = os.environ['PATH'].split(';')
    for dir_candidate in path_dirs:
        if 'assimp' in dir_candidate.lower():
            additional_dirs.append(dir_candidate)
            
#print(additional_dirs)
def vec2tuple(x):
    """ Converts a VECTOR3D to a Tuple """
    return (x.x, x.y, x.z)

def transform(vector3, matrix4x4):
    """ Apply a transformation matrix on a 3D vector.

    :param vector3: a numpy array with 3 elements
    :param matrix4x4: a numpy 4x4 matrix
    """
    return numpy.dot(matrix4x4, numpy.append(vector3, 1.))

   
def get_bounding_box(scene):
    bb_min = [1e10, 1e10, 1e10] # x,y,z
    bb_max = [-1e10, -1e10, -1e10] # x,y,z
    return get_bounding_box_for_node(scene.rootnode, bb_min, bb_max, linalg.inv(scene.rootnode.transformation))

def get_bounding_box_for_node(node, bb_min, bb_max, transformation):

    transformation = numpy.dot(transformation, node.transformation)
    for mesh in node.meshes:
        for v in mesh.vertices:
            v = transform(v, transformation)
            bb_min[0] = min(bb_min[0], v[0])
            bb_min[1] = min(bb_min[1], v[1])
            bb_min[2] = min(bb_min[2], v[2])
            bb_max[0] = max(bb_max[0], v[0])
            bb_max[1] = max(bb_max[1], v[1])
            bb_max[2] = max(bb_max[2], v[2])


    for child in node.children:
        bb_min, bb_max = get_bounding_box_for_node(child, bb_min, bb_max, transformation)

    return bb_min, bb_max



def try_load_functions(library,dll,candidates):
    """try to  functbind to aiImportFile and aiReleaseImport
    
    library - path to current lib
    dll - ctypes handle to it
    candidates - receives matching candidates

    They serve as signal functions to detect assimp,
    also they're currently the only functions we need.
    insert (library,aiImportFile,aiReleaseImport,dll) 
    into 'candidates' if successful.

    """
    try:
        load = dll.aiImportFile
        release = dll.aiReleaseImport
    except AttributeError:
        #OK, this is a library, but it has not the functions we need
        pass
    else:
        #Library found!
        from .structs import Scene
        load.restype = POINTER(Scene)
        
        candidates.append((library, load, release, dll))


def search_library():
    """Loads the assimp-Library.
    
    result (load-function, release-function)    
    exception AssimpError if no library is found
    
        """
    #this path
    folder = os.path.dirname(__file__)

    # silence 'DLL not found' message boxes on win
    try:
        ctypes.windll.kernel32.SetErrorMode(0x8007)
    except AttributeError:
        pass    

    candidates = []
    
    # test every file
    for curfolder in [folder]+additional_dirs:
        for filename in os.listdir(curfolder):
            # our minimum requirement for candidates is that
            # they should contain 'assimp' somewhere in 
            # their name
            if filename.lower().find('assimp')==-1 or\
                os.path.splitext(filename)[-1].lower() not in ext_whitelist:
                continue

            library = os.path.join(curfolder, filename)
            logger.debug('Try ' + library)
            try:
                dll = ctypes.cdll.LoadLibrary(library)
            except Exception as e:
                logger.warning(str(e))
                # OK, this except is evil. But different OSs will throw different
                # errors. So just ignore any errors.
                continue

            try_load_functions(library,dll,candidates)
    
    if not candidates:
        # no library found
        raise AssimpError("assimp library not found")
    else:
        # get the newest library
        candidates = map(lambda x: (os.lstat(x[0])[-2], x), candidates)
        res = max(candidates, key=operator.itemgetter(0))[1]
        logger.debug('Using assimp library located at ' + res[0])

        # XXX: if there are 1000 dll/so files containing 'assimp'
        # in their name, do we have all of them in our address
        # space now until gc kicks in?

        # XXX: take version postfix of the .so on linux?
        return res[1:]

def hasattr_silent(object, name):
    """
        Calls hasttr() with the given parameters and preserves the legacy (pre-Python 3.2)
        functionality of silently catching exceptions.
        
        Returns the result of hasatter() or False if an exception was raised.
    """
    
    try:
        return hasattr(object, name)
    except:
        return False
