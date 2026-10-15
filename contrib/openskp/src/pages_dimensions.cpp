#include <algorithm>
#include <functional>

#include "internal.hpp"

// VFF (2021+) scenes ("pages") and linear dimensions. Ported from Python's
// _core.py (PR #190) - see that module's _scan_vertex_positions /
// _scan_instance_transforms / _parse_dimensions / _find_page_node /
// _parse_pages for the byte-format details this file mirrors.

namespace openskp {
namespace {

// ── flat TLV, integer tags ──────────────────────────────────────────────
// A second, deliberately separate flat-TLV reader from parse_flat/flat_find
// (geometry.cpp): those use the STRING byte-order tag convention the main
// tree already relies on (e.g. "C409"), while the sub-records this feature
// reads (5208, 520A, 53FC, 5BCD, ...) are most directly and safely ported
// from Python's own _tlv_items/_tlv_find (which read the tag as a
// little-endian uint16) by keeping the SAME integer convention here,
// copying Python's numeric constants byte-for-byte rather than
// hand-converting each one to the swapped string form. Every payload is
// copied (ByteBuffer by value) rather than referenced by view: C++17 has
// no std::span, and a pointer into a temporary vector's element would
// dangle the moment that vector goes out of scope - the payloads involved
// here (leaf tags, floats, small ids) are small enough that copying costs
// nothing that matters.

struct FlatTlvItem {
  std::uint16_t tag;
  ByteBuffer payload;
};

std::vector<FlatTlvItem> tlv_items_int(const ByteBuffer& buf) {
  std::vector<FlatTlvItem> items;
  std::size_t off = 0;
  std::size_t n = buf.size();
  while (off < n) {
    if (off + 6 > n) return {};
    std::uint16_t tag = static_cast<std::uint16_t>(buf[off] | (buf[off + 1] << 8));
    auto ln = read_u32(buf, off + 2);
    if (tag == 0 || off + 6 + ln > n) return {};
    items.push_back({tag, ByteBuffer(buf.begin() + off + 6, buf.begin() + off + 6 + ln)});
    off += 6 + ln;
  }
  return items;
}

std::optional<ByteBuffer> tlv_find_int(const std::vector<FlatTlvItem>& items, std::uint16_t tag) {
  for (auto& it : items) {
    if (it.tag == tag) return it.payload;
  }
  return std::nullopt;
}

std::string hex_upper(const ByteBuffer& p) {
  static char h[] = "0123456789ABCDEF";
  std::string s;
  s.reserve(p.size() * 2);
  for (auto b : p) {
    s += h[b >> 4];
    s += h[b & 15];
  }
  return s;
}

ByteBuffer strip_de05(const ByteBuffer& p) {
  if (p.size() >= 6 && p[0] == 0xde && p[1] == 0x05) {
    auto idlen = read_u32(p, 2);
    if (idlen <= p.size() - 6) return ByteBuffer(p.begin() + 6, p.begin() + 6 + idlen);
  }
  return p;
}

const TlvNode* find_child_tag(const std::vector<TlvNode>& nodes, const std::string& target) {
  for (auto& n : nodes) {
    if (n.tag == target) return &n;
    if (auto* r = find_child_tag(n.children, target)) return r;
  }
  return nullptr;
}

std::optional<Vec3> vec3_of(const std::optional<ByteBuffer>& p) {
  if (p && p->size() == 24) return Vec3{read_f64(*p, 0), read_f64(*p, 8), read_f64(*p, 16)};
  return std::nullopt;
}

}  // namespace

// Accumulate every vertex's persistent id (hex) -> (x, y, z) inches. A
// vertex is a "C409" record: "DC05" holds its persistent id (the "DE05"
// var-int payload), "C509" its 3xf64 position. Dimension connection points
// reference geometry by this id. Called once per top-level record -
// full_parse streams the TLV tree and never holds it whole.
void scan_vertex_positions(const TlvNode& top, std::map<std::string, Vec3>& id2pos) {
  // Operates on `top` (and el.children, all real members) by reference the
  // whole way down - never wraps a node in a synthetic {node} vector,
  // which would construct a TEMPORARY vector (a value-type TlvNode copies
  // its whole subtree) whose lifetime ends at that expression, dangling
  // any pointer taken into it.
  std::function<void(const TlvNode&)> visit = [&](const TlvNode& el) {
    if (el.tag == "C409") {
      auto* dc05 = find_child_tag(el.children, "DC05");
      auto* c509 = find_child_tag(el.children, "C509");
      if (dc05 && c509 && c509->payload.size() == 24) {
        auto idb = strip_de05(dc05->payload);
        id2pos[hex_upper(idb)] = {read_f64(c509->payload, 0), read_f64(c509->payload, 8),
                                  read_f64(c509->payload, 16)};
      }
    }
    for (auto& c : el.children) visit(c);
  };
  visit(top);
}

// Accumulate each instance's persistent id (hex) -> its WORLD transform (a
// 13-double matrix, or an empty vector for the identity/no-transform
// case), walking the instance tree and composing parent x local at every
// "6419". Per top-level record, like scan_vertex_positions - an instance
// chain never crosses top-level records.
//
// A dimension connects to geometry INSIDE a placed component; its
// connection reference names the vertex AND the instance holding it. The
// vertex position is definition-local, so it must be lifted to world by
// the instance's transform for the dimension to land where the author
// drew it.
void scan_instance_transforms(const TlvNode& top,
                              std::map<std::string, std::vector<double>>& world) {
  // Same reference-only traversal discipline as scan_vertex_positions -
  // no synthetic {node} vector wrapping.
  std::function<void(const TlvNode&, const std::vector<double>&)> visit =
      [&](const TlvNode& el, const std::vector<double>& parent) {
        if (el.tag == "6419") {
          auto* d007 = find_child_tag(el.children, "D007");
          auto* dc05 = d007 ? find_child_tag(d007->children, "DC05") : nullptr;
          std::optional<std::string> iid;
          if (dc05) iid = hex_upper(strip_de05(dc05->payload));

          auto* m = find_child_tag(el.children, "6619");
          std::optional<std::vector<double>> mat;
          if (m && m->payload.size() == 104) {
            std::vector<double> v(13);
            for (int i = 0; i < 13; ++i) v[i] = read_f64(m->payload, i * 8);
            mat = std::move(v);
          }
          std::vector<double> here = mat ? multiply_matrices(parent, *mat) : parent;
          if (iid) world[*iid] = here;
          for (auto& c : el.children) visit(c, here);
        } else {
          for (auto& c : el.children) visit(c, parent);
        }
      };
  visit(top, {});
}

// Linear dimensions (SketchUp's Dimension tool).
//
// A dimension entity is a "5BCC" record (raw bytes cc 5b) holding:
//
// * 5BCD / 5BCE - the two connection points. Each wraps a 5208 whose 5209
//   is the connection TYPE (1 = a free explicit point in 520A, already
//   world space; 2 = connected to geometry, 520A is zero and 520B -> 53FC
//   names the target: 53FD = the vertex by persistent id, 53FE = a
//   length-prefixed persistent id of the INSTANCE holding it - the vertex
//   position is definition-local, so it is lifted to world by that
//   instance's transform).
// * 5BCF - the dimension plane's x-axis; 5BD0 - its normal.
// * 5BD2 - the offset distance (inches): how far the dimension line sits
//   from the measured segment, along the in-plane perpendicular.
//
// The measured value is auto-computed from the two points (no cached text
// on the samples seen), so callers format it themselves. Endpoints come
// out in WORLD space (inches). A connection point that cannot be resolved
// drops the whole dimension (fail-safe).
std::vector<RawDimension> parse_dimensions(
    const ByteBuffer& model_dat, const std::map<std::string, Vec3>& id2pos,
    const std::map<std::string, std::vector<double>>& inst_world) {
  std::vector<RawDimension> dims;
  const ByteBuffer needle{0xcc, 0x5b};
  std::size_t i = 0;
  std::size_t n = model_dat.size();

  auto point = [&](const std::optional<ByteBuffer>& block_payload) -> std::optional<Vec3> {
    if (!block_payload) return std::nullopt;
    auto blk_items = tlv_items_int(*block_payload);
    auto blk = tlv_find_int(blk_items, 0x5208);
    if (!blk) return std::nullopt;
    auto sub = tlv_items_int(*blk);
    auto typ_b = tlv_find_int(sub, 0x5209);
    std::optional<std::uint32_t> typ;
    if (typ_b && typ_b->size() == 4) typ = read_u32(*typ_b, 0);
    if (typ == 1u) {
      auto pos = tlv_find_int(sub, 0x520a);
      return vec3_of(pos);
    }
    // type 2: resolve the geometry reference (vertex + instance).
    auto ref_b = tlv_find_int(sub, 0x520b);
    std::optional<ByteBuffer> f53fc;
    if (ref_b) {
      auto ref_items = tlv_items_int(*ref_b);
      f53fc = tlv_find_int(ref_items, 0x53fc);
    }
    std::vector<FlatTlvItem> fi;
    if (f53fc) fi = tlv_items_int(*f53fc);
    auto vid = tlv_find_int(fi, 0x53fd);
    auto iid = tlv_find_int(fi, 0x53fe);
    if (!vid) return std::nullopt;
    auto local_it = id2pos.find(hex_upper(*vid));
    if (local_it == id2pos.end()) return std::nullopt;
    if (iid && !iid->empty() && (*iid)[0] > 0 && std::size_t(1 + (*iid)[0]) <= iid->size()) {
      ByteBuffer id_bytes(iid->begin() + 1, iid->begin() + 1 + (*iid)[0]);
      auto w_it = inst_world.find(hex_upper(id_bytes));
      if (w_it != inst_world.end() && !w_it->second.empty()) {
        return transform_point(w_it->second, local_it->second);
      }
    }
    return local_it->second;  // model-root vertex - already world
  };

  while (true) {
    auto found = std::search(model_dat.begin() + i, model_dat.end(), needle.begin(), needle.end());
    if (found == model_dat.end()) break;
    std::size_t j = static_cast<std::size_t>(found - model_dat.begin());
    i = j + 1;
    if (j + 6 > n) continue;
    auto ln = read_u32(model_dat, j + 2);
    if (ln < 40 || j + 6 + ln > n) continue;
    ByteBuffer body_bytes(model_dat.begin() + j + 6, model_dat.begin() + j + 6 + ln);
    auto body = tlv_items_int(body_bytes);
    if (body.empty()) continue;
    bool has_5bcd = false, has_5bce = false;
    for (auto& it : body) {
      if (it.tag == 0x5bcd) has_5bcd = true;
      if (it.tag == 0x5bce) has_5bce = true;
    }
    if (!has_5bcd || !has_5bce) continue;

    auto a = point(tlv_find_int(body, 0x5bcd));
    auto b = point(tlv_find_int(body, 0x5bce));
    if (!a || !b) continue;

    auto xaxis_b = tlv_find_int(body, 0x5bcf);
    auto normal_b = tlv_find_int(body, 0x5bd0);
    auto off_b = tlv_find_int(body, 0x5bd2);

    RawDimension d;
    d.a = *a;
    d.b = *b;
    d.plane_x = vec3_of(xaxis_b);
    d.normal = vec3_of(normal_b);
    d.offset = (off_b && off_b->size() == 8) ? read_f64(*off_b, 0) : 0.0;
    dims.push_back(std::move(d));
  }
  return dims;
}

// Return the "0702" scenes node inside top's subtree, or nullptr. Called
// per top-level record; retaining the (small) 0702 subtree is the only
// thing kept alive past the streaming loop.
const TlvNode* find_page_node(const TlvNode& top) {
  // Checks `top` itself first, then delegates to find_child_tag on
  // top.children (a real member) - never wraps `top` in a synthetic
  // {top} vector (see scan_vertex_positions's comment on why that
  // dangles).
  if (top.tag == "0702") return &top;
  return find_child_tag(top.children, "0702");
}

// Scenes ("pages"). The 0702 node's payload nests 6D60 > 6D61 > one 7148
// record per page:
//
// * 6F54 > 6F55 - page name (UTF-8)
// * 714A > 34BC - camera: 34BD eye, 34BE target, 34BF up (3xf64, inches),
//   34C4 field of view (degrees), 34C2 u8 = PERSPECTIVE flag (00 =
//   parallel projection - calibrated against the bundled scene
//   thumbnails: parallel plans/elevations carry 00 and their 34C3 visible
//   height matches the thumbnail framing exactly, while perspective
//   scenes carry 01 with a stale 34C3), 34C3 f64 = visible height when
//   parallel (inches)
// * 7150 - layers hidden in this page: (u8 length, var-int layer id) runs
std::vector<RawPage> parse_pages(const TlvNode* node) {
  std::vector<RawPage> pages;
  if (!node) return pages;

  auto t60_items = tlv_items_int(node->payload);
  for (auto& it60 : t60_items) {
    if (it60.tag != 0x6d60) continue;
    auto t61_items = tlv_items_int(it60.payload);
    for (auto& it61 : t61_items) {
      if (it61.tag != 0x6d61) continue;
      auto t48_items = tlv_items_int(it61.payload);
      for (auto& it48 : t48_items) {
        if (it48.tag != 0x7148) continue;
        auto items = tlv_items_int(it48.payload);
        if (items.empty()) continue;

        RawPage page;

        auto head_payload = tlv_find_int(items, 0x6f54);
        std::vector<FlatTlvItem> head;
        if (head_payload) head = tlv_items_int(*head_payload);
        auto name = tlv_find_int(head, 0x6f55);
        if (name && !name->empty()) {
          page.name.assign(reinterpret_cast<const char*>(name->data()), name->size());
        }

        auto cam_wrap_payload = tlv_find_int(items, 0x714a);
        std::vector<FlatTlvItem> cam_wrap;
        if (cam_wrap_payload) cam_wrap = tlv_items_int(*cam_wrap_payload);
        auto cam_payload = tlv_find_int(cam_wrap, 0x34bc);
        std::vector<FlatTlvItem> cam;
        bool has_cam = false;
        if (cam_payload) {
          cam = tlv_items_int(*cam_payload);
          has_cam = true;
        }
        if (has_cam) {
          page.eye = vec3_of(tlv_find_int(cam, 0x34bd));
          page.target = vec3_of(tlv_find_int(cam, 0x34be));
          page.up = vec3_of(tlv_find_int(cam, 0x34bf));
          auto fov = tlv_find_int(cam, 0x34c4);
          if (fov && fov->size() == 8) page.fov = read_f64(*fov, 0);
          auto flag = tlv_find_int(cam, 0x34c2);
          page.parallel = flag && !flag->empty() && (*flag)[0] == 0;
          auto height = tlv_find_int(cam, 0x34c3);
          if (height && height->size() == 8) page.ortho_height = read_f64(*height, 0);
        }

        auto hidden = tlv_find_int(items, 0x7150);
        std::size_t off = 0;
        while (hidden && off + 1 <= hidden->size()) {
          std::uint8_t ln = (*hidden)[off];
          if (ln == 0 || off + 1 + ln > hidden->size()) break;
          page.hidden_layer_ids.push_back(
              static_cast<EntityId>(parse_varint(*hidden, off + 1, ln)));
          off += 1 + ln;
        }

        if (page.eye && page.target) pages.push_back(std::move(page));
      }
    }
  }
  return pages;
}
}  // namespace openskp
