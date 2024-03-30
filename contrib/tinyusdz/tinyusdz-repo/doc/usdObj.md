# usdObj in TinyUSDZ

TinyUSDZ has built-in support of usdObj(wavefront .obj import).
Why built-in?

* It is not recommended to use plug-in architecture(e.g. separated dll) for mobile/wasm devices.
* People frequently want .obj support

.obj support is enabled by default. You can disable usdObj support by cmake flags(`TINYUSDZ_USE_USDOBJ=Off`)

## Data structure

* Shape(group/object) hierarchy is flattened to single mesh and no material.
* Texcoords and normals are decoded as face-varying attribute.

## Limitations

* No material info is parsed
* No per-object and per-face material

## TODO

* [ ] Preserve shape hierarchy
* [ ] per-face material
  * Decode to GeomSubset?
* [ ] Loading Skin weight(tinyobjloader's extension)
