#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <boost/geometry/core/access.hpp>
#include <boost/qvm.hpp>

#include "../support.hpp"

namespace rt2::tests {
namespace {

struct cubic_font_t {
    std::filesystem::path directory;
    std::filesystem::path file;

    cubic_font_t() {
        const auto id = std::chrono::steady_clock::now().time_since_epoch().count();
        directory = std::filesystem::temp_directory_path() / ("rt2-text-" + std::to_string(id));
        check(std::filesystem::create_directory(directory), "Create text fixture directory");
        file = directory / "cubic.cff";
        // Self-authored CFF: glyph A is a baseline and one cubic arch with
        // controls (1,0), (1,1), (0,1), (0,0), in em units. No third-party font data.
        constexpr std::string_view bytes{
            "\x01\x00\x04\x04\x00\x01\x01\x01\x0a\x43\x75\x62\x69\x63\x54\x65"
            "\x73\x74\x00\x01\x01\x01\x14\x8b\x8b\xfa\x7c\xfa\x7c\x05\x1d\x00"
            "\x00\x00\x2e\x0f\x1d\x00\x00\x00\x31\x11\x00\x00\x00\x00\x00\x00"
            "\x22\x00\x02\x01\x01\x02\x16\x0e\xfa\x7c\x8b\x8b\x15\xfa\x7c\x8b"
            "\x05\x8b\xfa\x7c\xfe\x7c\x8b\x8b\xfe\x7c\x08\x0e", 76};
        std::ofstream stream{file, std::ios::binary};
        stream.exceptions(std::ios::badbit | std::ios::failbit);
        stream.write(bytes.data(), bytes.size());
        stream.close();
    }

