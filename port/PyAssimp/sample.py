#-*- coding: UTF-8 -*-

"""
This module demonstrates the functionality of PyAssimp.
"""


from pyassimp import pyassimp
import os

#get a model out of assimp's test-data
MODEL = os.path.join(os.path.dirname(__file__),
                     "..", "..",
                     "test", "3DSFiles", "test1.3ds")

def main():
    scene = pyassimp.load(MODEL)
    
    #the model we load
    print "MODEL:", MODEL
    print
    
    #write some statistics
    print "SCENE:"
    print "  flags:", ", ".join(scene.list_flags())
    print "  meshes:", len(scene.meshes)
    print
    print "MESHES:"
    for index, mesh in enumerate(scene.meshes):
        print "  MESH", index+1
        print "    vertices:", len(mesh.vertices)
        print "    first:", mesh.vertices[:3]
        print "    colors:", len(mesh.colors)
        tc = mesh.texcoords
        print "    texture-coords 1:", len(tc[0]), "first:", tc[0][:3]
        print "    texture-coords 2:", len(tc[1]), "first:", tc[1][:3]
        print "    texture-coords 3:", len(tc[2]), "first:", tc[2][:3]
        print "    texture-coords 4:", len(tc[3]), "first:", tc[3][:3]
        print "    uv-counts:", mesh.uvsize
        print
    

if __name__ == "__main__":
    main()