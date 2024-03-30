from tinyusdz import Usd, UsdGeom, Sdf

stage = Usd.Stage.CreateNew('hello.usda')
UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)

xformPrim = UsdGeom.Xform.Define(stage, '/hello')
attr = xformPrim.CreateAttribute("test", Sdf.ValueTypeNames.Int)