    ~cubic_font_t() {
        std::error_code error;
        std::filesystem::remove(file, error);
        if (error) {
            std::fprintf(stderr, "Cannot remove cubic font fixture: %s\n", error.message().c_str());
        }
        std::filesystem::remove(directory, error);
        if (error) {
            std::fprintf(stderr, "Cannot remove text fixture directory: %s\n", error.message().c_str());
        }
    }
};

}

void test_text() {
    const std::filesystem::path font{RT2_FONT_FILE};
    const text_t h{font, "H", 0.25f};
    const vector_t stem{0.15f, 0.2f, 0.125f};
    check(h.bounds().has_value(), "Text bounds");
    check(h.inside(stem), "H stem interior");
    check(!h.inside({0.4f, 0.2f, 0.125f}), "H opening");
    check(!h.inside({0.15f, 0.2f, -0.1f})
        && !h.inside({0.15f, 0.2f, 0.3f}), "Text extrusion limits");
    check(!h.inside({0.15f, 0.2f, 0})
        && !h.inside({0.15f, 0.2f, 0.25f}), "Text cap boundary convention");
    const auto front = h.intersect(ray({0.15f, 0.2f, -1}, {0, 0, 1}));
    check(front.has_value(), "Text front cap");
    close(front->distance, 1, "Front distance");
    close(front->normal.a[2], -1, "Front outward normal");
    const auto back = h.intersect(ray({0.15f, 0.2f, 1}, {0, 0, -1}));
    check(back.has_value(), "Text back cap");
    close(back->distance, 0.75f, "Back distance");
    close(back->normal.a[2], 1, "Back outward normal");
    const auto exit = h.intersect(ray(stem, {0, 0, 2}));
    check(exit.has_value(), "Inside ray exits text");
    close(exit->distance, 0.0625f, "Non-unit text ray");
    check(!h.intersect(ray({0.15f, 0.2f, -1}, {0, 0, 1}, 0, 0.9f)),
        "Text finite ray limit");
    check(!h.intersect(ray(stem, {0, 0, 0})), "Text zero ray");
    const auto beyond_front = h.intersect(ray({0.15f, 0.2f, -1}, {0, 0, 1}, 1.1f));
    check(beyond_front.has_value(), "Text skips cap before ray minimum");
    close(beyond_front->distance, 1.25f, "Text exit beyond ray minimum");

    const auto side = h.intersect(ray({-1, 0.2f, 0.125f}, {1, 0, 0}));
    check(side.has_value(), "Linear outline side");
    close(side->normal.a[0], -1, "Linear side outward normal");
    check(!h.intersect(ray({-1, 0.2f, 0.3f}, {1, 0, 0})), "Side beyond extrusion");
    const text_t o{font, "O", 0.25f};
    check(!o.inside({0.39f, 0.36f, 0.125f}), "Curved glyph hole");
    check(!o.intersect(ray({0.39f, 0.36f, -1}, {0, 0, 1})), "Cap preserves hole");
    const auto outer = o.intersect(ray({-1, 0.36f, 0.125f}, {1, 0, 0}));
    check(outer.has_value(), "Quadratic outline side");
    check(outer->normal.a[0] < -0.99f, "Curve outer normal");
    const auto inner = o.intersect(ray({0.39f, 0.36f, 0.125f}, {1, 0, 0}));
    check(inner.has_value() && inner->normal.a[0] < -0.99f, "Hole side normal");
    const scalar_t ring_x = outer->point.a[0] + 0.04f;
    check(o.inside({ring_x, 0.36f, 0.125f}), "Curved glyph filled ring");
    check(o.intersect(ray({ring_x, 0.36f, -1}, {0, 0, 1})).has_value(),
        "Curved glyph cap");
    // At a contour's extremal Y, touching it must not fill the exterior.
    const auto top = boost::geometry::get<boost::geometry::max_corner, 1>(*o.bounds());
    check(!o.inside({-0.1f, top, 0.125f}), "No tangent crossing fill");

    const text_t spaced{font, "H H", 0.25f};
    check(spaced.inside(stem), "Space keeps first glyph");
    const scalar_t width = boost::geometry::get<boost::geometry::max_corner, 0>(*h.bounds())
        - boost::geometry::get<boost::geometry::min_corner, 0>(*h.bounds());
    check(!spaced.inside({width + 0.2f, 0.2f, 0.125f}), "Space has no geometry");
    const text_t lines{font, "H\nH", 0.25f};
    check(boost::geometry::get<boost::geometry::min_corner, 1>(*lines.bounds()) < -1,
        "Newline advances baseline");
    const text_t question{font, "?", 0.25f};
    check(question.bounds().has_value(), "Question mark is not old newline sentinel");
    const text_t unicode{font, "\xc3\x84\xc3\xb6", 0.25f};
    check(unicode.bounds().has_value(), "UTF-8 glyphs and composite outlines");
    const text_t kerned{font, "AV", 0.25f};
    const text_t separated{font, "A V", 0.25f};
    check(boost::geometry::get<boost::geometry::max_corner, 0>(*kerned.bounds())
        < boost::geometry::get<boost::geometry::max_corner, 0>(*separated.bounds()),
        "Kerning and advance");
    const text_t many{font, "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH", 0.25f};
    check(many.intersect(ray({0.15f, 0.2f, -1}, {0, 0, 1})).has_value(),
        "Multi-node glyph R-tree");

    const cubic_font_t fixture;
    const text_t cubic{fixture.file, "A", 0.25f};
    check(cubic.inside({0.5f, 0.5625f, 0.125f}), "Cubic fill");
    check(!cubic.inside({0.1f, 0.5625f, 0.125f}), "Outside cubic arch");
    const auto cubic_side = cubic.intersect(
        ray({-1.84375f, 1.0625f, 0.125f}, {1, -0.25f, 0}));
    check(cubic_side.has_value(), "Cubic polynomial side intersection");
    close(cubic_side->distance, 2, "Cubic side distance");
    close(cubic_side->normal.a[0], -0.8f, "PostScript cubic normal X");
    close(cubic_side->normal.a[1], 0.6f, "PostScript cubic normal Y");
    const auto tangent = cubic.intersect(ray({-1, 0.75f, 0.125f}, {1, 0, 0}));
    check(tangent.has_value(), "Tangent curve intersection");
    close(tangent->distance, 1.5f, "Cubic tangent distance");
    close(tangent->normal.a[1], 1, "Cubic tangent normal");
    check(!cubic.inside({-0.1f, 0.75f, 0.125f}), "Tangent does not change winding");
    const auto cubic_cap = cubic.intersect(ray({0.5f, 0.5625f, -1}, {0, 0, 1}));
    check(cubic_cap.has_value(), "Cubic cap fill");

    object_t object{.surface = surface_t{h},
        .shader = shader_t{phong_t{
            .ambient = {1, 0, 0}, .diffuse = {0, 0, 0}, .specular = {0, 0, 0}}}};
    object.scale({2, 3, 4}).rotate_y(std::acos(scalar_t{-1}) / 2).translate({2, 0, 0});
    const auto incoming = ray({4, 0.6f, -0.3f}, {-1, 0, 0});
    const auto transformed = object.intersect(incoming);
    check(transformed.has_value() && transformed->object == &object, "Transformed text object");
    close(transformed->distance, 1, "Transformed text distance");
    close(transformed->normal.a[0], 1, "Transformed cap normal");
    auto copy = object;
    copy.translate({0, 2, 0});
    check(object.intersect(incoming).has_value() && !copy.intersect(incoming),
        "Shared text has independent instance transforms");
    const scene_t scene{.objects = {object, copy}};
    check(scene.occluded(incoming), "Scene and glyph R-trees");
    close(scene.trace(incoming, 0).a[0], 1, "Text shading");

    const auto rejects = [&](std::string_view text, scalar_t depth) {
        try {
            const text_t invalid{font, text, depth};
        } catch (const std::invalid_argument&) {
            return;
        } catch (const std::out_of_range&) {
            return;
        }
        throw std::runtime_error("Invalid text accepted");
    };
    rejects("", 0.25f);
    rejects(" \n ", 0.25f);
    rejects("H", 0);
    rejects("H", -1);
    rejects("H", std::numeric_limits<scalar_t>::infinity());
    rejects("H", std::numeric_limits<scalar_t>::quiet_NaN());
    rejects("\xc3", 0.25f);
    rejects("\xc0\xaf", 0.25f);
    rejects("\xf4\x8f\xbf\xbf", 0.25f);
    try {
        const text_t missing{font / "missing.ttf", "H", 0.25f};
        throw std::logic_error("Missing font accepted");
    } catch (const std::runtime_error&) {
    }
}

}
