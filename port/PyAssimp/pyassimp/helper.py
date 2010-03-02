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

additional_dirs, ext_whitelist = [],[]

# populate search directories and lists of allowed file extensions
# depending on the platform we're running on.
if os.name=='posix':
    additional_dirs.append('/usr/local/lib/')

    # note - this won't catch libassimp.so.N.n, but 
    # currently there's always a symlink called
    # libassimp.so in /usr/local/lib.
    ext_whitelist.append('.so')

elif os.name=='nt':
    ext_whitelist.append('.dll')

print(additional_dirs)
def vec2tuple(x):
    """ Converts a VECTOR3D to a Tuple """
    return (x.x, x.y, x.z)


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
        load.restype = POINTER(structs.Scene)
        
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
            print 'Try ',library
            try:
                dll = ctypes.cdll.LoadLibrary(library)
            except:
                # OK, this except is evil. But different OSs will throw different
                # errors. So just ignore any errors.
                continue

            try_load_functions(library,dll,candidates)
    
    if not candidates:
        # no library found
        raise AssimpError, "assimp library not found"
    else:
        # get the newest library
        candidates = map(lambda x: (os.lstat(x[0])[-2], x), candidates)
        res = max(candidates, key=operator.itemgetter(0))[1]
        print 'Taking ',res[0]

        # XXX: if there are 1000 dll/so files containing 'assimp'
        # in their name, do we have all of them in our address
        # space now until gc kicks in?

        # XXX: take version postfix of the .so on linux?
        return res[1:]



