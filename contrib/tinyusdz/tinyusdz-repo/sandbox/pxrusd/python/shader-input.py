from pxr import Usd,UsdShade,Sdf,UsdGeom,Gf,Kind

stage = Usd.Stage.CreateInMemory()
UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)

modelRoot = UsdGeom.Xform.Define(stage, "/TexModel")
Usd.ModelAPI(modelRoot).SetKind(Kind.Tokens.component)
billboard = UsdGeom.Mesh.Define(stage, "/TexModel/card")
billboard.CreatePointsAttr([(0,0,0),(9.6,0,0), (9.6,5.4,0),(0,5.4,0)])
billboard.CreateFaceVertexCountsAttr([4])
billboard.CreateFaceVertexIndicesAttr([0,1,2,3])
billboard.CreateExtentAttr([(0,0, 0), (9.6,5.4, 0)])
texCoords = billboard.CreatePrimvar("st",
                                    Sdf.ValueTypeNames.TexCoord2fArray,
                                    UsdGeom.Tokens.varying)
texCoords.Set([(0, 0), (1, 0), (1,1), (0, 1)])

material = UsdShade.Material.Define(stage, '/TexModel/boardMat')
pbrShader = UsdShade.Shader.Define(stage, '/TexModel/boardMat/PBRShader')
pbrShader.CreateIdAttr("UsdPreviewSurface")
pbrShader.CreateInput("roughness", Sdf.ValueTypeNames.Float).Set(0.4)
pbrShader.CreateInput("metallic", Sdf.ValueTypeNames.Float).Set(0.0)
pbrShader.CreateInput("opacity",Sdf.ValueTypeNames.Float).Set(1.0)
material.CreateSurfaceOutput().ConnectToSource(pbrShader.ConnectableAPI(), "surface")

UsdShade.MaterialBindingAPI(billboard).Bind(material)

stReader = UsdShade.Shader.Define(stage, '/TexModel/boardMat/PBRShader/stReader')
stReader.CreateIdAttr('UsdPrimvarReader_float2')

diffuseTextureSampler = UsdShade.Shader.Define(stage,'/TexModel/boardMat/PBRShader/diffuseTexture')
diffuseTextureSampler.CreateIdAttr('UsdUVTexture')
diffuseTextureSampler.CreateInput('file', Sdf.ValueTypeNames.Asset).Set("D:/test.png")
diffuseTextureSampler.CreateInput("st", Sdf.ValueTypeNames.Float2).ConnectToSource(stReader.ConnectableAPI(), 'result')
diffuseTextureSampler.CreateOutput('rgb', Sdf.ValueTypeNames.Float3)

pbrShader.CreateInput("diffuseColor", Sdf.ValueTypeNames.Color3f).ConnectToSource(diffuseTextureSampler.ConnectableAPI(), 'rgb')
stInput = material.CreateInput('frame:stPrimvarName', Sdf.ValueTypeNames.Token)
stInput.Set('st')

stReader.CreateInput('varname',Sdf.ValueTypeNames.Token).ConnectToSource(stInput)

#stage.Reload()

p = stReader.GetInput('st').HasConnectedSource()

print(p.Get())
ss = p.GetConnectedSource().Get()
print(ss)
attrs = p.GetValueProducingAttributes()
print(attrs)
print("varname = ", attrs[0].Get())

print(stage.ExportToString())
