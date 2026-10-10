#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <optional>
#include <regex>
#include <unordered_map>
#include <unordered_set>

#include "internal.hpp"

namespace openskp {
bool legacy_instance_has_guid(const std::string& class_name, std::optional<int> schema) {
  if (!schema) return true;
  return *schema >= (class_name == "CGroup" ? 1 : 5);
}

// Widest zero padding seen between the v20 filler's empty string and the
// count that follows it (9 and 13 bytes occur in real files; the ceiling
// leaves room without letting the probe wander into unrelated records).
constexpr size_t kMaxV20FillerPad = 29;

// Locates the count that follows a v20 filler record, given the offset the
// bad count was read from. Pure byte logic, exposed for tests; see
// retry_count_after_v20_filler (legacy.cpp) for how it is used.
//
// SketchUp 2020 (v20) writes an extra, undocumented record ahead of some
// counts that v17 does not have, which leaves the reader a few bytes early
// and makes it read garbage as the count. The filler is an empty UTF-16
// string record followed by zero padding:
//
//   <ff fe ff> <u8 0>        empty string
//   <zero padding>           runs up to the real count
//
// Rather than hard-code an offset (the number of bytes before the marker
// differs per call site), locate the marker in the short window ahead,
// then take the first plausible u32 that follows the padding. Only the
// EMPTY-string form counts as filler: a real string here would mean
// genuine data, and moving the cursor past it would corrupt the parse.
//
// Returns the count and the offset just past it, or nullopt when the bytes
// do not match the filler layout.
std::optional<V20FillerHit> find_count_after_v20_filler(const ByteBuffer& d, size_t count_pos,
                                                        uint32_t limit) {
  size_t marker_at = std::string::npos;
  for (size_t i = count_pos; i + 4 <= d.size() && i < count_pos + 12; ++i) {
    if (d[i] == 255 && d[i + 1] == 254 && d[i + 2] == 255) {
      marker_at = i;
      break;
    }
  }
  if (marker_at == std::string::npos) return std::nullopt;
  if (d[marker_at + 3] != 0) return std::nullopt;  // non-empty string: real data

  // The count sits past a run of zero padding whose length varies per call
  // site (9 and 13 bytes both occur in real files), but always lands at
  // marker_at + 4 + pad with pad % 4 == 1. Step through those candidate
  // offsets and take the first plausible u32.
  //
  // Deliberately NOT "scan forward to the first non-zero byte": a count
  // that is an exact multiple of 256 has a 0x00 low byte, which such a
  // scan cannot tell apart from padding, so it would skip into the count
  // and misalign every later read. Probing whole u32s at 4-byte strides
  // never inspects an individual byte, so those counts round-trip
  // correctly.
  for (size_t pad = 1; pad <= kMaxV20FillerPad; pad += 4) {
    size_t at = marker_at + 4 + pad;
    if (at + 4 > d.size()) break;
    uint32_t count = read_u32(d, at);
    if (count > 0 && count <= limit) return V20FillerHit{count, at + 4};
  }
  return std::nullopt;
}

namespace {
struct R {
  const ByteBuffer& d;
  size_t p{};

  void need(size_t n) {
    if (p > d.size() || n > d.size() - p) throw std::out_of_range("legacy archive truncated");
  }

  uint8_t u8() {
    need(1);
    return d[p++];
  }

  uint16_t u16() {
    auto v = read_u16(d, p);
    p += 2;
    return v;
  }

  uint32_t u32() {
    auto v = read_u32(d, p);
    p += 4;
    return v;
  }

  double f64() {
    auto v = read_f64(d, p);
    p += 8;
    return v;
  }

  std::vector<double> f64s(size_t n) {
    std::vector<double> v;
    v.reserve(n);
    while (n--) v.push_back(f64());
    return v;
  }

  ByteBuffer raw(size_t n) {
    need(n);
    ByteBuffer v(d.begin() + p, d.begin() + p + n);
    p += n;
    return v;
  }

  bool marker() const {
    return p + 3 <= d.size() && d[p] == 255 && d[p + 1] == 254 && d[p + 2] == 255;
  }

  uint16_t peek_u16() {
    need(2);
    return read_u16(d, p);
  }

  std::string utf16() {
    if (!marker()) throw std::runtime_error("expected legacy string record");
    p += 3;
    auto n = u8();
    uint32_t z = n;
    if (n == 255) {
      z = u16();
      if (z == 65535) z = u32();
    }
    need(size_t(z) * 2);
    std::string s;
    for (uint32_t i = 0; i < z; ++i) {
      uint16_t c = uint16_t(d[p]) | uint16_t(d[p + 1]) << 8;
      p += 2;
      if (c < 0x80)
        s += char(c);
      else if (c < 0x800) {
        s += char(0xc0 | (c >> 6));
        s += char(0x80 | (c & 63));
      } else {
        s += char(0xe0 | (c >> 12));
        s += char(0x80 | ((c >> 6) & 63));
        s += char(0x80 | (c & 63));
      }
    }
    return s;
  }
};

// V is now defined in internal.hpp (moved out of this anonymous
// namespace so LegacySlotEntry there can hold a real shared_ptr<V> - see
// that struct's own comment).

// Same shape as internal.hpp's LegacySlotEntry (which is exposed there
// purely for scan_pages_for_layers's testability - see its own comment)
// - aliased here so every existing use of Entry throughout this file
// stays unchanged.
using Entry = LegacySlotEntry;

// True when the bytes at p are an MFC class-ref to class `slot`. Mirrors both
// encodings Archive::object() decodes: the short 16-bit form (0x8000|slot)
// and, for slots past 0x7fff, the big-tag escape (0x7fff followed by a u32
// of 0x80000000|slot).
bool is_class_ref(const ByteBuffer& d, size_t p, uint64_t slot) {
  if (slot <= 0x7fff) {
    return p + 2 <= d.size() && read_u16(d, p) == (0x8000 | slot);
  }
  return p + 6 <= d.size() && read_u16(d, p) == 0x7fff &&
         read_u32(d, p + 2) == (0x80000000u | uint32_t(slot));
}

// SketchUp 2020 (v20) writes an extra, undocumented record ahead of some
// counts that v17 does not have, which leaves the reader a few bytes early
// and makes it read garbage as the count. The filler is an empty UTF-16
// string record followed by zero padding:
//
//   <ff fe ff> <u8 0>        empty string
//   <zero padding>           runs up to the real count
//
// Rather than hard-code an offset (the number of bytes before the marker
// differs per call site), locate the marker in the short window ahead, then
// take the first non-zero u32 that follows the padding. Only the EMPTY-
// string form counts as filler: a real string here would mean genuine
// data, and moving the cursor past it would corrupt the parse.
//
// This only ever runs after a count came back implausible (or zero), so
// files that were already parsing (v17, and the VFF path) never reach it.
//
// count_pos is the offset the count was read FROM (i.e. r.p - 4). Returns
// the corrected count, or nullopt when this is not the v20 layout.
//
// Declared here (before struct Archive) so callers throughout struct
// Archive can see it; the Archive-aware overload actually implementing
// the extra validation is defined after struct Archive (it needs the
// complete type to inspect slots/next), and this is left a mere
// declaration until then.
std::optional<uint32_t> retry_count_after_v20_filler(R& r, size_t count_pos, uint32_t limit,
                                                     struct Archive* ar = nullptr);

struct Archive {
  R r;
  int ver;
  bool pid;
  uint64_t next{};
  uint64_t base{};
  uint64_t current_loop{};
  bool in_entity_list{};
  std::unordered_map<uint64_t, Entry> slots;
  std::unordered_map<std::string, uint64_t> class_slot;
  std::unordered_map<std::string, int> class_schema;

  // Burned store-map indices (see the CEdgeUse branch of read()): the
  // writer maps an annotation's connection points into the store map
  // WITHOUT writing bytes, so file back-references beyond each burn run
  // ahead of the walker's numbering. Registrations always stay at WALKER
  // indices - no captured slot ever goes stale - and back() translates
  // file references through the burn bands instead. `burns` holds
  // (file_band_start, width) per event; `cum_delta` their total;
  // `annot_watermark` the walker slot right after the last annotation
  // record - the only place a band can start.
  std::vector<std::pair<uint64_t, uint64_t>> burns;
  uint64_t cum_delta{};
  std::optional<uint64_t> annot_watermark;
  std::vector<int> burn_stack;  // per-entity-list burned-item credits
  std::optional<size_t> cline_tail;

