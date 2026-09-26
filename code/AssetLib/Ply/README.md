# PLY AssetLib

Mesh PLY importer (`PlyLoader` / `PlyParser`).

Reference notes distinguishing **classic mesh PLY** (`element face`,
`vertex_indices`) from **3D Gaussian Splatting** custom vertex properties
(`f_dc_*`, `f_rest_*`, `opacity`, `scale_*`, `rot_*`):

→ [`doc/PLY.md`](../../../doc/PLY.md)

### Runtime API (ABI-safe)

```cpp
#include <assimp/gaussian.h>
const aiGaussianSplat *gs = aiGetGaussianSplat(scene, meshIndex);
```

Positions remain on `aiMesh::mVertices`. Splat arrays are **not** fields of
`aiMesh` (no public-struct ABI break).

**No material baking:** the PLY loader does **not** invent `$clr.diffuse` /
`$clr.specular` / opacity / PBR factors from `f_dc_*` / `opacity` / etc.
Those stay only on `aiGaussianSplat` (raw file semantics). Decode and shade
in the renderer — see `doc/PLY.md` (“Materials (intentional non-goal)”).
