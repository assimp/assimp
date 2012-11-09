#!/usr/bin/env python
#-*- coding: UTF-8 -*-

"""
This module demonstrates the functionality of PyAssimp.
"""


import pyassimp
import os, sys

#get a model out of assimp's test-data if none is provided on the command line
DEFAULT_MODEL = os.path.join(os.path.dirname(__file__),
                     "..", "..",
                     "test", "models", "MDL", "MDL3 (3DGS A4)", "minigun.MDL")

def recur_node(node,level = 0):
    print("  " + "\t" * level + "- " + str(node))
    for child in node.children:
        recur_node(child, level + 1)


def main(filename=None):
    filename = filename or DEFAULT_MODEL
    scene = pyassimp.load(filename)
    
    #the model we load
    print "MODEL:", filename
    print
    
    #write some statistics
    print "SCENE:"
    print "  meshes:", len(scene.meshes)
    print "  materials:", len(scene.materials)
    print "  textures:", len(scene.textures)
    print
    
    print "NODES:"
    recur_node(scene.rootnode)

    print
    print "MESHES:"
    for index, mesh in enumerate(scene.meshes):
        print "  MESH", index+1
        print "    material id:", mesh.materialindex+1
        print "    vertices:", len(mesh.vertices)
        print "    first 3 verts:", mesh.vertices[:3]
        if mesh.normals:
                print "    first 3 normals:", mesh.normals[:3]
        else:
                print "    no normals"
        print "    colors:", len(mesh.colors)
        tc = mesh.texturecoords
        if tc:
            print "    texture-coords 1:", len(tc[0]), "first3:", tc[0][:3]
            print "    texture-coords 2:", len(tc[1]), "first3:", tc[1][:3]
            print "    texture-coords 3:", len(tc[2]), "first3:", tc[2][:3]
            print "    texture-coords 4:", len(tc[3]), "first3:", tc[3][:3]
        else:
            print "    no texture coordinates"
        print "    uv-component-count:", len(mesh.numuvcomponents)
        print "    faces:", len(mesh.faces), "first:", [f.indices for f in mesh.faces[:3]]
        print "    bones:", len(mesh.bones), "first:", [str(b) for b in mesh.bones[:3]]
        print

    print "MATERIALS:"
    for index, material in enumerate(scene.materials):
        print("  MATERIAL (id:" + str(index+1) + ")")
        for key, value in material.properties.items():
            print "    %s: %s" % (key, value)
    print
    
    print "TEXTURES:"
    for index, texture in enumerate(scene.textures):
        print "  TEXTURE", index+1
        print "    width:", texture.width
        print "    height:", texture.height
        print "    hint:", texture.achformathint
        print "    data (size):", len(texture.data)
   
    # Finally release the model
    pyassimp.release(scene)

if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv)>1 else None)