  Archive(const ByteBuffer& d, int v) : r{d}, ver(v), pid(v >= 17) {}

  uint64_t alloc(Entry e) {
    auto s = next++;
    slots[s] = std::move(e);
    return s;
  }

  std::tuple<uint64_t, std::string, std::shared_ptr<V>> object(
      std::optional<std::string> expect = {}) {
    auto tag = r.u16();
    if (!tag) return {};
    if (tag == 0x7fff) {
      auto big = r.u32();
      if (big & 0x80000000) return new_class_ref(big & 0x7fffffff, expect);
      return back(big);
    }
    if (tag == 0xffff) {
      auto schema = r.u16();
      auto n = r.u16();
      if (n > 40) throw std::runtime_error("implausible legacy class name");
      auto b = r.raw(n);
      std::string name(b.begin(), b.end());
      auto cs = alloc({true, name, int(schema), {}});
      class_slot[name] = cs;
      class_schema[name] = int(schema);
      return new_obj(name);
    }
    if (tag & 0x8000) return new_class_ref(tag & 0x7fff, expect);
    return back(tag);
  }

  std::tuple<uint64_t, std::string, std::shared_ptr<V>> new_class_ref(
      uint64_t s, std::optional<std::string> expect) {
    auto i = slots.find(s);
    if (i == slots.end()) {
      if (!expect) throw std::runtime_error("unknown legacy class slot");
      slots[s] = {true, *expect, 0, {}};
      class_slot[*expect] = s;
      i = slots.find(s);
    }
    if (!i->second.cls) throw std::runtime_error("class ref points to object");
    return new_obj(i->second.name);
  }

  // Maps a FILE store-map index to the walker's numbering through the
  // burn bands. Returns the walker slot, or nullopt when the reference
  // points INTO a band (a phantom, never-serialized connection point).
  std::optional<uint64_t> translate_ref(uint64_t slot) {
    uint64_t offset = 0;
    for (auto& band : burns) {
      if (slot < band.first) break;
      if (slot < band.first + band.second) return std::nullopt;
      offset += band.second;
    }
    return slot - offset;
  }

  std::tuple<uint64_t, std::string, std::shared_ptr<V>> back(uint64_t s) {
    if (!burns.empty() && s >= burns[0].first) {
      auto walker = translate_ref(s);
      if (!walker) {
        // a phantom (burned) connection-point index - annotation metadata
        // only; nothing was ever serialized for it
        return {s, "reserved", {}};
      }
      s = *walker;
    }
    auto i = slots.find(s);
    if (i == slots.end()) {
      if (s < base) return {s, "premodel", {}};
      throw std::runtime_error("legacy backref to unknown slot");
    }
    if (i->second.cls) throw std::runtime_error("legacy backref to class");
    return {s, i->second.name, i->second.v};
  }

  // Records that the writer burned `delta` store-map indices without
  // serializing any bytes for them. See the field comments above for the
  // mechanism this supports.
  void register_burn(uint64_t delta) {
    burns.push_back({*annot_watermark + cum_delta, delta});
    cum_delta += delta;
    annot_watermark.reset();
    // each burn event corresponds to ONE phantom top-level entity that the
    // entity list's declared count includes but the stream never carries -
    // credit it so the list doesn't run past its real end
    if (!burn_stack.empty()) burn_stack.back() += 1;
  }

  // A reference-to-entity tag: dimension connection points and text
  // leader attachments. Unlike object()'s back-ref path, this tolerates a
  // slot the walk has not reached yet - SketchUp serializes a
  // label/dimension BEFORE the entity it anchors to when both live in the
  // same entity list, so the reference can legitimately point forward.
  std::optional<uint64_t> entity_ref() {
    auto tag = r.u16();
    if (!tag) return std::nullopt;
    if (tag == 0x7fff) {
      auto big = r.u32();
      if (big & 0x80000000) throw std::runtime_error("entity ref is a new object");
      return big;
    }
    if (tag == 0xffff || (tag & 0x8000)) throw std::runtime_error("entity ref is a new object");
    return tag;
  }

  // True when the u16 at `at` starts an object read in one of the
  // UNAMBIGUOUS forms: null, escape, class definition, or a class-ref to
  // a class already known. Plain object back-refs are excluded on
  // purpose - any 2-byte junk below 0x8000 would qualify, which is
  // exactly the ambiguity this check exists to avoid.
  bool strict_next_tag(size_t at, bool allow_null = true) {
    if (at + 2 > r.d.size()) return false;
    auto t = read_u16(r.d, at);
    if (t == 0x0000) return allow_null;
    if (t == 0x7fff || t == 0xffff) return true;
    if (t & 0x8000) {
      auto i = slots.find(t & 0x7fff);
      return i != slots.end() && i->second.cls;
    }
    return false;
  }

  // The textured-material payload: an embedded CDib plus applied size,
  // source file name, average colour, and opacity. Shared verbatim
  // between a CMaterial with a texture and a colour-by-layer CLayer that
  // carries a textured material.
  void texture_block(V& v) {
    r.raw(ver >= 17 ? 2 : 1);  // texture flag pad
    auto q = object("CDib");
    v.tex_dib = std::get<0>(q);
    auto begin = r.p, limit = std::min(r.d.size(), r.p + 28);
    size_t marker = begin;
    for (; marker + 3 <= limit; ++marker)
      if (r.d[marker] == 255 && r.d[marker + 1] == 254 && r.d[marker + 2] == 255) break;
    if (marker - begin == 20)
      r.u32();
    else if (marker - begin != 16)
      throw std::runtime_error("texture size block misaligned");
    v.tw = r.f64();
    v.th = r.f64();
    v.tex_file = r.utf16();
    auto c = r.raw(9);  // RGBA + 00 + RGBA (colour stored twice)
    v.r = c[0];
    v.g = c[1];
    v.b = c[2];
    // c[3] here is not a colour alpha byte - it feeds the colorized check
    // below alongside blob[4]. v.a keeps its default (255); textured
    // materials/layers carry no separate alpha channel in this record
    // shape.
    r.utf16();
    auto blob = r.raw(8);  // u32 + u32 colorized flag
    v.opacity = r.f64();
    if (!r.u8()) v.opacity = 0;
    v.colorized = blob[4] != 0 || c[3] == 255;
  }

  // Returns the CAttributeContainer's slot, or nullopt when this entity
  // has none (a null object reference: tag 0, Archive::object() returns a
  // default-constructed tuple with an empty class name).
  std::optional<std::uint64_t> preamble() {
    auto attrs = object("CAttributeContainer");
    if (pid) {
      auto mask = r.u8();
      for (int i = 0; i < 8; ++i)
        if (mask & (1 << i)) r.u8();
    }
    if (std::get<1>(attrs).empty()) return std::nullopt;
    return std::get<0>(attrs);
  }

  void draw(V& v) {
    auto b = r.raw(8);
    v.mat = int(b[0] | b[1] << 8);
    v.hidden = b[2];
    v.soft = b[5];
    v.smooth = b[6];
    // The layer field is normally a u16 id, but an entity can carry the
    // layer BY OBJECT instead (seen on real 2018 instances): a full
    // inline CLayer record on first use, an escaped back-ref to it on
    // later siblings. Layer ids never have the 0x8000 bit and never equal
    // 0x7fff, so both object forms are unambiguous.
    auto lay_cs = class_slot.find("CLayer");
    auto tag = read_u16(r.d, r.p);
    if (lay_cs != class_slot.end() && tag == (0x8000 | lay_cs->second)) {
      object("CLayer");
      v.layer = 0;  // by-object layer: keep the default id
    } else if (tag == 0x7fff) {
      r.u16();
      auto big = r.u32();
      if (big & 0x80000000) throw std::runtime_error("drawbase layer: unexpected class");
      v.layer = 0;  // by-object layer (back-ref)
    } else {
      v.layer = r.u16();
    }
  }

  std::tuple<uint64_t, std::string, std::shared_ptr<V>> new_obj(const std::string& n) {
    auto slot = alloc({false, n, 0, {}});
    auto v = read(n, slot);
    slots[slot].v = v;
    if (n == "CDimensionLinear" || n == "CText") {
      annot_watermark = next;
    }
    return {slot, n, v};
  }

