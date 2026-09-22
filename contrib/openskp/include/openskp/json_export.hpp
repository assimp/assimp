#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <openskp/export.hpp>
#include <openskp/model.hpp>
#include <openskp/scene.hpp>

namespace openskp {

/// A minimal, dynamically-typed JSON value tree - used only to build
/// openskp's canonical JSON export schema (shared with the Python
/// (`to_dict`), TypeScript (`toJSON`), Dart (`toJson`), and .NET
/// (`JsonExport.ToDict`) ports). Hand-rolled rather than a dependency,
/// matching how none of the other 4 languages need a JSON library for
/// this either (a plain dict/object/Map/Dictionary already doubles as
/// this there).
class OPENSKP_EXPORT JsonValue {
 public:
  struct Null {};

  using Array = std::vector<JsonValue>;
  // A vector of pairs (not a map) to preserve insertion order, matching
  // every other port's object key ordering.
  using Object = std::vector<std::pair<std::string, JsonValue>>;

  enum class Kind { Null, Bool, Int, Double, String, Array, Object };

  JsonValue() : value_(Null{}) {}

  JsonValue(std::nullptr_t) : value_(Null{}) {}

  JsonValue(bool v) : value_(v) {}

  JsonValue(std::int64_t v) : value_(v) {}

  JsonValue(int v) : value_(static_cast<std::int64_t>(v)) {}

  JsonValue(double v) : value_(v) {}

  JsonValue(std::string v) : value_(std::move(v)) {}

  JsonValue(const char* v) : value_(std::string(v)) {}

  JsonValue(Array v) : value_(std::move(v)) {}

  JsonValue(Object v) : value_(std::move(v)) {}

  static JsonValue make_object() { return JsonValue(Object{}); }

  static JsonValue make_array() { return JsonValue(Array{}); }

  /// Appends a key/value pair. Only meaningful when this value holds an
  /// Object - asserts otherwise.
  void set(std::string key, JsonValue val);

  /// Appends an element. Only meaningful when this value holds an Array -
  /// asserts otherwise.
  void push(JsonValue val);

  Kind kind() const { return static_cast<Kind>(value_.index()); }

  bool is_null() const { return kind() == Kind::Null; }

  bool is_object() const { return kind() == Kind::Object; }

  bool is_array() const { return kind() == Kind::Array; }

  const Object& as_object() const { return std::get<Object>(value_); }

  const Array& as_array() const { return std::get<Array>(value_); }

  const std::string& as_string() const { return std::get<std::string>(value_); }

  std::int64_t as_int() const { return std::get<std::int64_t>(value_); }

  double as_double() const { return std::get<double>(value_); }

  bool as_bool() const { return std::get<bool>(value_); }

  /// Looks up a key in an Object value; returns nullptr when absent or
  /// this value isn't an Object.
  const JsonValue* find(const std::string& key) const;

 private:
  std::variant<Null, bool, std::int64_t, double, std::string, Array, Object> value_;
};

/// Serializes a JsonValue tree to a JSON string. indent <= 0 produces
/// compact (no whitespace) output; indent > 0 pretty-prints with that
/// many spaces per nesting level.
OPENSKP_EXPORT std::string to_json_string(const JsonValue& value, int indent = 2);

/// Converts a parsed SkpModel (and optionally a baked Scene) to
/// openskp's canonical JSON export schema. Pass the result of
/// build_scene() as `scene` to also include the resolved, world-space
/// scene_hierarchy/mesh_index; pass nullptr (the default) for a lighter
/// summary covering just the raw model.
///
/// The raw (pre-bake) per-definition `instances` list is intentionally
/// flat here, with no layer/properties/children keys, even though C++'s
/// own Instance struct is the only one of the 5 languages that actually
/// populates them at parse time (see item 17) - encoding them only for
/// C++ would reintroduce exactly the kind of cross-language schema
/// divergence item 16 exists to eliminate. The resolved, genuinely
/// nested tree (with correct layer/properties, for every language
/// equally) is available via scene_hierarchy instead.
OPENSKP_EXPORT JsonValue to_json(const SkpModel& model, const Scene* scene = nullptr);

/// Serializes to_json()'s result and writes it to a file.
OPENSKP_EXPORT void export_json(const SkpModel& model, const std::filesystem::path& output_path,
                                const Scene* scene = nullptr, int indent = 2);

}  // namespace openskp
