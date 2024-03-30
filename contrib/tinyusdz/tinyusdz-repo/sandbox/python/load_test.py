import ctinyusdz

pvar = ctinyusdz.PrimVar()
pvar.set(1.0)
print(pvar.dtype)
pvar.set(2)
print(pvar.dtype)

def emit_indent(i):
    s = ""
    for k in range(i):
        s += "  "

    return s

def prim_traverse(prim, depth: int = 0):
    print(emit_indent(depth) + "<Prim>", prim)

    for child in prim.children():
        prim_traverse(child, depth+1)

xform_prim = ctinyusdz.Prim("xform")
print(xform_prim)

stage = ctinyusdz.load_usd("../../models/texture-cat-plane.usda")

print(stage)
print(stage.metas)
print(stage.metas())
print(stage.metas().metersPerUnit)
stage.metas().metersPerUnit = 3.0
# FIXME. metersPerUnit does not update correctly.
print(stage.metas())
print(stage.metas().metersPerUnit)

prim = stage.GetPrimAtPath("/root")
print(prim.prim_id)

prim = stage.find_prim_by_prim_id(prim.prim_id)
print(prim)

for prim in stage.root_prims():
    print("RootPrim: ", prim)
    prim_traverse(prim)

print(stage.ExportToString())

