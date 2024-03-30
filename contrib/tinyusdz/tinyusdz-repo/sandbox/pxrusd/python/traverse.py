import os
import sys

from pxr import Usd,Sdf

def main():

    input_filename = None
    if len(sys.argv) > 1:
        input_filename = sys.argv[1]

    if input_filename is None:
        print("Need input.usd")
        sys.exit(-1)

    layer = Sdf.Layer.FindOrOpen(input_filename)
    stage = Usd.Stage.Open(layer)
    print(stage)
    #stage.GetPrimChildren()

    for prim in stage.Traverse():
        print(prim)
        print(prim.GetPropertyNames())


if __name__ == "__main__":
    main()
