# PreviewSurface

https://graphics.pixar.com/usd/docs/UsdPreviewSurface-Proposal.html 

https://github.com/PixarAnimationStudios/USD/blob/release/pxr/usdImaging/plugin/usdShaders/shaders/shaderDefs.usda

## Inputs

* diffuseColor - color3f - (0.18, 0.18, 0.18)
* emissiveColor - color3f - (0.0, 0.0, 0.0)
* useSpeclarWorkflow - int - 0
  * 1
    * specularColor - color3f - (0.0, 0.0, 0.0)
  * 0
    * metallic - float - 0.0
* roughness - float - 0.5
* clearcoat - float - 0.0
* clearcoatRoughness - float 0.01
* opacity - float - 1.0
* opacityThreshold - float - 0.0
* ior - float - 1.5
* normal - normal3f - (0.0, 0.0, 1.0)
* displacement - float - 0.0
* occlusion - float - 1.0

## Outputs

* surface - token
* displacement - token

## UsdUVTexture

### Inputs

* file - asset - string
* st - float2 - (0.0, 0.0)
* wrapS - token - useMetadata
  * black, clamp, repeat, mirror, 
* wrapT - token = useMetadata
* fallback - float4 - (0.0, 0.0, 0.0, 1.0)
* scale - float4 - (1.0, 1.0, 1.0, 1.0)
* bias - float4 - (0.0, 0.0, 0.0, 0.0)

### Outputs

* r, g, b, a


## UsdTransform2d

### Inputs

* in - float2 - (0.0, 0.0)
* rotation - float - (0.0)
  * counter-clockwise rotation in degrees around the origin
* scale - float2 - (1.0, 1.0)
* translation - float2 (0.0, 0.0)

### Outpuits

* result - float2


## TODO

* [ ] Primvar Reader(arbitrary vertex attributes)
* [ ] displacement
* [ ] Texture transform

