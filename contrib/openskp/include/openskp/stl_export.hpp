#ifndef OPENSKP_STL_EXPORT_HPP
#define OPENSKP_STL_EXPORT_HPP

#include <filesystem>
#include <string>
#include <vector>

#include "model.hpp"
#include "scene.hpp"

namespace openskp {

/// Serialize a baked \ref Scene into ASCII STL text format.
///
/// \param scene The baked scene returned by \ref SkpFile::build_scene.
/// \param scale Optional scale multiplier (e.g. 1000.0f for mm).
/// \return Formatted ASCII STL text string.
std::string to_stl_ascii(const Scene& scene, float scale = 1.0f);

/// Serialize a baked \ref Scene into Little-Endian Binary STL format.
///
/// \param scene The baked scene returned by \ref SkpFile::build_scene.
/// \param scale Optional scale multiplier (e.g. 1000.0f for mm).
/// \return Byte vector containing binary STL data.
std::vector<std::uint8_t> to_stl_binary(const Scene& scene, float scale = 1.0f);

/// Export a baked \ref Scene to an STL file at \p path.
///
/// \param scene The baked scene returned by \ref SkpFile::build_scene.
/// \param path Destination file path (.stl).
/// \param binary If true, writes Binary STL. Otherwise writes ASCII STL.
/// \param scale Optional scale multiplier (e.g. 1000.0f for mm).
void export_stl(const Scene& scene, const std::filesystem::path& path, bool binary = false,
                float scale = 1.0f);

}  // namespace openskp

#endif  // OPENSKP_STL_EXPORT_HPP
