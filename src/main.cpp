#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <numbers>
#include <print>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>
#include <boost/qvm.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/gil.hpp>
#include <boost/gil/extension/io/png.hpp>

import model;
import renderer;

namespace bg = boost::gil;
using namespace rt2::model;
using namespace rt2::renderer;

using image_t = bg::rgb8_image_t;

namespace {

scene_t make_csg_scene(const primitive_t& floor) {
    constexpr scalar_t pi = std::numbers::pi_v<scalar_t>;
    const auto material = [](const vector_t& color) {
        return shader_t{phong_t{
            .ambient = scalar_t{0.035f} * color,
            .diffuse = color,
            .specular = {0.65f, 0.65f, 0.65f}, .shininess = 64}};
    };
    const auto orange = material({0.85f, 0.32f, 0.07f});
    const auto cyan = material({0.06f, 0.55f, 0.75f});
    const auto green = material({0.18f, 0.65f, 0.25f});
    const auto coral = material({0.8f, 0.12f, 0.2f});

    constexpr std::size_t slices = 48;
    constexpr std::size_t stacks = 24;
    std::vector<vector_t> vertices{{0, 1.5f, 0}};
    std::vector<mesh_t::face_t> faces;
    for (std::size_t stack = 1; stack < stacks; ++stack) {
        const scalar_t theta = pi * static_cast<scalar_t>(stack) / stacks;
        for (std::size_t slice = 0; slice < slices; ++slice) {
            const scalar_t phi = 2 * pi * static_cast<scalar_t>(slice) / slices;
            const scalar_t radius = std::sin(theta)
                * (1 + scalar_t{0.18f} * std::cos(6 * phi + 2 * theta));
            vertices.push_back({scalar_t{1.3f} * radius * std::cos(phi),
                scalar_t{1.5f} * std::cos(theta), radius * std::sin(phi)});
        }
    }
    const auto bottom = vertices.size();
    vertices.push_back({0, -1.5f, 0});
    for (std::size_t slice = 0; slice < slices; ++slice) {
        const auto next = (slice + 1) % slices;
        faces.push_back({0, 1 + next, 1 + slice});
        for (std::size_t stack = 0; stack < stacks - 2; ++stack) {
            const auto a = 1 + stack * slices + slice;
            const auto b = 1 + stack * slices + next;
            faces.push_back({a, b, a + slices});
            faces.push_back({b, b + slices, a + slices});
        }
        faces.push_back({bottom, 1 + (stacks - 2) * slices + slice,
            1 + (stacks - 2) * slices + next});
    }
    const auto mesh = primitive_t{
        .surface = surface_t{mesh_t{std::move(vertices), std::move(faces)}},
        .shader = orange};
    const auto sor = primitive_t{
        .surface = surface_t{sor_t{{{0, 0.85f}, {0.25f, 1.05f},
            {1, 1.2f}, {1.8f, 0.65f}, {2.4f, 0.5f}, {2.8f, 0.85f}}}},
        .shader = green
    }.rotate_x(-pi / 2).translate({0, -1.4f, 0});
    const auto torus = primitive_t{
        .surface = surface_t{torus_t{0.95f * 0.95f, 0.32f * 0.32f}}, .shader = cyan};
    const auto text = primitive_t{
        .surface = surface_t{text_t{RT2_FONT_FILE, "CSG", 0.8f}},
        .shader = coral
    }.scale({1.3f, 1.3f, 1}).translate({-1.4f, -0.5f, -0.4f});

    objects_t objects{floor};
    const auto exhibit = [&](object_t object, scalar_t x, scalar_t z,
                             std::string_view caption) {
        const auto bounds = *object.bounds();
        const vector_t low{
            boost::geometry::get<boost::geometry::min_corner, 0>(bounds),
            boost::geometry::get<boost::geometry::min_corner, 1>(bounds),
            boost::geometry::get<boost::geometry::min_corner, 2>(bounds)};
        const vector_t high{
            boost::geometry::get<boost::geometry::max_corner, 0>(bounds),
            boost::geometry::get<boost::geometry::max_corner, 1>(bounds),
            boost::geometry::get<boost::geometry::max_corner, 2>(bounds)};
        const scalar_t scale = 3 / std::max({high.a[0] - low.a[0],
            high.a[1] - low.a[1], high.a[2] - low.a[2]});
        object.translate({-(low.a[0] + high.a[0]) / 2, -low.a[1],
                -(low.a[2] + high.a[2]) / 2})
            .scale({scale, scale, scale}).translate({x, -2, z});
        objects.push_back(std::move(object));
        auto label = primitive_t{
            .surface = surface_t{text_t{RT2_FONT_FILE, caption, 0.025f}},
            .shader = material({0.75f, 0.8f, 0.9f})};
        const auto label_bounds = *label.scene_bounds;
        const scalar_t center = (
            boost::geometry::get<boost::geometry::min_corner, 0>(label_bounds)
            + boost::geometry::get<boost::geometry::max_corner, 0>(label_bounds)) / 2;
        label.translate({-center, 0, 0}).scale({0.38f, 0.38f, 0.38f})
            .rotate_x(-pi / 2).translate({x, -1.98f, z + 2});
        objects.push_back(std::move(label));
    };
    auto groove = torus;
    groove.translate({0, 0, 0.55f});
    exhibit(difference_t{mesh, groove}, -4.2f, -5, "mesh - torus");
    auto lettering_cutter = text;
    lettering_cutter.scale({0.75f, 1, 4});
    exhibit(difference_t{sor, lettering_cutter}, 0, -5, "sor - text");
    auto thick_ring = primitive_t{
        .surface = surface_t{torus_t{1, 0.65f * 0.65f}}, .shader = cyan};
    thick_ring.rotate_x(pi / 10);
    exhibit(intersection_t{text, thick_ring}, 4.2f, -5, "text & torus");
    auto mesh_clip = sor;
    mesh_clip.translate({0.6f, 0, 0.3f});
    exhibit(intersection_t{mesh, mesh_clip}, -4.2f, 3, "mesh & sor");
    auto collar = torus;
    collar.rotate_x(pi / 2).translate({0, 0.65f, 0});
    exhibit(union_t{sor, collar}, 0, 3, "sor | torus");
    auto text_ring = torus;
    text_ring.translate({-0.9f, 0, -0.15f});
    exhibit(union_t{text, text_ring}, 4.2f, 3, "text | torus");

    return scene_t{
        .objects = std::move(objects),
        .lights = {
            light_t{.origin = {-6, 9, 9}, .intensity = {0.85f, 0.78f, 0.68f}},
            light_t{.origin = {7, 6, 4}, .intensity = {0.35f, 0.5f, 0.75f}},
            light_t{.origin = {0, 7, -7}, .intensity = {0.55f, 0.65f, 0.8f}}},
        .camera = camera_t{
            .origin = {0, 14, 18}, .direction = {0, -0.9f, -1},
            .up = {0, 1, 0}, .fov = 30}};
}

}

