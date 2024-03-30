import ctinyusdz

t = ctinyusdz.PyTest()
print("----")
#t.intv = [1]
print(t.intv)
print(len(t.intv))
t.intv.append(2)
print(t.intv)

prim = ctinyusdz.Prim()
#t.intv = [prim]


#a = ctinyusdz.PrimVector()
#prim.primChildren = a
#[prim]
#print(prim.children)

#print(len(prim.primChildren))

