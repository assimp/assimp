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

**To update:** replace `include/`, `src/`, `CMakeLists.txt`, and `LICENSE`
in this directory with the current contents of the upstream repository's
own `packages/cpp/` directory, then rebuild with
`-DASSIMP_BUILD_SKP_IMPORTER=ON` to confirm nothing broke.
