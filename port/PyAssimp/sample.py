from pyassimp import pyassimp

MODEL = r"../test.ase"

def main():
    scene = pyassimp.load(MODEL)
    
    #write some statistics
    print "SCENE:"
    print "  flags:", ", ".join(scene.list_flags())
    print "  meshes:", len(scene.meshes)
    print ""
    print "MESHES:"
    for index, mesh in enumerate(scene.meshes):
        print "  MESH", index+1
        print "    vertices:", len(mesh.vertices)
        print "    first:", mesh.vertices[:3]
        print "    colors:", len(mesh.colors)
        print "    uv-counts:", mesh.numuv
        print ""
    

if __name__ == "__main__":
    main()