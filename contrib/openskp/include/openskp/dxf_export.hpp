#ifndef OPENSKP_DXF_EXPORT_HPP
#define OPENSKP_DXF_EXPORT_HPP

#include <filesystem>
#include <string>

#include "model.hpp"
#include "scene.hpp"

namespace openskp {

/**
 * Serialize a baked Scene into AutoCAD R2000 (AC1015) 3D ASCII DXF text format.
 * Exports Polyface Mesh (POLYLINE 70=64) entities with layer and entity RGB materials.
 *
 * @param scene The baked scene returned by SkpFile::build_scene()
 * @param scale Scale factor for vertex coordinates (default: METRES_TO_INCHES)
 * @param mode Export mode ("3dface" or "polyface", default: "polyface")
 * @return Formatted ASCII DXF text string.
 */
std::string to_dxf(const Scene& scene, double scale = METRES_TO_INCHES,
                   const std::string& mode = "polyface");

/**
 * Export a baked Scene directly to an AutoCAD R2000 3D DXF file.
 *
 * @param scene The baked scene returned by SkpFile::build_scene()
 * @param path Destination file path (.dxf)
 * @param scale Scale factor for vertex coordinates (default: METRES_TO_INCHES)
 * @param mode Export mode ("3dface" or "polyface", default: "polyface")
 */
void export_dxf(const Scene& scene, const std::filesystem::path& path,
                double scale = METRES_TO_INCHES, const std::string& mode = "polyface");

}  // namespace openskp

#endif  // OPENSKP_DXF_EXPORT_HPP
