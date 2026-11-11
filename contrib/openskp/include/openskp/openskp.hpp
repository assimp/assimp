#pragma once

// This vendored copy carries only the *read* path (parsing a .skp file
// into an InstancedScene) - what assimp's SkpImporter actually needs. The
// upstream openskp repository also ships a writer, editor, and exporters to
// OBJ/STL/PLY/DXF/IFC/GLB/Fragments; those aren't included here since they'd
// otherwise pull in miniz-unrelated third-party deps (tinygltf, FlatBuffers)
// this importer never touches. See ASSIMP_VENDORING.md.

#include <openskp/errors.hpp>
#include <openskp/instanced_scene.hpp>
#include <openskp/model.hpp>
#include <openskp/observability.hpp>
#include <openskp/parser.hpp>
#include <openskp/scene.hpp>
#include <openskp/triangulator.hpp>
