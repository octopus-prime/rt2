module;

// Adapted from gamma-ray-2/source/surface/primitive/text:
// FreeType outlines, extruded Bezier sides and glyph-level spatial indexing.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/math/tools/cubic_roots.hpp>
#include <boost/qvm.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

module model;

namespace rt2::model {
namespace {

using polynomial_t = std::array<scalar_t, 4>;

scalar_t evaluate(const polynomial_t& p, scalar_t t) {
    return ((p[3] * t + p[2]) * t + p[1]) * t + p[0];
}

struct text_curve_t {
    polynomial_t x;
    polynomial_t y;
    std::vector<scalar_t> y_splits;
};

struct text_glyph_t {
    std::vector<text_curve_t> curves;
    box3_t box;
    scalar_t normal_sign;
    bool even_odd;

    bool on_boundary(const vector_t& point) const {
        for (const auto& curve : curves) {
            const scalar_t x_span = std::abs(curve.x[1]) + std::abs(curve.x[2])
                + std::abs(curve.x[3]);
            const scalar_t y_span = std::abs(curve.y[1]) + std::abs(curve.y[2])
                + std::abs(curve.y[3]);
            const bool solve_x = x_span > y_span;
            const auto& p = solve_x ? curve.x : curve.y;
            const auto roots = boost::math::tools::cubic_roots(
                p[3], p[2], p[1], p[0] - point.a[solve_x ? 0 : 1]);
            const scalar_t tolerance = 16 * std::numeric_limits<scalar_t>::epsilon()
                * std::max({scalar_t{1}, std::abs(point.a[0]), std::abs(point.a[1]),
                    x_span, y_span});
            const auto matches = [&](scalar_t t) {
                return t >= 0 && t <= 1
                    && std::abs(evaluate(curve.x, t) - point.a[0]) <= tolerance
                    && std::abs(evaluate(curve.y, t) - point.a[1]) <= tolerance;
            };
            if (matches(0) || matches(1) || std::ranges::any_of(roots, matches)) {
                return true;
            }
        }
        return false;
    }

    bool contains(const vector_t& point) const {
        int winding = 0;
        for (const auto& curve : curves) {
            // Monotone arcs plus half-open Y intervals count shared endpoints
            // once and make tangential contacts cancel instead of toggling fill.
            for (std::size_t i = 1; i < curve.y_splits.size(); ++i) {
                scalar_t lo = curve.y_splits[i - 1];
                scalar_t hi = curve.y_splits[i];
                const scalar_t y0 = evaluate(curve.y, lo);
                const scalar_t y1 = evaluate(curve.y, hi);
                const bool rising = y1 > y0;
                if (!(std::min(y0, y1) <= point.a[1] && point.a[1] < std::max(y0, y1))) {
                    continue;
                }
                for (int iteration = 0; iteration < std::numeric_limits<scalar_t>::digits + 2;
                     ++iteration) {
                    const scalar_t mid = (lo + hi) / 2;
                    if (mid == lo || mid == hi) {
                        break;
                    }
                    if ((evaluate(curve.y, mid) < point.a[1]) == rising) {
                        lo = mid;
                    } else {
                        hi = mid;
                    }
                }
                if (evaluate(curve.x, (lo + hi) / 2) > point.a[0]) {
                    winding += rising ? 1 : -1;
                }
            }
        }
        return even_odd ? winding % 2 != 0 : winding != 0;
    }
};

struct text_library_t {
    FT_Library handle = nullptr;
    text_library_t() {
        if (const auto error = FT_Init_FreeType(&handle)) {
            throw std::runtime_error("Cannot initialize FreeType: " + std::to_string(error));
        }
    }
    ~text_library_t() { FT_Done_FreeType(handle); }
};

struct text_face_t {
    FT_Face handle = nullptr;
    text_face_t(FT_Library library, const std::filesystem::path& path) {
        if (const auto error = FT_New_Face(library, path.string().c_str(), 0, &handle)) {
            throw std::runtime_error("Cannot load font '" + path.string()
                + "': FreeType error " + std::to_string(error));
        }
    }
    ~text_face_t() { FT_Done_Face(handle); }
};

struct text_decomposer_t {
    text_glyph_t glyph{};
    vector_t current{0, 0, 0};
    vector_t offset;
    scalar_t factor;
    std::exception_ptr failure;
    vector_t lower{std::numeric_limits<scalar_t>::infinity(),
        std::numeric_limits<scalar_t>::infinity(), 0};
    vector_t upper{-std::numeric_limits<scalar_t>::infinity(),
        -std::numeric_limits<scalar_t>::infinity(), 0};