  std::shared_ptr<V> read(const std::string& n, uint64_t self) {
    auto v = std::make_shared<V>();
    if (n == "CVertex") {
      preamble();
      v->k = "vertex";
      auto a = r.f64s(3);
      v->xyz = {a[0], a[1], a[2]};
    } else if (n == "CEdge") {
      preamble();
      v->k = "edge";
      draw(*v);
      v->v1 = std::get<0>(object("CVertex"));
      v->v2 = std::get<0>(object("CVertex"));
      object();
    } else if (n == "CCurve") {
      preamble();
      v->k = "curve";
      r.u8();
      r.u32();
    } else if (n == "CArcCurve") {
      preamble();
      v->k = "curve";
      r.raw(5);
      r.f64s(14);
    } else if (n == "CEdgeUse") {
      preamble();
      v->k = "edgeuse";
      v->edge = std::get<0>(object("CEdge"));
      v->sense = r.u8() != 0;
      // parent-loop back-ref: the alignment oracle. Read as a RAW file
      // index - after annotations the claimed index can sit AHEAD of the
      // walker's numbering (burned MapObject indices, see
      // register_burn()), which is a correction signal, not a mis-parse.
      size_t p0 = r.p;
      auto tag = r.u16();
      std::optional<uint64_t> parent;
      if (tag == 0x7fff) {
        auto big = r.u32();
        if (big & 0x80000000) throw std::runtime_error("edge-use parent is a new object");
        parent = big;
      } else if (tag == 0xffff || (tag & 0x8000)) {
        throw std::runtime_error("edge-use parent is a new object");
      } else if (tag != 0) {
        parent = tag;
      }
      uint64_t expected = current_loop + cum_delta;
      if (!parent || *parent != expected) {
        int64_t delta = parent ? int64_t(*parent) - int64_t(expected) : 0;
        if (delta > 0 && delta <= 4096 && annot_watermark) {
          register_burn(uint64_t(delta));
        } else {
          r.p = p0;
          throw std::runtime_error("edge-use parent mismatch");
        }
      }
    } else if (n == "CLoop") {
      auto old = current_loop;
      current_loop = self;
      preamble();
      r.raw(2);
      v->k = "loop";
      while (r.p + 2 <= r.d.size() && read_u16(r.d, r.p)) {
        auto q = std::get<2>(object("CEdgeUse"));
        if (q) v->uses.push_back(q);
      }
      r.u16();
      current_loop = old;
    } else if (n == "CFace") {
      v->attrs = preamble();
      v->k = "face";
      draw(*v);
      v->plane = r.f64s(4);
      auto count = r.u32();
      if (count > 10000) throw std::runtime_error("implausible loop count");
      while (count--) {
        auto q = std::get<2>(object("CLoop"));
        if (q) v->loops.push_back(q);
      }
      v->back_mat = r.u16();
    } else if (n == "CAttributeContainer") {
      preamble();
      v->k = "attrs";
      while (r.p + 2 <= r.d.size() && read_u16(r.d, r.p))
        v->ents.push_back(object("CAttributeNamed"));
      r.u16();
    } else if (n == "CAttributeNamed") {
      preamble();
      v->k = "dict";
      r.raw(4);
      v->name = r.utf16();
      while (true) {
        auto key = r.utf16();
        if (key.empty()) break;
        v->entries[key] = typed(r.u8());
      }
      r.u32();
    } else if (n == "CLayer") {
      preamble();
      v->k = "layer";
      v->name = r.utf16();
      ByteBuffer mid;
      while (mid.size() < 8 && !r.marker()) mid.push_back(r.u8());
      v->hidden = mid.empty() ? 0 : mid[0];
      r.utf16();
      auto flags = r.u16();
      if (flags & 0x00ff) {
        // Colour-by-layer with a TEXTURED material: instead of the flat
        // RGBA, the layer embeds the same texture block a CMaterial
        // carries (SketchUp Pro assigns full materials to layers). Low
        // byte of the flag word set = textured; a plain colour layer has
        // 0 there (its high byte carries an unrelated flag, so the word
        // as a whole is non-zero either way).
        texture_block(*v);
        r.raw(4);  // trailing u32
      } else {
        auto c = r.raw(4);
        v->r = c[0];
        v->g = c[1];
        v->b = c[2];
        r.utf16();
        r.raw(21);
      }
    } else if (n == "CMaterial") {
      preamble();
      v->k = "material";
      v->name = r.utf16();
      auto flag = r.u16();
      if (!flag) {
        auto c = r.raw(4);
        v->r = c[0];
        v->g = c[1];
        v->b = c[2];
        v->a = c[3];
        r.utf16();
        r.raw(8);
        v->opacity = r.f64();
        if (!r.u8()) v->opacity = 0;
      } else {
        texture_block(*v);
      }
    } else if (n == "CDib") {
      v->k = "dib";
      r.u32();
      auto z = r.u32();
      if (z > r.d.size()) throw std::runtime_error("implausible dib length");
      v->blob = r.raw(z);
    } else if (n == "CFaceTextureCoords") {
      preamble();
      v->k = "ftc";
      r.u32();
      auto a = r.f64s(24);
      v->uvf.assign(a.begin(), a.begin() + 9);
      v->uvb.assign(a.begin() + 12, a.begin() + 21);
      auto z = r.u32();
      while (z--) r.f64s(4);
      z = r.u32();
      while (z--) r.f64s(4);
      auto fflags = r.u32();
      auto bflags = r.u32();
      v->front_projected = (fflags & 2) != 0;
      v->back_projected = (bflags & 2) != 0;
    } else if (n == "CCamera") {
      r.raw(137);
      r.u16();
      r.utf16();
      r.raw(33);
    } else if (n == "CThumbnail") {
      preamble();
      object("CCamera");
      object("CDib");
    } else if (n == "CImage") {
      // CImage: an Image entity - instance-shaped: a back-ref to the
      // (already walked) CComponentDefinition holding the image's face
      // and texture, a 3x4 placement, a constant 1.0, the source path
      // string (empty in every sample), and a 16-byte GUID. It appears as
      // a normal entity-list item inside the definition that owns the
      // image (typically a face-me/photo definition), whose own tail the
      // ordinary definition reader then consumes. Its parsed value is
      // never consumed downstream - it exists purely so the byte stream
      // stays aligned.
      preamble();
      v->k = "image";
      draw(*v);
      auto q = object();
      v->def = std::get<0>(q);
      v->xf = r.f64s(12);
      r.f64();    // constant 1.0
      r.utf16();  // source path
      auto g = r.raw(16);
      static char h[] = "0123456789ABCDEF";
      for (auto x : g) {
        v->guid += h[x >> 4];
        v->guid += h[x & 15];
      }
    } else if (n == "CRelationship") {
      // two object pointers (small maps: two u16 back-refs - which read
      // like the "u32" of the public notes; big maps escalate them to
      // big-tags). They bind an annotation to the entity it labels, and
      // the annotation side is routinely serialized BEFORE the geometry
      // side - so these can point forward, past the walk cursor;
      // entity_ref() tolerates that where object()'s back-ref path
      // (rightly) does not.
      preamble();
      v->k = "relationship";
      entity_ref();
      entity_ref();
    } else if (n == "CConstructionLine") {
      preamble();
      v->k = "constructionline";
      draw(*v);
      // point(3) + direction(3) + two signed distance params along
      // direction marking where the visible segment starts/ends - the
      // same shape Sketchup::ConstructionLine's own start/end/direction
      // properties expose. A parameter magnitude past kHugeParam means
      // unbounded in that direction (matches the real API returning nil).
      auto line_params = r.f64s(8);
      v->xyz = {line_params[0], line_params[1], line_params[2]};
      v->direction = {line_params[3], line_params[4], line_params[5]};
      constexpr double kHugeParam = 1e20;
      double start_param = line_params[6], end_param = line_params[7];
      if (std::abs(start_param) < kHugeParam) {
        v->start = Vec3{v->xyz[0] + v->direction[0] * start_param,
                        v->xyz[1] + v->direction[1] * start_param,
                        v->xyz[2] + v->direction[2] * start_param};
      }
      if (std::abs(end_param) < kHugeParam) {
        v->end =
            Vec3{v->xyz[0] + v->direction[0] * end_param, v->xyz[1] + v->direction[1] * end_param,
                 v->xyz[2] + v->direction[2] * end_param};
      }
      // The trailing block varies by the WRITING BUILD, not cleanly by
      // version: 7 bytes on the v17 calibration corpus, 4 on v16 and on a
      // real v18, 0 on another real v17. Self-calibrate on the first
      // guide line of the file - the length that lands on a legitimate
      // next tag (strict forms only) - and cache it for the rest of the
      // file.
      if (!cline_tail) {
        size_t dflt = ver == 17 ? 7 : 4;
        std::vector<size_t> order{dflt};
        for (size_t c : {size_t(0), size_t(4), size_t(7)})
          if (c != dflt) order.push_back(c);
        // two passes: a zero tail full of padding can mimic a null tag,
        // so only accept a null-anchored candidate when no candidate
        // lands on a STRONG form (escape / known class / class
        // definition)
        std::optional<size_t> k;
        for (bool allow_null : {false, true}) {
          for (size_t cand : order) {
            if (strict_next_tag(r.p + cand, allow_null)) {
              k = cand;
              break;
            }
          }
          if (k) break;
        }
        cline_tail = k ? *k : dflt;
      }
      r.raw(*cline_tail);
    } else if (n == "CConstructionPoint") {
      preamble();
      v->k = "constructionpoint";
      draw(*v);
      auto pos = r.f64s(3);
      v->xyz = {pos[0], pos[1], pos[2]};
      r.f64s(3);  // reserved/unused (observed all-zero) - no corresponding
                  // Sketchup::ConstructionPoint property to name it after
      r.u8();     // reserved/unused (observed 0)
    } else if (n == "CSectionPlane") {
      preamble();
      v->k = "sectionplane";
      draw(*v);
      auto first = read_f64(r.d, r.p);
      if (std::abs(first) > 1.0001) object();
      v->plane = r.f64s(4);
      if (r.marker()) {
        v->name = r.utf16();
        v->label = r.utf16();
      }
    } else if (n == "CSkFont") {
      object("CAttributeContainer");
      if (pid) r.u8();
      r.utf16();
      r.raw(15);
    } else if (n == "CDimensionLinear") {
      preamble();
      v->k = "dimension";
      draw(*v);
      v->text = r.utf16();
      object("CSkFont");
      // The tail is NOT a fixed 165-byte blob: it embeds two object
      // references (the dimension's connection points into the
      // geometry). Each is a normal MFC tag - 2 bytes in small files, but
      // 6 bytes once the archive holds more than 0x7ffe objects and the
      // 0x7fff big-tag escape kicks in - so a fixed-size skip walks off
      // the rails exactly on large models.
      r.raw(37);
      entity_ref();  // connection point 1 (may be null)
      r.raw(42);
      entity_ref();  // connection point 2 (may be null)
      r.raw(82);
    } else if (n == "CText") {
      preamble();
      v->k = "text";
      draw(*v);
      object("CSkFont");
      size_t found = std::string::npos;
      for (size_t q = r.p; q + 14 <= std::min(r.d.size(), r.p + 512); ++q)
        if (r.d[q] == 1 && r.d[q + 1] == 0 && r.d[q + 2] == 0 && r.d[q + 3] == 0 &&
            r.d[q + 6] == 3 && r.d[q + 7] == 0 && r.d[q + 8] == 0 && r.d[q + 9] == 0 &&
            r.d[q + 10] == 1 && r.d[q + 11] == 255 && r.d[q + 12] == 254 && r.d[q + 13] == 255) {
          found = q + 11;
          break;
        }
      if (found == std::string::npos) throw std::runtime_error("text delimiter not found");
      r.raw(found - r.p);
      v->text = r.utf16();
      r.raw(5);
      // Optional leader-attachment refs follow the fixed tail (a text
      // label anchored to geometry stores the anchored entities here;
      // they can point FORWARD - see entity_ref()). Only the escaped
      // 6-byte form is recognisable without risk: a 2-byte back-ref here
      // would be indistinguishable from the next list item's tag, and
      // every known sample either has no attachments or lives in a
      // >0x7ffe-object file where the escape is mandatory anyway.
      while (r.p + 2 <= r.d.size() && r.d[r.p] == 255 && r.d[r.p + 1] == 127) {
        if (r.p + 6 > r.d.size()) break;
        uint32_t val = read_u32(r.d, r.p + 2);
        if (val & 0x80000000) break;  // new-object tag - the next entity
        r.raw(6);
      }
    } else if (n == "CComponentDefinition") {
      preamble();
      v->k = "definition";
      r.raw(ver >= 17 ? 22 : 20);
      auto nl = r.u32();
      if (nl > 10000) throw std::runtime_error("implausible def layers");
      // like the model-level layer list, the count is REAL layers (new
      // records or back-refs); SketchUp 2020 interleaves null separators
      // between them
      {
        uint32_t got = 0;
        while (got < nl) {
          if (r.p + 2 <= r.d.size() && read_u16(r.d, r.p) == 0) {
            r.p += 2;
            continue;
          }
          object("CLayer");
          got++;
        }
      }
      auto decl = r.u16();
      if (decl == 0x7fff) r.u32();
      // v20 can drop its undocumented filler right here, swallowing the
      // u32 field (and, behind a layer-separator null, even the decl
      // itself): if the empty-string marker sits in the next few bytes,
      // the real count is the first non-zero u32 after its padding.
      uint32_t count = 0;
      bool filled = false;
      if (ver >= 20) {
        auto c = retry_count_after_v20_filler(r, r.p, 5000000, this);
        if (c) {
          count = *c;
          filled = true;
        }
      }
      if (!filled) {
        r.u32();
        count = r.u32();
      }
      // A zero count is as much a symptom of the v20 filler as an
      // implausibly large one: the reader lands on the leading zero bytes
      // of the filler instead of the count. A genuinely empty definition
      // reads zero with no filler ahead, and retry_count_after_v20_filler
      // leaves those alone.
      if (count > 5000000 || count == 0) {
        auto retry = retry_count_after_v20_filler(r, r.p - 4, 5000000, this);
        if (retry) count = *retry;
      }
      if (count > 5000000) throw std::runtime_error("implausible def entities");
      v->ents = entity_list(count, false);
      auto nr = r.u32();
      if (nr > 100000) {
        auto retry = retry_count_after_v20_filler(r, r.p - 4, 100000, this);
        if (retry) nr = *retry;
      }
      if (nr > 100000) throw std::runtime_error("definition list misaligned");
      while (nr--) object("CRelationship");
      r.u16();
      // The GUID is followed immediately by the name string. Some files
      // (SketchUp 2020) carry two extra bytes ahead of the GUID, which
      // would shift this read and leave the cursor mid-record. Anchor on
      // the string marker that must follow the 16 GUID bytes instead of
      // trusting the fixed prefix width.
      if (!(r.p + 19 <= r.d.size() && r.d[r.p + 16] == 255 && r.d[r.p + 17] == 254 &&
            r.d[r.p + 18] == 255)) {
        for (size_t skip = 1; skip <= 4; ++skip) {
          size_t at = r.p + skip;
          if (at + 19 <= r.d.size() && r.d[at + 16] == 255 && r.d[at + 17] == 254 &&
              r.d[at + 18] == 255) {
            r.p = at;
            break;
          }
        }
      }
      auto g = r.raw(16);
      static char h[] = "0123456789ABCDEF";
      for (auto x : g) {
        v->guid += h[x >> 4];
        v->guid += h[x & 15];
      }
      v->name = r.utf16();
      r.utf16();
      r.utf16();
      r.u32();
      size_t tpos = std::string::npos;
      auto thumb_cs = class_slot.find("CThumbnail");
      for (size_t off = 0; off < 96 && r.p + off + 26 <= r.d.size(); ++off) {
        auto p = r.p + off;
        if (r.d[p] == 255 && r.d[p + 1] == 255 && r.d[p + 4] == 10 && r.d[p + 5] == 0 &&
            std::equal(r.d.begin() + p + 6, r.d.begin() + p + 16, "CThumbnail")) {
          tpos = p;
          break;
        }
        if (thumb_cs != class_slot.end() && is_class_ref(r.d, p, thumb_cs->second)) {
          tpos = p;
          break;
        }
      }
      if (tpos == std::string::npos) throw std::runtime_error("definition thumbnail not found");
      auto gap = r.raw(tpos - r.p);
      v->faces_camera = gap.size() >= 9 && (gap[gap.size() - 9] & 1);
      v->shadows_face_sun = gap.size() >= 9 && (gap[gap.size() - 9] & 2);
      object("CThumbnail");
    } else if (n == "CComponentInstance" || n == "CGroup") {
      v->attrs = preamble();
      v->k = "instance";
      draw(*v);
      auto q = object("CComponentDefinition");
      v->def = std::get<0>(q);
      v->xf = r.f64s(13);
      v->name = r.utf16();
      // The trailing GUID was introduced in CComponentInstance schema 5 and
      // CGroup schema 1. SketchUp 2013 writes component-instance schema 4,
      // whose record ends at the name.
      auto schema = class_schema.find(n);
      std::optional<int> schema_number;
      if (schema != class_schema.end()) schema_number = schema->second;
      if (legacy_instance_has_guid(n, schema_number)) r.raw(16);
    } else
      throw std::runtime_error("no legacy reader for " + n);
    return v;
  }

