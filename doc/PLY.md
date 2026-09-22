# PLY format references — mesh vs. Gaussian Splatting

Branch notes for `(improve)-PLY-importer`: what is *actually* in the classic PLY
specification vs. what is only a *convention* or a *later application layout*
that reuses the PLY container.

## Short answers

| Name | Classic PLY standard? | Notes |
|---|---|---|
| `element face` + `property list … vertex_index(ices)` | **Yes (conventional core)** | Documented as the usual mesh description together with `element vertex`. Not a reserved keyword of a closed schema — PLY allows arbitrary element names — but **face + vertex indices** is the canonical mesh pattern in Turk’s format write-ups. |
| `property float f_dc_*` / `f_rest_*` | **No** | Not in Greg Turk / Stanford PLY docs. Valid as *user-defined scalar vertex properties* (PLY allows any property name). Semantic meaning comes from **3D Gaussian Splatting** (INRIA graphdeco), not from PLY 1.0. |
| `opacity`, `scale_*`, `rot_*` (splat) | **No** | Same: custom vertex properties for 3DGS PLY interchange. |

So: **faces are PLY mesh practice; `f_dc_*` / `f_rest_*` are 3DGS-on-PLY, not “PLY standard fields”.**

---

## 1. Classic PLY (mesh / polygon objects)

### What the format standardizes

PLY (“Polygon File Format” / Stanford Triangle Format) describes **one object** as
lists of **elements**, each with a fixed set of **properties**. The header is
ASCII; payload may be ASCII or binary.

Core ideas (Turk):

- Magic: first line `ply`
- `format ascii|binary_little_endian|binary_big_endian 1.0`
- `element <name> <count>` then one or more `property …`
- `end_header`, then payload in header order
- Properties may be scalars (`float`, `uchar`, …) or **lists**:
  `property list <count-type> <index-type> <name>`

### Faces

A typical mesh file defines:

```text
element vertex N
property float x
property float y
property float z
element face M
property list uchar int vertex_indices
```

- **`element face`**: conventional name for polygons (also `vertex`, sometimes `edge`, …).
- **`property list … vertex_index` / `vertex_indices`**: variable-length index list into the vertex element (triangle fan interpretation is called out in the classic docs).

PLY does **not** hard-code a closed enum of element names. New elements/properties
are allowed; unknown ones should be ignorable by older readers. Mesh tools still
treat **vertex + face** as the interchange core Assimp’s PLY importer targets
(see `test/models/PLY/Wuson.ply`: vertices, normals, UVs, **faces**).

### Primary references (classic PLY)

1. **Paul Bourke — “PLY - Polygon File Format”** (widely cited summary of Turk’s format)  
   http://paulbourke.net/dataformats/ply/

2. **Wikipedia overview** (links / history)  
   https://en.wikipedia.org/wiki/PLY_(file_format)

3. Original design context: Greg Turk / Stanford Graphics (1990s) — PLY as a
   simple polygon-object exchange format (vertices, faces, optional properties).

Assimp’s own short list entry: [`doc/Fileformats.md`](Fileformats.md) → PLY.

---

## 2. 3D Gaussian Splatting PLY (`f_dc_*`, `f_rest_*`, …)

### Relation to PLY

3DGS exporters write a **PLY file that usually has only `element vertex`**
(no faces). Each “vertex” is one Gaussian. Extra `property float …` names carry
training parameters. That is **legal PLY syntax** (custom properties), but the
property set is an **application convention**, not part of the 1990s PLY write-up.

### Typical property set (INRIA / graphdeco reference layout)

Common layout (degree-3 SH; exact counts depend on SH degree):

| Properties | Role |
|---|---|
| `x y z` | Splat center |
| `nx ny nz` | Often present in exports (normals); not required for all pipelines |
| `f_dc_0..2` | SH **DC** (degree 0) colour coeffs (RGB) |
| `f_rest_0..M` | Higher-order SH coeffs (channel-major in the reference impl.) |
| `opacity` | Opacity **logit** (pre-sigmoid) |
| `scale_0..2` | **Log** scales |
| `rot_0..3` | Rotation quaternion (commonly wxyz; may need normalize) |

For SH degree 3, `f_rest` is often **45** floats (`f_rest_0` … `f_rest_44`) =
\(3 \times ((3+1)^2 - 1)\).

Reference loader (property names):  
https://github.com/graphdeco-inria/gaussian-splatting/blob/main/scene/gaussian_model.py  
(`load_ply` / `save_ply` — reads `f_dc_*`, `f_rest_*`, `opacity`, `scale_*`, `rot*`).

Paper: Kerbl et al., *3D Gaussian Splatting for Real-Time Radiance Field Rendering*
(SIGGRAPH 2023) — https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/

### Secondary write-ups of the 3DGS PLY convention

- PlayCanvas — “The PLY Format” (Gaussian Splatting):  
  https://developer.playcanvas.com/user-manual/gaussian-splatting/formats/ply/
- splatreg — “PLY interop” (documents graphdeco layout):  
  https://archerkattri.github.io/splatreg/ply-interop/

These describe **ecosystem practice**, not an ISO/Khronos “PLY 2.0” standard.

---

## 3. Implications for Assimp

| File kind | What Assimp PLY does today | Improve-importer direction |
|---|---|---|
| Mesh PLY (`vertex` + `face`) | Supported when `ASSIMP_BUILD_NO_PLY_IMPORTER` is off | Keep / harden mesh path (`Wuson.ply`, etc.) |
| Point-only mesh PLY (vertices, no faces) | Partial / point-cloud style paths in loader | Clarify point clouds |
| 3DGS PLY (`f_dc_*` / `f_rest_*`, no faces) | **Not** a Gaussian renderer; at best unknown extras on vertices | Optional: detect splat property set, expose as custom vertex attrs or dedicated metadata — **do not** pretend they are `aiFace` meshes |

**Assimp is a mesh/scene importer.** Shipping `test/models/PLY/*.ply` proves
**mesh PLY** support. It does **not** mean Assimp implements 3D Gaussian
Splatting. Detecting `f_dc_*`/`f_rest_*` is an *interop/detection* problem on top
of the classic format.

---

## 4. Quick header comparison

**Mesh (classic):**

```text
ply
format ascii 1.0
element vertex …
property float x
property float y
property float z
element face …
property list uchar int vertex_indices
end_header
```

**3DGS (convention on PLY container):**

```text
ply
format binary_little_endian 1.0
element vertex …
property float x
property float y
property float z
property float f_dc_0
property float f_dc_1
property float f_dc_2
property float f_rest_0
…
property float opacity
property float scale_0
…
property float rot_0
…
end_header
# typically NO element face
```
