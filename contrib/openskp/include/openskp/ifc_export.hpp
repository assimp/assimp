#ifndef OPENSKP_IFC_EXPORT_HPP
#define OPENSKP_IFC_EXPORT_HPP

#include <filesystem>
#include <functional>
#include <string>

#include "model.hpp"
#include "scene.hpp"

namespace openskp {

// METRES_TO_INCHES (SketchUp's native unit) is declared once in
// model.hpp, shared with dxf_export.hpp - kept available here for callers
// that want inch-scaled coordinates explicitly, but it is NOT the
// default scale below: the file always declares its length unit as
// millimetres (see IFCSIUNIT in ifc_export.cpp), so the default scale has
// to produce millimetre-scaled values or every coordinate reads back
// ~25.4x too small in any IFC consumer that respects the unit
// declaration.
constexpr double METRES_TO_MM = 1000.0;

/// A (STEP_ENTITY_TYPE, IFC_CLASS_NAME) classification, and the signature
/// a custom classifier passed to to_ifc()/export_ifc() must match.
using IfcClassifier =
    std::function<std::pair<std::string, std::string>(const std::string&, const std::string&)>;

/**
 * Generate a standard 22-character IFC base64 compressed GUID.
 */
std::string generate_ifc_guid();

/**
 * Classify a geometry/component name to an IFC entity type (STEP_TYPE, CLASS_NAME).
 *
 * Tries geom_name first, then falls back to layer_name (many
 * SketchUp-for-BIM workflows organize by tag/layer - "Walls", "Doors" -
 * even when individual components are never renamed away from
 * SketchUp's own defaults like "Component#109415").
 *
 * If neither matches and classify_using_full_path is set, falls back
 * further to keyword-matching the component's full ancestor hierarchy
 * path (e.g. "ROOT / Wall Frame / Stud 12") - a broader, noisier signal
 * than the component's own name/layer, since it can match on an
 * ancestor's name rather than the part itself. Off by default: matching
 * keywords against a mangled internal path unconditionally was a past
 * bug here (it also corrupted the element's displayed Name), so that
 * broader matching is an explicit opt-in rather than the only behavior
 * available. Only if nothing at all matches does this fall back to a
 * generic, untyped element.
 */
std::pair<std::string, std::string> classify_element(const std::string& geom_name,
                                                     const std::string& layer_name = "",
                                                     const std::string& path_name = "",
                                                     bool classify_using_full_path = false);

/**
 * Serialize a baked Scene into ISO-10303-21 STEP ASCII IFC4 format.
 *
 * @param scene The baked scene returned by SkpFile::build_scene()
 * @param scale Scale factor for vertex coordinates (default: METRES_TO_MM -
 *   matches the millimetre length unit this exporter always declares)
 * @param schema IFC schema version (default: "IFC4")
 * @param classifier Optional override for classify_element() - supply your
 *   own naming convention or metadata-driven typing instead of the
 *   built-in keyword/layer heuristic. Ignored (never called) when
 *   classify_using_full_path is set, since that flag only affects the
 *   built-in classifier.
 * @param classify_using_full_path When the built-in classifier (i.e.
 *   classifier is not given) can't type an element from its own name or
 *   layer, also try keyword-matching its full ancestor hierarchy path.
 *   Off by default - see classify_element() for why.
 * @return Formatted ASCII IFC text string.
 */
std::string to_ifc(const Scene& scene, double scale = METRES_TO_MM,
                   const std::string& schema = "IFC4", const IfcClassifier& classifier = nullptr,
                   bool classify_using_full_path = false);

/**
 * Export a baked Scene directly to an ISO-10303-21 STEP ASCII IFC4 file.
 *
 * @param scene The baked scene returned by SkpFile::build_scene()
 * @param path Destination file path (.ifc)
 * @param scale Scale factor for vertex coordinates (default: METRES_TO_MM -
 *   matches the millimetre length unit this exporter always declares)
 * @param schema IFC schema version (default: "IFC4")
 * @param classifier Optional override for classify_element() - see to_ifc().
 * @param classify_using_full_path See to_ifc().
 */
void export_ifc(const Scene& scene, const std::filesystem::path& path, double scale = METRES_TO_MM,
                const std::string& schema = "IFC4", const IfcClassifier& classifier = nullptr,
                bool classify_using_full_path = false);

}  // namespace openskp

#endif  // OPENSKP_IFC_EXPORT_HPP
