#-*- coding: UTF-8 -*-

"""
This module demonstrates the functionality of PyAssimp.
"""


from pyassimp import pyassimp
import os

#get a model out of assimp's test-data
MODEL = os.path.join(os.path.dirname(__file__),
                     "..", "..",
                     "test", "models", "MDL", "MDL3 (3DGS A4)", "minigun.MDL")

def main():
    scene = pyassimp.load(MODEL)
    
    #the model we load
    print "MODEL:", MODEL
    print
    
    #write some statistics
    print "SCENE:"
    print "  meshes:", len(scene.meshes)
    print "  materials:", len(scene.materials)
    print "  textures:", len(scene.textures)
    print
    
    print "MESHES:"
    for index, mesh in enumerate(scene.meshes):
        print "  MESH", index+1
        print "    material:", mesh.mMaterialIndex+1
        print "    vertices:", len(mesh.vertices)
        print "    first:", mesh.vertices[:3]
        print "    colors:", len(mesh.colors)
        tc = mesh.texcoords
        print "    texture-coords 1:", len(tc[0]), "first:", tc[0][:3]
        print "    texture-coords 2:", len(tc[1]), "first:", tc[1][:3]
        print "    texture-coords 3:", len(tc[2]), "first:", tc[2][:3]
        print "    texture-coords 4:", len(tc[3]), "first:", tc[3][:3]
        print "    uv-component-count:", len(mesh.mNumUVComponents)
        print "    faces:", len(mesh.faces), "first:", [f.indices for f in mesh.faces[:3]]
        print "    bones:", len(mesh.bones), "first:", [b.mName for b in mesh.bones[:3]]
        print

    print "MATERIALS:"
    for index, material in enumerate(scene.materials):
        print "  MATERIAL", index+1
        properties = pyassimp.GetMaterialProperties(material)
        for key in properties:
            print "    %s: %s" % (key, properties[key])
    print
    
    print "TEXTURES:"
    for index, texture in enumerate(scene.textures):
        print "  TEXTURE", index+1
        print "    width:", texture.mWidth
        print "    height:", texture.mHeight
        print "    hint:", texture.achFormatHint
        print "    data (size):", texture.mWidth*texture.mHeight
   
    # Finally release the model
    pyassimp.release(scene)

if __name__ == "__main__":
    main()