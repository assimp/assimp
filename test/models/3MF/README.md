# 3MF test models

Source and provenance of the binary `.3mf` fixtures in this directory. Each file is an
OPC (ZIP) container; inspect with `unzip -l <file>.3mf`.

## box.3mf
Minimal core 3MF: a single box mesh (8 vertices, 12 faces). Pre-existing fixture used by
the basic import/round-trip test (`import3MFFromFileTest`).

## malformed_property_index_oob.3mf
Hand-crafted malformed file whose property/vertex indices point out of bounds. Regression
fixture ensuring the importer bounds-checks them instead of crashing
(`importMalformedPropertyIndexOob`, added with assimp PR #6709).

## cube_gears_prod_req.3mf
Official **3MF Production Extension** sample from the 3MF Consortium. The build `<item>`
elements reference objects in separate `.model` parts via the `p:path` attribute, and every
referenced part reuses `objectid="1"` — exercising per-part resource scoping.

- Source: https://github.com/3MFConsortium/3mf-samples — `examples/production/cube_gears_prod_req.3mf`
- License: BSD-2-Clause (3mf-samples repository)
- Embedded metadata: `Copyright (c) 2018 3MF Consortium. All rights reserved.`
- Used by `importProductionBuildItemPaths` (expects 17 meshes / 17 nodes).

The sibling `cube_gears_prod.3mf` from the same upstream directory is the non-required
(`requiredextensions` omitted) variant and is not committed here.

## production_components.3mf
Synthetic fixture hand-authored for assimp (not from an external source). It exercises the
Production Extension **component** path: the root object's `<components>` reference objects in
two separate parts (`partA.model`, `partB.model`) via `p:path`, and both parts deliberately
reuse `objectid="1"` so the test proves cross-part id scoping yields two distinct meshes
rather than collapsing into one.

- Used by `importProductionComponentPaths` (expects 2 meshes / 1 node).