  // Reads one typed CAttributeNamed value off the stream and returns its
  // string representation, matching the string-valued properties contract
  // extract_legacy_dynamic_properties() (below) produces.
  std::string typed(uint8_t t) {
    switch (t) {
      case 0:
        return "";
      case 4:
        return std::to_string(read_i32(r.raw(4), 0));
      case 6:
        return std::to_string(r.f64());
      case 7:
        return std::to_string(r.u8());
      case 9:
        return std::to_string(r.u32());
      case 10:
        return r.utf16();
      case 12:
        return std::to_string(r.f64());  // Length (a double, inches)
      case 11: {
        auto n = r.u32();
        if (n > 100000) throw std::runtime_error("attr array too large");
        std::string joined;
        while (n--) {
          if (!joined.empty()) joined += ",";
          joined += typed(r.u8());
        }
        return joined;
      }
      case 17: {  // 3D point (Geom::Point3d)
        auto v = r.f64s(3);
        return std::to_string(v[0]) + "," + std::to_string(v[1]) + "," + std::to_string(v[2]);
      }
      case 18: {  // 3D vector (Geom::Vector3d)
        auto v = r.f64s(3);
        return std::to_string(v[0]) + "," + std::to_string(v[1]) + "," + std::to_string(v[2]);
      }
      default:
        throw std::runtime_error("unknown legacy attribute type");
    }
  }