    vector_t point(const FT_Vector& value) const {
        return {offset.a[0] + static_cast<scalar_t>(value.x) * factor,
            offset.a[1] + static_cast<scalar_t>(value.y) * factor, 0};
    }

    void add(const std::array<vector_t, 4>& points, int degree) {
        text_curve_t curve{};
        for (int axis = 0; axis < 2; ++axis) {
            auto& p = axis == 0 ? curve.x : curve.y;
            const scalar_t a = points[0].a[axis];
            const scalar_t b = points[1].a[axis];
            const scalar_t c = points[2].a[axis];
            const scalar_t d = points[3].a[axis];
            p = degree == 1 ? polynomial_t{a, b - a, 0, 0}
                : degree == 2 ? polynomial_t{a, 2 * (b - a), a - 2 * b + c, 0}
                : polynomial_t{a, 3 * (b - a), 3 * (a - 2 * b + c), -a + 3 * b - 3 * c + d};
            for (int i = 0; i <= degree; ++i) {
                lower.a[axis] = std::min(lower.a[axis], points[i].a[axis]);
                upper.a[axis] = std::max(upper.a[axis], points[i].a[axis]);
            }
        }
        curve.y_splits = {0, 1};
        const auto extrema = boost::math::tools::quadratic_roots(
            3 * curve.y[3], 2 * curve.y[2], curve.y[1]);
        for (const auto t : {extrema.first, extrema.second}) {
            if (t > 0 && t < 1) {
                curve.y_splits.push_back(t);
            }
        }
        std::ranges::sort(curve.y_splits);
        const auto duplicate = std::unique(curve.y_splits.begin(), curve.y_splits.end());
        curve.y_splits.erase(duplicate, curve.y_splits.end());
        glyph.curves.push_back(std::move(curve));
        current = points[degree];
    }

    template<class Function>
    static int callback(void* user, Function function) noexcept {
        auto& self = *static_cast<text_decomposer_t*>(user);
        try {
            function(self);
            return 0;
        } catch (...) {
            // Allocation exceptions must not unwind through FreeType's C frames.
            self.failure = std::current_exception();
            return 1;
        }
    }

