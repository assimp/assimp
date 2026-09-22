#ifndef OPENSKP_OBJ_EXPORT_HPP
#define OPENSKP_OBJ_EXPORT_HPP

#include <filesystem>
#include <string>

#include "model.hpp"
#include "scene.hpp"

namespace openskp {

/// Serialize a baked \ref Scene's materials to Wavefront MTL text representation.
///
/// \param scene The baked scene returned by \ref SkpFile::build_scene.
/// \return The formatted MTL text string.
std::string to_mtl(const Scene& scene);

/// Serialize a baked \ref Scene to Wavefront OBJ text representation.
///
/// \param scene The baked scene returned by \ref SkpFile::build_scene.
/// \param mtl_filename Optional companion .mtl filename to reference.
/// \return The formatted OBJ text string.
std::string to_obj(const Scene& scene, const std::string& mtl_filename = "");

/// Export a baked \ref Scene to a Wavefront OBJ text file at \p path and optional companion MTL
/// file.
///
/// \param scene The baked scene returned by \ref SkpFile::build_scene.
/// \param path Destination file path (.obj).
/// \param export_mtl Whether to export companion .mtl file alongside .obj.
void export_obj(const Scene& scene, const std::filesystem::path& path, bool export_mtl = true);

}  // namespace openskp

#endif  // OPENSKP_OBJ_EXPORT_HPP
