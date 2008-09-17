#-*- coding: UTF-8 -*-

"""
This module demonstrates the functionality of PyAssimp.
"""


from pyassimp import pyassimp
import os

#get a model out of assimp's test-data
MODEL = os.path.join(os.path.dirname(__file__),
                     "..", "..",
                     "test", "ASEFiles", "MotionCaptureROM.ase")

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
        print "    uv-counts:", mesh.numuv
        print
    

if __name__ == "__main__":
    main()