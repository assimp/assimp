#!/usr/bin/env python
#-*- coding: UTF-8 -*-

"""
This module uses the sample.py script to load all test models it finds.

Note: this is not an exhaustive test suite, it does not check the
data structures in detail. It just verifies whether basic
loading and querying of 3d models using pyassimp works.
"""


import sys,os
import sample
from pyassimp import errors

# paths to be walkd recursively
basepaths = [os.path.join('..','..','..','test','models'), os.path.join('..','..','..','test','models-nonbsd')]

# file extensions to be considered
extensions = ['.3ds','.x','.lwo','.obj','.md5mesh','.dxf','.ply','.stl','.dae','.md5anim','.lws','.irrmesh','.nff','.off','.blend']

def run_tests():
    ok,err = 0,0
    for path in basepaths:
        print("Looking for models in %s..." % path)
        for root, dirs, files in os.walk(path):
            for afile in files:
                base,ext = os.path.splitext(afile)
                if ext in extensions:
                    try:
                        sample.main(os.path.join(root,afile))
                        ok += 1
                    except errors.AssimpError as error:
                        # assimp error is fine, this is a controlled case
                        print error
                        err += 1
                    except Exception:
                        print("Error encountered while loading <%s>"%os.path.join(root,afile))
    print('** Loaded %s models, got controlled errors for %s files' % (ok,err))


if __name__ == '__main__':
	run_tests()