  std::vector<std::tuple<uint64_t, std::string, std::shared_ptr<V>>> entity_list(uint32_t n,
                                                                                 bool root) {
    burn_stack.push_back(0);

    struct PopGuard {
      Archive* a;

      ~PopGuard() { a->burn_stack.pop_back(); }
    } pop_guard{this};

    std::vector<std::tuple<uint64_t, std::string, std::shared_ptr<V>>> v;
    while (v.size() < n) {
      auto save = r.p;
      bool has_burn_credit = !root && !burn_stack.empty() && burn_stack.back() > 0;
      if (has_burn_credit && save + 25 <= r.d.size() && read_u32(r.d, save) == 0 &&
          r.d[save + 22] == 255 && r.d[save + 23] == 254 && r.d[save + 24] == 255) {
        // burned MapObject indices (see register_burn()) mean the
        // declared count includes phantom entities the stream never
        // carries; the definition tail signature (nrel=0 + pad + 16-byte
        // GUID + name marker at +22) marks the list's REAL end
        break;
      }
      try {
        v.push_back(object());
      } catch (...) {
        if (root) {
          // over-declared root counts run into the document tail - stop
          r.p = save;
          break;
        }
        if (has_burn_credit) {
          // this list had burned MapObject indices (see register_burn()):
          // the phantom connection points were also counted as items, so
          // the declared count overshoots the real records. Stop at the
          // failed item - the definition tail that follows (nrel, GUID
          // anchor, thumbnail scan) validates the cut.
          r.p = save;
          break;
        }
        throw;
      }
    }
    return v;
  }
};

// True when the u16 at `at` can legally start an object read: a null, an
// escape, a class definition, a class-ref to a KNOWN class, or an object
// back-ref within the allocated range.
bool plausible_list_tag(Archive& ar, const ByteBuffer& d, size_t at) {
  if (at + 2 > d.size()) return false;
  auto t = read_u16(d, at);
  if (t == 0x0000 || t == 0x7fff || t == 0xffff) return true;
  if (t & 0x8000) {
    auto i = ar.slots.find(t & 0x7fff);
    return i != ar.slots.end() && i->second.cls;
  }
  return t < ar.next;
}

// SketchUp 2020's filler can appear at MORE call sites than the count-only
// read this originally guarded (a v20 layer-separator null before a
// definition's decl field, for one) - the same byte-stride search as
// find_count_after_v20_filler, but each candidate must ALSO look like a
// legal object-read start (plausible_list_tag) to reduce false positives.
// Deliberately a separate search from find_count_after_v20_filler's own
// (which stays a pure, Archive-free function so its own tests keep
// working unchanged): needs Archive for the validation, and Archive is an
// anonymous-namespace type find_count_after_v20_filler's header-declared
// signature cannot reference.
std::optional<uint32_t> retry_count_after_v20_filler(R& r, size_t count_pos, uint32_t limit,
                                                     Archive* ar) {
  const ByteBuffer& d = r.d;
  size_t marker_at = std::string::npos;
  for (size_t i = count_pos; i + 4 <= d.size() && i < count_pos + 12; ++i) {
    if (d[i] == 255 && d[i + 1] == 254 && d[i + 2] == 255) {
      marker_at = i;
      break;
    }
  }
  if (marker_at == std::string::npos) return std::nullopt;
  if (d[marker_at + 3] != 0) return std::nullopt;  // non-empty string: real data
  for (size_t pad = 1; pad <= kMaxV20FillerPad; pad += 4) {
    size_t at = marker_at + 4 + pad;
    if (at + 4 > d.size()) break;
    uint32_t count = read_u32(d, at);
    if (count > 0 && count <= limit && (!ar || plausible_list_tag(*ar, d, at + 4))) {
      r.p = at + 4;
      return count;
    }
  }
  return std::nullopt;
}

struct WalkResult {
  Archive ar;
  std::vector<std::tuple<uint64_t, std::string, std::shared_ptr<V>>> root;
  std::vector<std::pair<uint64_t, std::shared_ptr<V>>> layers;
  std::vector<std::pair<uint64_t, std::shared_ptr<V>>> materials;
};

// Bootstrap the absolute slot base: parse material 1 with a throwaway
// archive; material 2's class-ref tag names CMaterial's true slot.
uint64_t bootstrap_two_materials(const ByteBuffer& data, int ver, size_t mat_hdr) {
  Archive boot(data, ver);
  boot.next = boot.base = 1 << 20;
  boot.r.p = mat_hdr;
  boot.object("CMaterial");
  auto tag = read_u16(data, boot.r.p);
  if (tag == 0xffff || !(tag & 0x8000)) throw std::runtime_error("cannot bootstrap the slot base");
  return tag & 0x7fff;
}

// Slot-base candidates for files where the two-material trick is
// unavailable (0 or 1 materials).
//
// Parse the model prefix (materials, layer list) with a throwaway base; the
// object right after the layer list is the definition-list anchor - an
// ABSOLUTE back-ref to the active layer, an object we just allocated
// relatively. Each walked layer yields one candidate base; with a single
// layer (the common case) the answer is exact.
std::vector<uint64_t> probe_layer_anchor_bases(const ByteBuffer& data, int ver, size_t start,
                                               uint32_t mat_count) {
  Archive boot(data, ver);
  constexpr uint64_t b0 = 1 << 20;
  boot.next = boot.base = b0;
  boot.r.p = start;
  for (uint32_t i = 0; i < mat_count; ++i) boot.object("CMaterial");
  boot.r.u32();
  if (ver >= 17) boot.r.u8();
  auto layer_count = boot.r.u32();
  if (layer_count < 1 || layer_count > 100000)
    throw std::runtime_error("implausible layer count in base probe");
  std::vector<uint64_t> layer_slots;
  for (uint32_t i = 0; i < layer_count; ++i) {
    auto q = boot.object("CLayer");
    layer_slots.push_back(std::get<0>(q));
  }
  auto anchor = boot.object();
  if (std::get<1>(anchor) != "premodel")
    // under the throwaway base every absolute back-ref classifies as
    // premodel; anything else means the prefix did not parse
    throw std::runtime_error("base probe: anchor resolved to " + std::get<1>(anchor));
  uint64_t s = std::get<0>(anchor);
  std::vector<uint64_t> result;
  for (auto rel : layer_slots) {
    int64_t candidate = int64_t(s) - (int64_t(rel) - int64_t(b0));
    if (candidate > 0 && uint64_t(candidate) < b0) result.push_back(uint64_t(candidate));
  }
  return result;
}

WalkResult walk_model(const ByteBuffer& data, int ver, size_t start, uint32_t mat_count,
                      uint64_t base);

// Tries each candidate base in turn, returning the first one that walks
// cleanly. WalkResult holds an Archive whose R holds a reference member, so
// it's move-constructible but not assignable - built via a direct `return`
// from inside the loop (RVO/move-construction) rather than storing into an
// outer-scope optional/variable, which would need assignment instead.
WalkResult walk_with_bases(const ByteBuffer& data, int ver, size_t start, uint32_t mat_count,
                           const std::vector<uint64_t>& bases) {
  std::exception_ptr last_exc;
  for (auto base : bases) {
    try {
      return walk_model(data, ver, start, mat_count, base);
    } catch (const std::exception&) {
      last_exc = std::current_exception();
    }
  }
  if (last_exc) std::rethrow_exception(last_exc);
  throw std::runtime_error("no viable slot base candidate");
}

WalkResult walk_model(const ByteBuffer& data, int ver, size_t start, uint32_t mat_count,
                      uint64_t base) {
  Archive ar(data, ver);
  ar.next = ar.base = base;
  ar.r.p = start;
  std::vector<std::pair<uint64_t, std::shared_ptr<V>>> mats, layers;
  for (uint32_t i = 0; i < mat_count; ++i) {
    auto q = ar.object("CMaterial");
    mats.push_back({std::get<0>(q), std::get<2>(q)});
  }
  ar.r.u32();
  if (ver >= 17) ar.r.u8();
  auto lc = ar.r.u32();
  if (lc > 100000) throw std::runtime_error("invalid layer count");
  // lc counts REAL layers. SketchUp 2020 interleaves a null object-ref
  // after each layer record (a separator, not a layer), so counting reads
  // walks off mid-list on files with several layers; count parsed layers
  // instead, skip the separators, and stop early if the next tag is a
  // back-ref (the definition-list anchor) - a v20 variant where the count
  // over-includes separators.
  while (layers.size() < lc) {
    if (ar.r.p + 2 > data.size()) break;
    auto tag = read_u16(data, ar.r.p);
    if (tag == 0) {
      ar.r.p += 2;
      continue;
    }
    if (tag != 0xffff && !(tag & 0x8000)) break;
    auto q = ar.object("CLayer");
    if (!std::get<2>(q)) continue;
    layers.push_back({std::get<0>(q), std::get<2>(q)});
  }
  // trailing separators (and any layer records past the declared count)
  {
    auto lay_cs = ar.class_slot.find("CLayer");
    while (ar.r.p + 2 <= data.size()) {
      auto tag = read_u16(data, ar.r.p);
      if (tag == 0) {
        ar.r.p += 2;
        continue;
      }
      if (lay_cs != ar.class_slot.end() && tag == (0x8000 | lay_cs->second)) {
        auto q = ar.object("CLayer");
        if (std::get<2>(q)) layers.push_back({std::get<0>(q), std::get<2>(q)});
        continue;
      }
      break;
    }
  }
  auto anchor = ar.object();
  if (std::get<1>(anchor) != "CLayer") throw std::runtime_error("definition anchor is not a layer");
  auto dc = ar.r.u32();
  if (dc > 1000000) {
    auto retry = retry_count_after_v20_filler(ar.r, ar.r.p - 4, 1000000, &ar);
    if (retry) dc = *retry;
  }
  if (dc > 1000000) throw std::runtime_error("invalid definition count");
  while (dc--) ar.object("CComponentDefinition");
  auto cs = ar.class_slot.find("CComponentDefinition");
  while (ar.r.p + 2 <= data.size()) {
    auto t = read_u16(data, ar.r.p);
    bool yes = cs != ar.class_slot.end() && t == (0x8000 | cs->second);
    if (!yes && t == 0xffff && ar.r.p + 26 <= data.size())
      yes =
          std::equal(data.begin() + ar.r.p + 6, data.begin() + ar.r.p + 26, "CComponentDefinition");
    if (!yes) break;
    ar.object();
  }
  auto root_count = ar.r.u32();
  if (root_count > 5000000) {
    auto retry = retry_count_after_v20_filler(ar.r, ar.r.p - 4, 5000000, &ar);
    if (retry) root_count = *retry;
  }
  if (root_count > 5000000) throw std::runtime_error("implausible root entity count");
  auto root = ar.entity_list(root_count, true);
  return {std::move(ar), std::move(root), std::move(layers), std::move(mats)};
}

void add_edge(GeometryBuilder& b, uint64_t s, const V& e,
              const std::unordered_map<uint64_t, Entry>& slots) {
  if (b.edges.count(s)) return;
  for (auto id : {e.v1, e.v2}) {
    auto i = slots.find(id);
    if (i != slots.end() && i->second.v && i->second.v->k == "vertex")
      b.vertices[id] = i->second.v->xyz;
  }
  b.edges[s] = {EntityId(e.v1), EntityId(e.v2)};
  int f = (e.soft ? 8 : 0) | (e.smooth ? 16 : 0) | (e.hidden ? 1 : 0);
  if (f) b.edge_flags[s] = f;
}

// SketchUp's Dynamic Components extension stores its data in an attribute
// dictionary literally named "dynamic_attributes" - a stable, publicly
// documented part of the SketchUp Ruby API
// (Entity#attribute_dictionary("dynamic_attributes")). CAttributeContainer/
// CAttributeNamed reading above already fully decodes an entity's
// attribute dictionaries for other purposes (CFaceTextureCoords lookup on
// faces) - this just looks up that one dictionary by name, mirroring what
// the VFF path's dynamic-properties extraction does for D007/DC05 TLV
// data.
std::map<std::string, std::string> extract_legacy_dynamic_properties(
    std::optional<uint64_t> attrs_slot, const std::unordered_map<uint64_t, Entry>& slots) {
  if (!attrs_slot) return {};
  auto ai = slots.find(*attrs_slot);
  if (ai == slots.end() || !ai->second.v) return {};
  for (auto& ent : ai->second.v->ents) {
    auto& ev = std::get<2>(ent);
    if (ev && ev->k == "dict" && ev->name == "dynamic_attributes") return ev->entries;
  }
  return {};
}

void fill(GeometryBuilder& b,
          const std::vector<std::tuple<uint64_t, std::string, std::shared_ptr<V>>>& ents,
          const std::unordered_map<uint64_t, Entry>& slots) {
  for (auto& x : ents) {
    auto s = std::get<0>(x);
    auto v = std::get<2>(x);
    if (!v) continue;
    if (v->k == "edge")
      add_edge(b, s, *v, slots);
    else if (v->k == "face") {
      RawFace f;
      f.normal = {v->plane[0], v->plane[1], v->plane[2]};
      if (v->mat) f.material_id = v->mat;
      if (v->back_mat) f.back_material_id = v->back_mat;
      f.hidden = v->hidden != 0;
      for (auto& lp : v->loops) {
        std::vector<CoEdge> co;
        for (auto& u : lp->uses) {
          auto i = slots.find(u->edge);
          if (i != slots.end() && i->second.v) {
            add_edge(b, u->edge, *i->second.v, slots);
            co.push_back({EntityId(u->edge), u->sense ? -1 : 1});
          }
        }
        f.loops.push_back(std::move(co));
      }
      // A positioned/photo-fitted texture mapping (CFaceTextureCoords)
      // lives as a child of this face's attribute container, alongside
      // any CAttributeNamed dictionaries - so it has to be found by
      // scanning the resolved attrs' children for one tagged "ftc"
      // rather than read directly off the face record.
      if (v->attrs) {
        auto ai = slots.find(*v->attrs);
        if (ai != slots.end() && ai->second.v) {
          for (auto& ent : ai->second.v->ents) {
            auto& ev = std::get<2>(ent);
            if (!ev || ev->k != "ftc") continue;
            if (ev->uvf.size() == 9) {
              std::array<double, 9> m{};
              std::copy(ev->uvf.begin(), ev->uvf.end(), m.begin());
              f.uv_transform = m;
            }
            if (ev->uvb.size() == 9) {
              std::array<double, 9> m{};
              std::copy(ev->uvb.begin(), ev->uvb.end(), m.begin());
              f.uv_transform_back = m;
            }
            f.uv_projected = ev->front_projected;
            f.uv_projected_back = ev->back_projected;
          }
        }
      }
      b.faces[s] = std::move(f);
    } else if (v->k == "instance") {
      RawInstance i;
      i.name = v->name;
      i.ref_idx = v->def;
      i.matrix = v->xf;
      if (v->mat) i.material_id = v->mat;
      if (v->layer) i.layer = std::to_string(v->layer);
      i.hidden = v->hidden != 0;
      i.properties = extract_legacy_dynamic_properties(v->attrs, slots);
      b.instances.push_back(std::move(i));
    } else if (v->k == "image") {
      // Placed exactly like an ordinary component instance - same
      // transform/definition-reference shape - the only difference is the
      // definition it points at is flagged is_image=true (set below via
      // image_def_ids), matching how the VFF/modern reader already treats
      // an Image entity as "a component placed through the same instance
      // machinery as any other". Previously dropped here entirely: this
      // record's parsed value was documented as "never consumed
      // downstream, exists purely so the byte stream stays aligned" - an
      // Image entity parsed without error but never appeared anywhere a
      // caller could see it.
      RawInstance i;
      i.ref_idx = v->def;
      i.ref_guid = v->guid;
      i.matrix = v->xf;
      if (v->mat) i.material_id = v->mat;
      if (v->layer) i.layer = std::to_string(v->layer);
      i.hidden = v->hidden != 0;
      b.instances.push_back(std::move(i));
    } else if (v->k == "sectionplane") {
      SectionPlane sp;
      if (v->plane.size() == 4) {
        sp.plane = {v->plane[0], v->plane[1], v->plane[2], v->plane[3]};
      }
      sp.name = v->name;
      sp.label = v->label;
      sp.hidden = v->hidden != 0;
      b.section_planes.push_back(std::move(sp));
    } else if (v->k == "text") {
      TextEntity te;
      te.text = v->text;
      te.hidden = v->hidden != 0;
      b.texts.push_back(std::move(te));
    } else if (v->k == "dimension") {
      Dimension dim;
      dim.text = v->text;
      dim.hidden = v->hidden != 0;
      b.dimensions.push_back(std::move(dim));
    } else if (v->k == "constructionline") {
      ConstructionLine cl;
      cl.point = v->xyz;
      cl.direction = v->direction;
      cl.start = v->start;
      cl.end = v->end;
      b.construction_lines.push_back(std::move(cl));
    } else if (v->k == "constructionpoint") {
      ConstructionPoint cp;
      cp.position = v->xyz;
      b.construction_points.push_back(std::move(cp));
    }
  }
}

// ── pages (scenes) ────────────────────────────────────────────────────
//
// CViewPage's full record embeds a camera, an optional thumbnail image, a
// font, and - whenever any of its several independent "use_*" capture
// flags beyond hidden-layers is set - an entire Style sub-object with no
// documented fixed size. Reverse engineering that is out of scope, so
// only the narrow case is supported: a page whose captured flags are
// exactly the baseline (nothing captured) or baseline + hidden-layers.
// Ground truth for both (the flags bitmask, the hidden-layers list shape)
// was captured from real v17-native SketchUp saves - mirrors Python's
// legacy._scan_pages exactly.
constexpr uint32_t kPageAllowedFlags[] = {0xc00, 0xc20};
constexpr uint32_t kPageHiddenLayersBit = 0x20;

bool is_allowed_page_flags(uint32_t flags) {
  for (auto f : kPageAllowedFlags)
    if (f == flags) return true;
  return false;
}

// Same conversion R::utf16() applies per UTF-16 code unit (no surrogate
// pairing - matches this file's existing, already-shipped string
// decoding elsewhere), just addressed by a raw (pos, char_count) range
// instead of a cursor, since the page scan below needs to decode a name
// it has already found the bounds of without re-walking a marker.
std::string decode_utf16le_range(const ByteBuffer& d, size_t pos, size_t char_count) {
  std::string s;
  for (size_t i = 0; i < char_count; ++i) {
    uint16_t c = uint16_t(d[pos]) | uint16_t(d[pos + 1]) << 8;
    pos += 2;
    if (c < 0x80) {
      s += char(c);
    } else if (c < 0x800) {
      s += char(0xc0 | (c >> 6));
      s += char(0x80 | (c & 63));
    } else {
      s += char(0xe0 | (c >> 12));
      s += char(0x80 | ((c >> 6) & 63));
      s += char(0x80 | (c & 63));
    }
  }
  return s;
}

// Consume one reference to an object of `cls_name` - a null tag, a
// back-ref (value unneeded here, so left unresolved), a class-ref to an
// already-declared class, or (rarely, if this is the class's first use
// in the file) a fresh declaration - without relying on the referenced
// object's slot already being known to `slots` the way Archive::object()
// requires. The page scan below runs over a region of the file the main
// walk never visits, so back-refs into that region can't be resolved.
// Mirrors Python's legacy._skip_typed_ref exactly.
void skip_typed_ref(LegacySlotTable& slots, R& r, const std::string& cls_name,
                    const std::function<void(R&)>& read_body) {
  auto tag = r.peek_u16();
  if (tag == 0) {
    r.u16();
    return;
  }
  if (tag == 0xffff) {
    r.u16();
    auto schema = r.u16();
    auto namelen = r.u16();
    if (namelen > 40) throw std::runtime_error("implausible class name length");
    auto raw_name = r.raw(namelen);
    std::string name(raw_name.begin(), raw_name.end());
    if (name != cls_name) throw std::runtime_error("expected " + cls_name + " decl, got " + name);
    if (!slots.class_slot.count(cls_name))
      slots.class_slot[cls_name] = slots.alloc({true, cls_name, int(schema), {}});
    read_body(r);
    return;
  }
  if (tag & 0x8000) {
    uint64_t cslot = tag & 0x7fff;
    auto it = slots.slots.find(cslot);
    if (it == slots.slots.end() || !it->second.cls || it->second.name != cls_name)
      throw std::runtime_error("unexpected " + cls_name + " class-ref");
    r.u16();
    read_body(r);
    return;
  }
  r.u16();  // plain back-ref - the referenced value isn't needed here
}

// Same byte layout as the CCamera/CDib branches of Archive::object()'s
// dispatch above (r.raw(137); r.u16(); r.utf16(); r.raw(33) / r.u32();
// z=r.u32(); r.raw(z)) - duplicated here as standalone bodies since
// skip_typed_ref can't safely route through the full object() dispatch
// (see its own comment) but still needs to consume the exact same bytes
// when a page candidate's camera/thumbnail happens to be a fresh
// declaration or class-ref rather than the usual null tag.
void read_camera_body(R& r) {
  r.raw(137);
  r.u16();
  r.utf16();
  r.raw(33);
}

void read_dib_body(R& r) {
  r.u32();
  auto z = r.u32();
  if (z > r.d.size()) throw std::runtime_error("implausible dib length");
  r.raw(z);
}

}  // namespace

// Best-effort scan for CViewPage (scene) records: name + hidden-layer
// slot ids only, for the narrow flag combinations this reader
// understands (see kPageAllowedFlags above).
//
// Runs as an independent scan over the file tail (from where the main
// entity walk stops) rather than as a sequential archive read: each
// candidate is validated by its own local structure (a name string
// immediately followed by an empty second string and a recognized flags
// word), and any candidate that doesn't fully validate - an unsupported
// flag combination, or a reference into a part of the file this scan
// never visited - is silently skipped rather than guessed at, so a page
// this reader can't fully understand is just absent from the result
// instead of producing wrong data. Mirrors Python's legacy._scan_pages
// exactly. Declared in internal.hpp (see LegacySlotTable's own comment
// for why) - not just a parse_legacy() implementation detail.
std::vector<RawPage> scan_pages_for_layers(const ByteBuffer& data, std::size_t start_pos,
                                           LegacySlotTable slots) {
  std::vector<RawPage> pages;
  size_t pos = start_pos;
  const size_t n = data.size();
  while (true) {
    size_t idx = std::string::npos;
    for (size_t i = pos; i + 3 <= n; ++i) {
      if (data[i] == 255 && data[i + 1] == 254 && data[i + 2] == 255) {
        idx = i;
        break;
      }
    }
    if (idx == std::string::npos) break;
    pos = idx + 1;
    size_t nlen_pos = idx + 3;
    if (nlen_pos >= n) break;
    uint8_t nlen = data[nlen_pos];
    if (nlen == 0 || nlen > 40) continue;
    size_t name_start = nlen_pos + 1;
    size_t name_end = name_start + 2 * size_t(nlen);
    if (name_end + 7 > n) continue;
    if (!(data[name_end] == 255 && data[name_end + 1] == 254 && data[name_end + 2] == 255 &&
          data[name_end + 3] == 0)) {
      continue;  // second name must be empty
    }
    size_t flags_pos = name_end + 4;
    uint32_t flags = read_u32(data, flags_pos);
    if (!is_allowed_page_flags(flags)) continue;
    std::string name = decode_utf16le_range(data, name_start, nlen);
    try {
      R r{data, flags_pos + 4};
      skip_typed_ref(slots, r, "CCamera", read_camera_body);
      skip_typed_ref(slots, r, "CDib", read_dib_body);
      std::vector<EntityId> hidden_ids;
      if (flags & kPageHiddenLayersBit) {
        while (true) {
          size_t save = r.p;
          auto v = r.u16();
          auto it = slots.slots.find(v);
          if (it != slots.slots.end() && !it->second.cls && it->second.name == "CLayer") {
            hidden_ids.push_back(static_cast<EntityId>(v));
          } else {
            r.p = save;
            break;
          }
        }
        if (r.u32() != 1) throw std::runtime_error("unexpected hidden-layers marker");
      }
      RawPage page;
      page.name = std::move(name);
      page.hidden_layer_ids = std::move(hidden_ids);
      pages.push_back(std::move(page));
    } catch (const std::exception&) {
      continue;
    }
  }
  return pages;
}

RawParsed parse_legacy(const ByteBuffer& data, const ParseOptions& o) {
  emit_log(o, LogLevel::information,
           "Parsing legacy MFC container (" + std::to_string(data.size()) + " bytes)");
  RawParsed out;
  out.version = extract_version(data);
  // Legacy (pre-2021 MFC) files carry no meta/meta.dat container - that's
  // a VFF/ZIP-only construct - so there is no known source for the
  // model's unit-system string here (out.units stays unset).
  try {
    std::string ascii;
    for (size_t i = 0; i < std::min<size_t>(96, data.size()); ++i)
      if (data[i]) ascii += char(data[i]);
    std::smatch vm;
    std::regex_search(ascii, vm, std::regex("\\{(\\d+)\\."));
    int ver = std::stoi(vm[1]);

    // anchor: the material manager (u32 count right before the first
    // CMaterial new-class record); zero-material files have no CMaterial
    // record anywhere, so fall back to the first CLayer class record and
    // start at the layer-list marker just before it
    size_t mh = std::string::npos;
    for (size_t i = 0; i + 15 < data.size(); ++i)
      if (data[i] == 255 && data[i + 1] == 255 && read_u16(data, i + 4) == 9 &&
          std::equal(data.begin() + i + 6, data.begin() + i + 15, "CMaterial")) {
        mh = i;
        break;
      }
    size_t start;
    uint32_t mc;
    if (mh != std::string::npos && mh >= 4) {
      start = mh;
      mc = read_u32(data, mh - 4);
      if (mc > 100000) throw std::runtime_error("implausible material count");
    } else {
      size_t lh = std::string::npos;
      for (size_t i = 0; i + 12 < data.size(); ++i)
        if (data[i] == 255 && data[i + 1] == 255 && read_u16(data, i + 4) == 6 &&
            std::equal(data.begin() + i + 6, data.begin() + i + 12, "CLayer")) {
          lh = i;
          break;
        }
      if (lh == std::string::npos)
        throw std::runtime_error("no CMaterial or CLayer class record found");
      mc = 0;
      start = lh - (ver >= 17 ? 9 : 8);
    }

    std::vector<uint64_t> bases;
    if (mc >= 2) {
      bases.push_back(bootstrap_two_materials(data, ver, start));
    } else {
      bases = probe_layer_anchor_bases(data, ver, start, mc);
    }

    WalkResult walked = walk_with_bases(data, ver, start, mc, bases);
    Archive& ar = walked.ar;
    auto& root = walked.root;
    auto& layers = walked.layers;
    auto& mats = walked.materials;
    for (auto& m : mats) {
      auto v = m.second;
      auto x = std::make_shared<RawMaterial>();
      x->name = v->name;
      x->r = v->r;
      x->g = v->g;
      x->b = v->b;
      x->a = v->a;
      x->transparency = std::clamp(1.0 - v->opacity, 0.0, 1.0);
      x->colorized = v->colorized;
      x->colorize_type = v->colorized ? 1 : 0;
      if (v->tex_dib) {
        RawTexture t;
        t.filename = v->tex_file;
        t.x_scale = v->tw;
        t.y_scale = v->th;
        auto di = ar.slots.find(v->tex_dib);
        if (di != ar.slots.end() && di->second.v) t.data = di->second.v->blob;
        if (t.filename.empty())
          t.filename =
              v->name + (t.data && t.data->size() >= 4 && (*t.data)[0] == 0x89 ? ".png" : ".jpg");
        x->texture = std::move(t);
      }
      out.materials[x->name] = x;
      out.material_id_to_name[m.first] = x->name;
    }
    for (auto& l : layers) {
      out.layer_id_to_name[l.first] = l.second->name;
      if (!out.layer_colors.count(l.second->name)) out.layer_order.push_back(l.second->name);
      out.layer_colors[l.second->name] = {uint8_t(l.second->r), uint8_t(l.second->g),
                                          uint8_t(l.second->b)};
      out.layer_hidden[l.second->name] = l.second->hidden != 0;
    }
    if (!out.layer_colors.count("Layer0")) {
      out.layer_order.push_back("Layer0");
      out.layer_colors["Layer0"] = {136, 136, 136};
    }
    if (!out.layer_hidden.count("Layer0")) out.layer_hidden["Layer0"] = false;
    // Scanned before the definitions loop below so is_image can be set
    // correctly the first time a definition is built, matching the VFF
    // reader's RawDefinition::is_image field.
    std::unordered_set<std::uint64_t> image_def_ids;
    for (auto& s : ar.slots)
      if (!s.second.cls && s.second.name == "CImage" && s.second.v) {
        image_def_ids.insert(s.second.v->def);
      }
    for (auto& s : ar.slots)
      if (!s.second.cls && s.second.name == "CComponentDefinition" && s.second.v) {
        RawDefinition d;
        d.name = s.second.v->name;
        d.guid = s.second.v->guid;
        d.always_faces_camera = s.second.v->faces_camera;
        d.shadows_face_sun = s.second.v->shadows_face_sun;
        d.is_image = image_def_ids.count(s.first) != 0;
        fill(d.builder, s.second.v->ents, ar.slots);
        out.definitions[s.first] = std::move(d);
      }
    fill(out.root.builder, root, ar.slots);
    try {
      out.pages =
          scan_pages_for_layers(data, ar.r.p, LegacySlotTable{ar.slots, ar.class_slot, ar.next});
    } catch (const std::exception&) {
      out.pages.clear();
    }
    emit_progress(o, ParseStage::legacy_defs, out.definitions.size(), out.definitions.size());
    emit_log(o, LogLevel::information,
             "Parse complete: " + std::to_string(out.definitions.size()) + " defs");
    return out;
  } catch (const SkpParseError&) {
    throw;
  } catch (...) {
    throw SkpParseError("legacy .skp parse failed", ParseStage::legacy_walk, {}, {}, {}, {}, {},
                        std::current_exception());
  }
}
}  // namespace openskp