    static int move_to(const FT_Vector* to, void* user) {
        return callback(user, [&](auto& self) { self.current = self.point(*to); });
    }
    static int line_to(const FT_Vector* to, void* user) {
        return callback(user, [&](auto& self) {
            self.add({self.current, self.point(*to), {}, {}}, 1);
        });
    }
    static int conic_to(const FT_Vector* control, const FT_Vector* to, void* user) {
        return callback(user, [&](auto& self) {
            self.add({self.current, self.point(*control), self.point(*to), {}}, 2);
        });
    }
    static int cubic_to(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* user) {
        return callback(user, [&](auto& self) {
            self.add({self.current, self.point(*c1), self.point(*c2), self.point(*to)}, 3);
        });
    }
};

}

struct text_t::data_t {
    std::vector<text_glyph_t> glyphs;
    boost::geometry::index::rtree<std::pair<box3_t, std::size_t>,
        boost::geometry::index::quadratic<8>> index;
    box3_t box;
    scalar_t extrusion;
};

text_t::text_t(const std::filesystem::path& font, std::string_view text, scalar_t extrusion) {
    if (!std::isfinite(extrusion) || extrusion <= 0) {
        throw std::invalid_argument("Text extrusion must be finite and positive");
    }
    using utf8_iterator = boost::u8_to_u32_iterator<std::string_view::const_iterator, char32_t>;
    const std::u32string characters{
        utf8_iterator{text.begin(), text.begin(), text.end()},
        utf8_iterator{text.end(), text.begin(), text.end()}};
    text_library_t library;
    text_face_t face{library.handle, font};
    if (!FT_IS_SCALABLE(face.handle) || face.handle->units_per_EM == 0) {
        throw std::invalid_argument("Text requires a scalable outline font: " + font.string());
    }
    if (const auto error = FT_Select_Charmap(face.handle, FT_ENCODING_UNICODE)) {
        throw std::runtime_error("Font has no Unicode character map: " + font.string()
            + " (FreeType error " + std::to_string(error) + ")");
    }
    auto result = std::make_shared<data_t>();
    result->extrusion = extrusion;
    const scalar_t factor = scalar_t{1} / face.handle->units_per_EM;
    vector_t advance{0, 0, 0};
    FT_UInt previous = 0;
    std::vector<std::pair<box3_t, std::size_t>> boxes;
    for (const char32_t character : characters) {
        if (character == U'\n') {
            advance.a[0] = 0;
            advance.a[1] -= static_cast<scalar_t>(face.handle->height) * factor;
            previous = 0;
            continue;
        }
        const auto index = FT_Get_Char_Index(face.handle, character);
        if (index == 0) {
            throw std::invalid_argument("Font '" + font.string()
                + "' has no glyph for code point " + std::to_string(character));
        }
        if (previous && FT_HAS_KERNING(face.handle)) {
            FT_Vector kerning{};
            if (const auto error = FT_Get_Kerning(face.handle, previous, index,
                    FT_KERNING_UNSCALED, &kerning)) {
                throw std::runtime_error("Cannot read font kerning: " + std::to_string(error));
            }
            advance.a[0] += static_cast<scalar_t>(kerning.x) * factor;
            advance.a[1] += static_cast<scalar_t>(kerning.y) * factor;
        }
        if (const auto error = FT_Load_Glyph(face.handle, index,
                FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP)) {
            throw std::runtime_error("Cannot load font glyph: " + std::to_string(error));
        }
        auto& slot = *face.handle->glyph;
        if (slot.format != FT_GLYPH_FORMAT_OUTLINE) {
            throw std::invalid_argument("Text requires outline glyphs: " + font.string());
        }
        if (slot.outline.n_contours > 0) {
            text_decomposer_t builder{.offset = advance, .factor = factor};
            const FT_Outline_Funcs functions{text_decomposer_t::move_to,
                text_decomposer_t::line_to, text_decomposer_t::conic_to,
                text_decomposer_t::cubic_to, 0, 0};
            const auto error = FT_Outline_Decompose(&slot.outline, &functions, &builder);
            if (builder.failure) {
                std::rethrow_exception(builder.failure);
            }
            if (error) {
                throw std::runtime_error("Cannot decompose font outline: " + std::to_string(error));
            }
            builder.glyph.normal_sign =
                FT_Outline_Get_Orientation(&slot.outline) == FT_ORIENTATION_POSTSCRIPT ? -1 : 1;
            builder.glyph.even_odd = slot.outline.flags & FT_OUTLINE_EVEN_ODD_FILL;
            builder.upper.a[2] = extrusion;
            for (int axis = 0; axis < 3; ++axis) {
                const scalar_t padding = 16 * std::numeric_limits<scalar_t>::epsilon()
                    * std::max({scalar_t{1}, std::abs(builder.lower.a[axis]),
                        std::abs(builder.upper.a[axis])});
                builder.lower.a[axis] -= padding;
                builder.upper.a[axis] += padding;
            }
            builder.glyph.box = {{builder.lower.a[0], builder.lower.a[1], builder.lower.a[2]},
                {builder.upper.a[0], builder.upper.a[1], builder.upper.a[2]}};
            boxes.emplace_back(builder.glyph.box, result->glyphs.size());
            result->glyphs.push_back(std::move(builder.glyph));
        }
        advance.a[0] += static_cast<scalar_t>(slot.advance.x) * factor;
        previous = index;
    }
    if (boxes.empty()) {
        throw std::invalid_argument("Text requires at least one visible outline glyph");
    }
    result->box = boxes.front().first;
    for (const auto& [box, index] : boxes) {
        boost::geometry::expand(result->box, box);
    }
    result->index = decltype(result->index){boxes};
    data = std::move(result);
}

std::optional<box3_t> text_t::bounds() const {
    return data->box;
}

bool text_t::inside(const vector_t& point) const {
    validate_point(point);
    if (!(point.a[2] > 0 && point.a[2] < data->extrusion)) {
        return false;
    }
    const auto predicate = boost::geometry::index::intersects(
        point3_t{point.a[0], point.a[1], point.a[2]});
    for (auto it = data->index.qbegin(predicate); it != data->index.qend(); ++it) {
        const auto& glyph = data->glyphs[it->second];
        if (!glyph.on_boundary(point) && glyph.contains(point)) {
            return true;
        }
    }
    return false;
}

std::size_t text_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    if (boost::qvm::mag_sqr(ray.direction) == 0) {
        return 0;
    }
    std::size_t count = 0;
    const auto keep = [&](scalar_t distance, const vector_t& normal) {
        if (ray[distance]) {
            append_hit(hits, count, hit_t{.point = ray(distance), .normal = normal, .distance = distance});
        }
    };
    const auto predicate = boost::geometry::index::intersects(ray.segment());
    for (auto it = data->index.qbegin(predicate); it != data->index.qend(); ++it) {
        const auto& glyph = data->glyphs[it->second];
        if (ray.direction.a[2] != 0) {
            for (int cap = 0; cap < 2; ++cap) {
                const scalar_t distance = (cap * data->extrusion - ray.origin.a[2])
                    / ray.direction.a[2];
                if (ray[distance] && glyph.contains(ray(distance))) {
                    keep(distance, vector_t{0, 0, cap == 0 ? scalar_t{-1} : scalar_t{1}});
                }
            }
        }
        const scalar_t dx = ray.direction.a[0];
        const scalar_t dy = ray.direction.a[1];
        if (dx == 0 && dy == 0) {
            continue;
        }
        for (const auto& curve : glyph.curves) {
            polynomial_t p{};
            for (int i = 0; i < 4; ++i) {
                p[i] = dy * (curve.x[i] - (i == 0 ? ray.origin.a[0] : 0))
                    - dx * (curve.y[i] - (i == 0 ? ray.origin.a[1] : 0));
            }
            // A ray collinear with an outline segment has no isolated side hit.
            if (std::ranges::all_of(p, [](scalar_t value) { return value == 0; })) {
                continue;
            }
            const auto roots = boost::math::tools::cubic_roots(p[3], p[2], p[1], p[0]);
            for (const scalar_t t : roots) {
                if (!(t >= 0 && t <= 1)) {
                    continue;
                }
                const scalar_t distance = std::abs(dx) > std::abs(dy)
                    ? (evaluate(curve.x, t) - ray.origin.a[0]) / dx
                    : (evaluate(curve.y, t) - ray.origin.a[1]) / dy;
                if (!ray[distance]) {
                    continue;
                }
                const scalar_t z = ray(distance).a[2];
                if (z < 0 || z > data->extrusion) {
                    continue;
                }
                const scalar_t tx = (3 * curve.x[3] * t + 2 * curve.x[2]) * t + curve.x[1];
                const scalar_t ty = (3 * curve.y[3] * t + 2 * curve.y[2]) * t + curve.y[1];
                const vector_t normal{-ty * glyph.normal_sign, tx * glyph.normal_sign, 0};
                if (boost::qvm::mag_sqr(normal) != 0) {
                    keep(distance, boost::qvm::normalized(normal));
                }
            }
        }
    }
    return finish_hits(hits, count);
}

}
