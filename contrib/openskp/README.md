# OpenSKP (assimp vendored subset)

**The open-source SketchUp (`.skp`) file parser — C++17 edition.**

This is a trimmed copy of OpenSKP's C++ package, vendored into assimp to
back `code/AssetLib/Skp/SkpImporter.*`. It carries only the *read* path -
parsing a `.skp` file and baking it to a world-space scene graph. See
[ASSIMP_VENDORING.md](ASSIMP_VENDORING.md) for exactly what was kept, why,
and how to refresh this copy from upstream.

The full upstream package also writes new `.skp` files, edits existing
ones, and converts to glTF (GLB)/OBJ/STL/PLY/DXF/IFC4/JSON - none of that
is present here since assimp's importer never calls it. See
[the project README](https://github.com/iamahsanmehmood/openskp) for the
complete, cross-language (C++/Python/TypeScript/.NET/Dart) picture.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

🏠 [openskp.com](https://openskp.com) · 🌐 [Try the Live Web Viewer](https://iamahsanmehmood.github.io/openskp/) · 📖 [Docs](https://iamahsanmehmood.github.io/openskp/docs/) · [Changelog](https://github.com/iamahsanmehmood/openskp/blob/main/CHANGELOG.md)

> [!IMPORTANT]
> This project was built by reverse engineering a proprietary binary format. It is not affiliated with or endorsed by Trimble Inc. or SketchUp.

## What's here

OpenSKP is the first and only open-source, cross-platform parser for
SketchUp binary files — reverse-engineered from both the modern **VFF
container** (SketchUp 2021+) and the classic **MFC `CArchive`** container
(SketchUp 2013–2020). This subset gives programmatic read access to
geometry, materials, components, layers, and metadata, with no SketchUp
installation and no proprietary SDK required.

## Features

- **Full-fidelity parsing** — vertices, edges, faces, normals, UV
  coordinates, nested component hierarchies, layers/tags, materials,
  textures, styles, and dynamic-component attributes.
- **Both SketchUp file generations** — modern VFF (2021+) and legacy MFC
  (2013–2020) containers, transparently, behind one `parse()` call.
- **Scene baking** — an opt-in `build_scene()` pass resolves the full
  placed scene graph to world-space, triangulated, export-ready geometry.

## Dependencies

Building this subset requires:

- CMake 3.21 or newer.
- A C++17 compiler and standard library, plus a C compiler for the bundled
  miniz sources. GCC, Clang, and MSVC are supported.
- Git and network access during the first CMake configure, unless the
  FetchContent dependency has been provided locally.

CMake fetches this one pinned dependency:

| Dependency | Version | Used for | Required when |
| --- | --- | --- | --- |
| [miniz](https://github.com/richgel999/miniz) | 3.1.2 (`77d0dce8627735138c51770d1799a1ef48f2117d`) | Reading modern SKP ZIP containers | Always |

miniz is compiled privately into OpenSkp and is not a transitive
dependency for installed consumers. The triangulation implementation is
included in this source tree and does not require a separate library.
(The full upstream package also uses TinyGLTF and FlatBuffers for its GLB
and Fragments exporters - neither is vendored here since this subset
doesn't build them; see ASSIMP_VENDORING.md.)

Standard FetchContent source overrides and offline workflows are
supported, including `FETCHCONTENT_SOURCE_DIR_MINIZ`.

clang-format is an optional developer dependency. Version 18 is the canonical
CI version; it is needed only for the formatting targets documented below.

## Build and install

```bash
cmake -S . -B build
cmake --build build
cmake --install build --prefix /your/prefix
```

Consumers use the installed config package:

```cmake
find_package(OpenSkp CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE OpenSkp::OpenSkp)
```

```cpp
#include <openskp/openskp.hpp>

auto file = openskp::SkpFile::open("model.skp");
auto model = file.parse();
auto scene = file.build_scene(); // independent reparse
```

`BUILD_SHARED_LIBS` controls static/shared output (static is the CMake
default). `OPENSKP_BUILD_TESTS` defaults on only when this directory is the
top-level project, and `OPENSKP_BUILD_EXAMPLES` defaults off - both are
forced off when built as part of assimp.

## Formatting

C++ sources use the Google clang-format style with a 100-column limit.
clang-format 18 is the canonical CI version. Data members in structs and
classes use separate declarations: declare one member per line, even when
adjacent members have the same type.

```bash
cmake --build build --target openskp-format
cmake --build build --target openskp-format-check
```

If CMake does not find the desired executable automatically, configure with
`-DOPENSKP_CLANG_FORMAT_EXECUTABLE=/path/to/clang-format`.

## Used in Production

OpenSKP (the full upstream package) powers the SketchUp import pipeline
for [FrameSmart](https://frame-smart.com/) (a 3D collaboration platform
with nearly 200 active users) and [IngeTrazo](https://ingetrazo.com/) (a
SketchUp-alternative 3D modeler with a BIM → IFC bridge).

## License

MIT — see the [root repository](https://github.com/iamahsanmehmood/openskp) for
full documentation and multi-language packages.
