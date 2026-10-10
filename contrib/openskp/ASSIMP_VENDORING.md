# OpenSKP (vendored)

This directory is a vendored copy of the C++ package
(`packages/cpp`) from [OpenSKP](https://github.com/iamahsanmehmood/openskp)
(MIT licensed), used by `code/AssetLib/Skp/SkpImporter.*` to read native
SketchUp `.skp` files. See `LICENSE` in this directory for the upstream
license.

Vendored as plain committed source (not a git submodule), matching how
this repository's other `contrib/` dependencies are handled.

**Vendored from:** `iamahsanmehmood/openskp`, `packages/cpp/` subdirectory
of that repository's `main` branch, as of commit `cb57112` (2026-09-22).

**Trimmed to the read path.** Upstream also ships a writer/editor and
exporters to OBJ/STL/PLY/DXF/IFC/GLB/Fragments, which SkpImporter never
calls and which pull in two extra third-party dependencies (tinygltf,
FlatBuffers) this importer has no use for - vendoring those just to leave
them unused would mean two more configure-time fetches for nothing. Only
what `parser.hpp`/`instanced_scene.hpp` need to turn a `.skp` file into an
`InstancedScene` is kept - see `include/openskp/openskp.hpp`'s own comment
for the exact included/excluded header list, and `CMakeLists.txt`'s
`OPENSKP_SOURCES` for the source list. `miniz` stays vendored (via
`FetchContent` in `CMakeLists.txt`) since `core.cpp` uses it to read the
zip-based container modern `.skp` files use.

**To update:** replace `include/`, `src/`, `cmake/`, `CMakeLists.txt`,
`.clang-format`, and `LICENSE` in this directory with the current contents
of the upstream repository's own `packages/cpp/` directory (`.clang-format`
lives one level up, at that repository's root), then re-apply the trim above
(re-delete the writer/editor/exporter/WASM sources and headers, and the
`tinygltf`/`flatbuffers` `FetchContent` blocks + WASM target in
`CMakeLists.txt`, and drop their includes from `openskp.hpp`), then
rebuild with `-DASSIMP_BUILD_SKP_IMPORTER=ON` to confirm nothing broke.
