module;

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <cstddef>
#include <string_view>

export module model:text;

import :math;
import :ray;
import :hit;

export namespace rt2::model {

// Extruded FreeType outlines in the XY plane, from z = 0 to extrusion.
// Font coordinates are scaled to one scene unit per em; layout starts at
// the baseline origin. UTF-8, kerning and newlines are supported, not shaping.
// FreeType development files are required; the caller supplies a licensed font.
struct text_t {
    // Copies share immutable curves and the glyph R-tree, not FreeType handles.
    // Rejects invalid UTF-8, missing glyphs, text without outlines and nonpositive depth.
    text_t(const std::filesystem::path& font, std::string_view text,
        scalar_t extrusion = scalar_t{0.2f});

    std::optional<box3_t> bounds() const;
    std::size_t intersect(const ray_t& ray, std::span<hit_t> hits) const;
    // Strict interior; caps and outline boundaries are excluded.
    bool inside(const vector_t& point) const;

private:
    struct data_t;
    std::shared_ptr<const data_t> data;
};

}
