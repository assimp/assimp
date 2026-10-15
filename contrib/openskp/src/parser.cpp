#include <fstream>

#include "internal.hpp"

namespace openskp {
static ByteBuffer read_file(const std::filesystem::path& p) {
  auto full = std::filesystem::absolute(p);
  if (!std::filesystem::exists(full))
    throw std::filesystem::filesystem_error(
        "File not found", full, std::make_error_code(std::errc::no_such_file_or_directory));
  auto ext = full.extension().string();
  for (auto& c : ext) c = char(std::tolower(static_cast<unsigned char>(c)));
  if (ext != ".skp") throw std::invalid_argument("Expected a .skp file, got: " + ext);
  std::ifstream f(full, std::ios::binary | std::ios::ate);
  if (!f) throw std::runtime_error("Cannot open: " + full.string());
  auto n = f.tellg();
  if (n < 0) throw std::runtime_error("Cannot determine file size");
  ByteBuffer d(static_cast<std::size_t>(n));
  f.seekg(0);
  f.read(reinterpret_cast<char*>(d.data()), n);
  if (!f && n) throw std::runtime_error("Cannot read: " + full.string());
  return d;
}

SkpFile SkpFile::open(const std::filesystem::path& p) { return SkpFile(read_file(p)); }

SkpFile SkpFile::from_buffer(ByteBuffer b) { return SkpFile(std::move(b)); }

SkpModel SkpFile::parse(const ParseOptions& o) const { return parse_skp(data_, o); }

Scene SkpFile::build_scene(const ParseOptions& o) const { return openskp::build_scene(data_, o); }

InstancedScene SkpFile::build_instanced_scene(const ParseOptions& o) const {
  return openskp::build_instanced_scene(data_, o);
}

SkpModel parse_skp(ByteBuffer b, const ParseOptions& o) { return build_model(full_parse(b, o), o); }

Scene build_scene(ByteBuffer b, const ParseOptions& o) {
  return build_scene_raw(full_parse(b, o), o);
}

InstancedScene build_instanced_scene(ByteBuffer b, const ParseOptions& o) {
  return build_instanced_scene_raw(full_parse(b, o), o);
}
}  // namespace openskp
