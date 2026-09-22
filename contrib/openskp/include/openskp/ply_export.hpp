#ifndef OPENSKP_PLY_EXPORT_HPP
#define OPENSKP_PLY_EXPORT_HPP

#include <filesystem>
#include <string>
#include <vector>

#include "model.hpp"
#include "scene.hpp"

namespace openskp {

/// Serialize a baked \ref Scene into ASCII PLY text format.
///
/// \param scene The baked scene returned by \ref SkpFile::build_scene.
/// \return Formatted ASCII PLY text string.
std::string to_ply_ascii(const Scene& scene);

/// Serialize a baked \ref Scene into Little-Endian Binary PLY format.
///
/// \param scene The baked scene returned by \ref SkpFile::build_scene.
/// \return Byte vector containing binary PLY data.
std::vector<std::uint8_t> to_ply_binary(const Scene& scene);

/// Export a baked \ref Scene to a PLY file at \p path.
///
/// \param scene The baked scene returned by \ref SkpFile::build_scene.
/// \param path Destination file path (.ply).
/// \param binary If true, writes Binary PLY. Otherwise writes ASCII PLY.
void export_ply(const Scene& scene, const std::filesystem::path& path, bool binary = false);

}  // namespace openskp

#endif  // OPENSKP_PLY_EXPORT_HPP
