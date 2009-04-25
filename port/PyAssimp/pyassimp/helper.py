#-*- coding: UTF-8 -*-

"""
Some fancy helper functions.
"""

import os
import ctypes
import structs
import operator
from errors import AssimpError
from ctypes import POINTER


def vec2tuple(x):
    """
    Converts a VECTOR3D to a Tuple
    """
    return (x.x, x.y, x.z)


def search_library():
    """
    Loads the assimp-Library.
    
    result (load-function, release-function)
    
    exception AssimpError if no library is found
    """
    #this path
    folder = os.path.dirname(__file__)
    
    ctypes.windll.kernel32.SetErrorMode(0x8007)
    
    candidates = []
    
    #test every file
    for filename in os.listdir(folder):
        library = os.path.join(folder, filename)
        
        try:
            dll = ctypes.cdll.LoadLibrary(library)
        except:
            #OK, this except is evil. But different OSs will throw different
            #errors. So just ignore errors.
            pass
        else:
            #get the functions
            try:
                load = dll.aiImportFile
                release = dll.aiReleaseImport
            except AttributeError:
                #OK, this is a library, but it has not the functions we need
                pass
            else:
                #Library found!
                load.restype = POINTER(structs.Scene)
                
                candidates.append((library, load, release, dll))
    
    if not candidates:
        #no library found
        raise AssimpError, "assimp library not found"
    else:
        #get the newest library
        candidates = map(lambda x: (os.lstat(x[0])[-2], x), candidates)
        return max(candidates, key=operator.itemgetter(0))[1][1:]