// static_assert(Intersectable<sphere_t>);
// static_assert(Intersectable<torus_t>);
// static_assert(Intersectable<surface_t>);

// static_assert(Intersectable<primitive_t>);
// static_assert(Transformable<primitive_t>);

// static_assert(Intersectable<scene_t>);

int main(int argc, char** argv) try {
    constexpr scalar_t quarter_turn = std::numbers::pi_v<scalar_t> / 2;
    bool many_objects = false;
    bool csg_scene = false;
    bool benchmark = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument{argv[i]};
        if (argument == "--many") {
            many_objects = true;
        } else if (argument == "--csg") {
            csg_scene = true;
        } else if (argument == "--benchmark") {
            benchmark = true;
        } else if (argument == "--help") {
            std::println("Usage: rt2 [--many | --csg] [--benchmark]\n"
                "  --many       Render a second scene with 10,000 bounded objects and one plane.\n"
                "  --csg        Render a CSG gallery of mesh, SOR, text and torus solids.\n"
                "  --benchmark  Warm up, time five renders, print median and pixel checksum;\n"
                "               skip PNG output. Scene construction is not timed.");
            return 0;
        } else {
            throw std::invalid_argument("Unknown argument; use --help for usage");
        }
    }
    if (many_objects && csg_scene) {
        throw std::invalid_argument("--many and --csg select different scenes; use only one");
    }
    const auto object1 = primitive_t{
        .surface = surface_t{sphere_t{}},
        .shader = shader_t{dielectric_t{.refractive_index = 1.5f}}
    }
    .scale({1.35f, 1.35f, 1.35f})
    .translate({0.7f, -0.65f, 0.5f});

    const auto object2 = primitive_t {
        .surface = surface_t{torus_t{4.f, 0.25f}},
        .shader = shader_t{metal_t{.reflectance = {0.92f, 0.68f, 0.32f}}}
    }
    .rotate_x(quarter_turn / 6)
    .rotate_y(-quarter_turn / 5)
    .rotate_z(quarter_turn / 8)
    .translate({-2.6f, 0.5f, -3.0f});

    const auto make_sphere = [](const vector_t& origin, scalar_t radius, const shader_t& shader) {
        auto sphere = primitive_t{
            .surface = surface_t{sphere_t{}},
            .shader = shader
        };
        sphere.scale({radius, radius, radius}).translate(origin);
        return sphere;
    };

    const auto copper = make_sphere({-4.8f, -0.9f, 2.5f}, 1.1f,
        shader_t{metal_t{.reflectance = {0.95f, 0.48f, 0.28f}}});
    const auto blue = make_sphere({0, 0, 0}, 1.65f,
        shader_t{phong_t{
            .ambient = {0.008f, 0.018f, 0.035f},
            .diffuse = {0.035f, 0.18f, 0.38f},
            .specular = {0.5f, 0.6f, 0.7f},
            .shininess = 80
        }}).scale({1.4f, 0.6f, 1})
        .translate({0.3f, 0.9f, -4.6f});
    const auto coral = make_sphere({0, 0, 0}, 0.65f,
        shader_t{phong_t{
            .ambient = {0.025f, 0.006f, 0.008f},
            .diffuse = {0.55f, 0.055f, 0.075f},
            .specular = {0.35f, 0.25f, 0.25f},
            .shininess = 48
        }}).scale({0.6f, 1.4f, 1})
        .translate({-3.0f, -2 + 0.65f * 1.4f, 1.0f});
    const auto silver = make_sphere({2.7f, -1.5f, 1.5f}, 0.5f,
        shader_t{metal_t{.reflectance = {0.88f, 0.92f, 0.98f}}});

    const auto make_sor = [quarter_turn](const sor_t& surface, const vector_t& origin, const shader_t& shader) {
        auto object = primitive_t{
            .surface = surface_t{surface},
            .shader = shader
        };
        object.rotate_x(-quarter_turn).translate(origin);
        return object;
    };
    const auto vase = make_sor(
        sor_t{{{0, 0.55f}, {0.3f, 0.8f}, {1.1f, 0.9f},
               {1.8f, 0.4f}, {2.5f, 0.3f}, {2.8f, 0.55f}}},
        {-4.8f, -2, -1.0f},
        shader_t{phong_t{
            .ambient = {0.015f, 0.025f, 0.008f},
            .diffuse = {0.25f, 0.45f, 0.08f},
            .specular = {0.65f, 0.65f, 0.65f},
            .shininess = 64
        }});
    const auto spindle = make_sor(
        sor_t{{{0, 0.6f}, {0.25f, 0.7f}, {0.9f, 0.25f},
               {1.6f, 0.55f}, {2.2f, 0.2f}, {2.5f, 0.1f}}},
        {0, 0, 0},
        shader_t{metal_t{.reflectance = {0.8f, 0.85f, 0.92f}}})
        .scale({1.6f, 1.6f, 1.6f}).translate({3.5f, -2, -2.8f});

    const auto gem = primitive_t{
        .surface = surface_t{mesh_t{
            {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}},
            {{2, 4, 0}, {2, 1, 4}, {2, 5, 1}, {2, 0, 5},
             {3, 0, 4}, {3, 4, 1}, {3, 1, 5}, {3, 5, 0}}}},
        .shader = shader_t{phong_t{
            .ambient = {0.025f, 0.01f, 0.035f},
            .diffuse = {0.4f, 0.08f, 0.55f},
            .specular = {0.6f, 0.6f, 0.6f}, .shininess = 64}}
    }.scale({0.8f, 1.2f, 0.8f}).rotate_y(quarter_turn / 3)
        .translate({-1.3f, -0.8f, 2.5f});

    const auto text = primitive_t{
        .surface = surface_t{text_t{RT2_FONT_FILE, "rt2", 0.18f}},
        .shader = shader_t{phong_t{
            .ambient = {0.025f, 0.03f, 0.035f},
            .diffuse = {0.55f, 0.7f, 0.85f},
            .specular = {0.8f, 0.8f, 0.8f}, .shininess = 80}}
    }.scale({1.8f, 1.8f, 1.8f}).rotate_y(-quarter_turn / 8)
        .translate({-1.4f, 3.1f, -5});

    const auto make_wall = [](const vector_t& origin, const vector_t& normal, const vector_t& color) {
        const vector_t reference = normal.a[1] == 0
            ? vector_t{0, 1, 0} : vector_t{1, 0, 0};
        const vector_t right = boost::qvm::normalized(boost::qvm::cross(reference, normal));
        const vector_t up = boost::qvm::cross(normal, right);
        auto wall = primitive_t{
            .surface = surface_t{plane_t{}},
            .shader = shader_t{phong_t{
                .ambient = 0.05f * color,
                .diffuse = color,
                .specular = {0, 0, 0},
                .shininess = 1
            }},
            .scene_object = matrix_t{{
                {right.a[0], up.a[0], normal.a[0], 0},
                {right.a[1], up.a[1], normal.a[1], 0},
                {right.a[2], up.a[2], normal.a[2], 0},
                {0, 0, 0, 1}
            }}
        };
        wall.translate(origin);
        return wall;
    };

    const auto floor = make_wall({0, -2, 0}, {0, 1, 0}, {0.10f, 0.13f, 0.17f});
    const auto ceiling = make_wall({0, 7, 0}, {0, -1, 0}, {0.12f, 0.15f, 0.20f});
    const auto back = make_wall({0, 0, -8}, {0, 0, 1}, {0.045f, 0.075f, 0.13f});
    const auto left = make_wall({-8, 0, 0}, {1, 0, 0}, {0.20f, 0.09f, 0.07f});
    const auto right = make_wall({8, 0, 0}, {-1, 0, 0}, {0.055f, 0.16f, 0.18f});

    const auto light1 = light_t{
        .origin = {-4.0f, 5.5f, 4.0f},
        .intensity = {0.85f, 0.68f, 0.50f}
    };
    const auto light2 = light_t{
        .origin = {5.0f, 3.5f, 1.0f},
        .intensity = {0.25f, 0.48f, 0.75f}
    };
    const auto light3 = light_t{
        .origin = {0.0f, 4.5f, -6.0f},
        .intensity = {0.55f, 0.65f, 0.85f}
    };

    const auto csg_sphere = make_sphere({0, 0, 0}, 1,
        shader_t{phong_t{
            .ambient = {0.025f, 0.012f, 0.005f},
            .diffuse = {0.8f, 0.3f, 0.06f},
            .specular = {0.7f, 0.7f, 0.7f}, .shininess = 64}});
    auto cutter = make_sphere({0.35f, 0.25f, 0.65f}, 0.85f, blue.shader);
    const object_t difference_template{difference_t{csg_sphere, cutter}};
    const object_t intersection_template{intersection_t{
        make_sphere({-0.45f, 0, 0}, 1, coral.shader),
        make_sphere({0.45f, 0, 0}, 1, coral.shader)}};
    const object_t union_template{union_t{
        make_sphere({-0.45f, 0, 0}, 0.8f, vase.shader),
        make_sphere({0.45f, 0, 0}, 0.8f, vase.shader)}};

    auto scene = scene_t{
        .objects = {object1, object2, copper, blue, coral, silver, vase, spindle, gem, text,
                    floor, ceiling, back, left, right},
        .lights = {light1, light2, light3},
        .camera = camera_t{
            .origin = {0.0f, 3.0f, 12.0f},
            .direction = {0.0f, -0.20f, -1.0f},
            .up = {0.0f, 1.0f, 0.0f},
            .fov = 48.0f
        }
    };

    if (csg_scene) {
        scene = make_csg_scene(floor);
    } else {
        const auto load_dragon = [&](const std::filesystem::path &relative_path,
                                     const shader_t &shader, bool z_up) {
            const auto file = std::filesystem::path{RT2_MESH_DIRECTORY} / relative_path;
            std::println("Loading dragon: {}", file.string());
            const auto start = std::chrono::steady_clock::now();
            auto dragon = primitive_t{.surface = surface_t{read_ply(file)}, .shader = shader};
            if (z_up) {
                dragon.rotate_x(-quarter_turn);
            }
            const auto bounds = *dragon.scene_bounds;
            const scalar_t min_x = boost::geometry::get<boost::geometry::min_corner, 0>(bounds);
            const scalar_t min_y = boost::geometry::get<boost::geometry::min_corner, 1>(bounds);
            const scalar_t min_z = boost::geometry::get<boost::geometry::min_corner, 2>(bounds);
            const scalar_t max_x = boost::geometry::get<boost::geometry::max_corner, 0>(bounds);
            const scalar_t max_y = boost::geometry::get<boost::geometry::max_corner, 1>(bounds);
            const scalar_t max_z = boost::geometry::get<boost::geometry::max_corner, 2>(bounds);
            const scalar_t scale = scalar_t{3.2f} / (max_y - min_y);
            dragon.translate({-(min_x + max_x) / 2, -min_y, -(min_z + max_z) / 2})
                .scale({scale, scale, scale});
            std::println("Dragon loaded and indexed in {} ms",
                         std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count());
            return dragon;
        };
        const auto dragon_template =
            load_dragon("fantasy-dragon-ply/Fantasy Dragon.ply",
                        shader_t{phong_t{.ambient = {0.025f, 0.018f, 0.006f},
                                         .diffuse = {0.65f, 0.38f, 0.08f},
                                         .specular = {0.7f, 0.6f, 0.4f},
                                         .shininess = 64}},
                        false);
        const auto new_dragon_template =
            load_dragon("dragon-ply/Dragon-ply.ply",
                        shader_t{phong_t{.ambient = {0.008f, 0.025f, 0.02f},
                                         .diffuse = {0.08f, 0.45f, 0.3f},
                                         .specular = {0.6f, 0.8f, 0.7f},
                                         .shininess = 80}},
                        true);

        if (!many_objects) {
            auto dragon = dragon_template;
            dragon.rotate_y(-quarter_turn / 3).translate({4.7f, -2, 2.5f});
            scene.objects.push_back(std::move(dragon));
            auto new_dragon = new_dragon_template;
            new_dragon.scale({0.75f, 0.75f, 0.75f})
                .rotate_y(quarter_turn / 3)
                .translate({2.5f, -2, 3.8f});
            scene.objects.push_back(std::move(new_dragon));
            auto difference = difference_template;
            difference.scale({0.65f, 0.65f, 0.65f}).translate({-2.4f, -1.35f, 3.7f});
            scene.objects.push_back(std::move(difference));
            auto intersection = intersection_template;
            intersection.scale({0.7f, 0.7f, 0.7f})
                .rotate_y(quarter_turn / 3)
                .translate({-0.6f, -1.375f, 3.7f});
            scene.objects.push_back(std::move(intersection));
            auto united = union_template;
            united.scale({0.6f, 0.6f, 0.6f})
                .rotate_y(quarter_turn / 4)
                .translate({1.1f, -1.52f, 3.7f});
            scene.objects.push_back(std::move(united));
            scene.rebuild_index();
        }

        if (many_objects) {
            constexpr int grid_size = 100;
            constexpr scalar_t grid_center = scalar_t{grid_size - 1} / 2;
            objects_t objects;
            objects.reserve(grid_size * grid_size + 1);
            for (int row = 0; row < grid_size; ++row) {
                for (int column = 0; column < grid_size; ++column) {
                    const vector_t origin{(static_cast<scalar_t>(column) - grid_center) * 2.2f, -2,
                                          (static_cast<scalar_t>(row) - grid_center) * 2.2f};
                    const int kind = (row * grid_size + column) % 10;
                    if (kind == 0) {
                        auto object = make_sor(sor_t{{{0, 0.55f},
                                                      {0.3f, 0.8f},
                                                      {1.1f, 0.9f},
                                                      {1.8f, 0.4f},
                                                      {2.5f, 0.3f},
                                                      {2.8f, 0.55f}}},
                                               {0, 0, 0}, vase.shader);
                        object.scale({0.7f, 0.7f, 0.7f}).translate(origin);
                        objects.push_back(object);
                    } else if (kind == 1) {
                        auto object = primitive_t{.surface = surface_t{torus_t{4, 0.25f}},
                                                  .shader = object2.shader};
                        object.scale({0.4f, 0.4f, 0.4f})
                            .rotate_x(quarter_turn * static_cast<scalar_t>(row % 5) / 4)
                            .rotate_y(quarter_turn * static_cast<scalar_t>(row % 3) / 3)
                            .rotate_z(quarter_turn * static_cast<scalar_t>(row % 4) / 4)
                            .translate(origin + vector_t{0, 1, 0});
                        objects.push_back(object);
                    } else if (kind == 2) {
                        auto dragon = dragon_template;
                        dragon.scale({0.55f, 0.55f, 0.55f})
                            .rotate_y(quarter_turn * static_cast<scalar_t>(row % 8) / 2)
                            .translate(origin);
                        objects.push_back(std::move(dragon));
                    } else if (kind == 3) {
                        auto label = primitive_t{.surface = text.surface, .shader = text.shader};
                        label.translate({-0.7f, 0, 0})
                            .scale({1.2f, 1.2f, 1.2f})
                            .rotate_x(-quarter_turn / 4)
                            .rotate_y(quarter_turn * static_cast<scalar_t>(row % 5 - 2) / 8)
                            .translate(origin);
                        objects.push_back(std::move(label));
                    } else if (kind == 4) {
                        auto dragon = new_dragon_template;
                        dragon.scale({0.55f, 0.55f, 0.55f})
                            .rotate_y(quarter_turn * static_cast<scalar_t>(row % 8) / 2)
                            .translate(origin);
                        objects.push_back(std::move(dragon));
                    } else if (kind >= 5 && kind <= 7) {
                        auto csg = kind == 5   ? difference_template
                                   : kind == 6 ? intersection_template
                                               : union_template;
                        const scalar_t height = kind == 6   ? scalar_t{0.625f}
                                                : kind == 7 ? scalar_t{0.56f}
                                                            : scalar_t{0.7f};
                        csg.scale({0.7f, 0.7f, 0.7f})
                            .rotate_y(quarter_turn * static_cast<scalar_t>(row % 8) / 4)
                            .translate(origin + vector_t{0, height, 0});
                        objects.push_back(std::move(csg));
                    } else {
                        const auto &shader = kind % 3 == 0   ? copper.shader
                                             : kind % 3 == 1 ? blue.shader
                                                             : coral.shader;
                        const scalar_t phase = quarter_turn * static_cast<scalar_t>(row) / 2;
                        const scalar_t strain =
                            kind % 3 == 0 ? scalar_t{0} : scalar_t{0.4f} * std::sin(phase);
                        const scalar_t scale_y = 1 - strain;
                        auto sphere = make_sphere({0, 0, 0}, 0.7f, shader);
                        sphere.scale({1 + strain, scale_y, 1})
                            .translate(origin + vector_t{0, scalar_t{0.7f} * scale_y, 0});
                        objects.push_back(std::move(sphere));
                    }
                }
            }
            objects.push_back(floor);
            scene = scene_t{
                .objects = std::move(objects),
                .lights = {light_t{.origin = {-20, 30, 20}, .intensity = {0.85f, 0.68f, 0.50f}},
                           light_t{.origin = {20, 25, 0}, .intensity = {0.25f, 0.48f, 0.75f}},
                           light_t{.origin = {0, 20, -20}, .intensity = {0.55f, 0.65f, 0.85f}}},
                .camera = camera_t{.origin = {0, 32, 48},
                                   .direction = {0, -0.6875f, -1},
                                   .up = {0, 1, 0},
                                   .fov = 28}};
        }
    }

    constexpr int width = 3840;
    constexpr int height = 2160;
    image_t image(width, height);
    std::println("Scene: {} ({} bounded, {} unbounded objects)",
        csg_scene ? "csg" : many_objects ? "many" : "demo", scene.spatial_index.bounded.size(),
        scene.spatial_index.unbounded.size());
    if (benchmark) {
        render(scene, bg::view(image));
        std::vector<double> times;
        for (int run = 0; run < 5; ++run) {
            const auto start = std::chrono::steady_clock::now();
            render(scene, bg::view(image));
            const double milliseconds = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count();
            times.push_back(milliseconds);
            std::println("Run {}: {:.2f} ms", run + 1, milliseconds);
        }
        std::ranges::sort(times);
        std::uint64_t checksum = 14695981039346656037ULL;
        const auto pixels = bg::const_view(image);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                for (int channel = 0; channel < 3; ++channel) {
                    checksum ^= pixels(x, y)[channel];
                    checksum *= 1099511628211ULL;
                }
            }
        }
        std::println("Median: {:.2f} ms; pixel checksum: {:016x}", times[2], checksum);
        return 0;
    }
    std::println("Rendering {}x{}...", width, height);
    const auto start_time = std::chrono::steady_clock::now();
    render(scene, bg::view(image));
    const auto end_time = std::chrono::steady_clock::now();
    std::println("Rendering time: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());
    const char* output = csg_scene ? "output-csg.png"
        : many_objects ? "output-many.png" : "output.png";
    bg::write_view(output, bg::const_view(image), bg::png_tag{});
    std::println("Saved {} ({}x{})", output, width, height);


    // const auto ray = ray_t{
    //     .origin = {0.0, 0.0, 0.0},
    //     .direction = {0.0, 0.0, -1.0},
    //     .min = 0.0,
    //     .max = 100.0
    // };

    // hit_buffer_t buffer;
    // if (const auto hits = buffer.intersect(scene, ray); !hits.empty()) {
    //     const hit_t nearest = hits.front();
    //     const auto* hit = &nearest;
    //     std::print("distance: {}, point: ({}, {}, {}), normal: ({}, {}, {})\n",
    //                hit->distance,
    //                hit->point.a[0], hit->point.a[1], hit->point.a[2],
    //                hit->normal.a[0], hit->normal.a[1], hit->normal.a[2]);

    //     const primitive_t& object = *hit->object;
    //     const vector_t color = object.eluminate(*hit, ray, scene, 10, buffer);
    //     std::print("color: ({}, {}, {})\n", color.a[0], color.a[1], color.a[2]);
    // }

    return 0;
} catch (const std::exception& error) {
    std::println(stderr, "Render failed: {}", error.what());
    return 1;
}
