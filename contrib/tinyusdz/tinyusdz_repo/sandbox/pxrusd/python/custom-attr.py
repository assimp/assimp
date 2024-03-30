from pxr import Usd, Sdf
 
stage = Usd.Stage.CreateInMemory()
prim = stage.DefinePrim('/muda')
attr = prim.CreateAttribute('spin', Sdf.ValueTypeNames.Float)

attr.Set(0.0, 0.0)
attr.Set(192, 1440.0)

print(stage.ExportToString())
