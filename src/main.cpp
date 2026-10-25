#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <print>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>
#include <boost/qvm.hpp>
#include <boost/gil.hpp>
#include <boost/gil/extension/io/png.hpp>

import model;
import renderer;

namespace bg = boost::gil;
using namespace rt2::model;
using namespace rt2::renderer;

using image_t = bg::rgb8_image_t;

// static_assert(Intersectable<sphere_t>);
// static_assert(Intersectable<torus_t>);
// static_assert(Intersectable<surface_t>);

// static_assert(Intersectable<object_t>);
// static_assert(Transformable<object_t>);

// static_assert(Intersectable<scene_t>);

int main(int argc, char** argv) try {
    bool many_objects = false;
    bool benchmark = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument{argv[i]};
        if (argument == "--many") {
            many_objects = true;
        } else if (argument == "--benchmark") {
            benchmark = true;
        } else if (argument == "--help") {
            std::println("Usage: rt2 [--many] [--benchmark]\n"
                "  --many       Render a second scene with 400 bounded objects and one plane.\n"
                "  --benchmark  Warm up, time five renders, print median and pixel checksum;\n"
                "               skip PNG output. Scene construction is not timed.");
            return 0;
        } else {
            throw std::invalid_argument("Unknown argument; use --help for usage");
        }
    }
    const auto object1 = object_t{
        .surface = surface_t{sphere_t{}},
        .shader = shader_t{dielectric_t{.refractive_index = 1.5f}}
    }
    .scale({1.35f, 1.35f, 1.35f})
    .translate({0.7f, -0.65f, 0.5f});

    const auto object2 = object_t {
        .surface = surface_t{torus_t{4.f, 0.25f}},
        .shader = shader_t{metal_t{.reflectance = {0.92f, 0.68f, 0.32f}}}
    }
    .translate({-2.6f, 0.5f, -3.0f});

    const auto make_sphere = [](const vector_t& origin, scalar_t radius, const shader_t& shader) {
        auto sphere = object_t{
            .surface = surface_t{sphere_t{}},
            .shader = shader
        };
        sphere.scale({radius, radius, radius}).translate(origin);
        return sphere;
    };

    const auto copper = make_sphere({3.3f, -0.9f, -2.0f}, 1.1f,
        shader_t{metal_t{.reflectance = {0.95f, 0.48f, 0.28f}}});
    const auto blue = make_sphere({0.3f, -0.35f, -4.6f}, 1.65f,
        shader_t{phong_t{
            .ambient = {0.008f, 0.018f, 0.035f},
            .diffuse = {0.035f, 0.18f, 0.38f},
            .specular = {0.5f, 0.6f, 0.7f},
            .shininess = 80
        }});
    const auto coral = make_sphere({-3.0f, -1.35f, 1.0f}, 0.65f,
        shader_t{phong_t{
            .ambient = {0.025f, 0.006f, 0.008f},
            .diffuse = {0.55f, 0.055f, 0.075f},
            .specular = {0.35f, 0.25f, 0.25f},
            .shininess = 48
        }});
    const auto silver = make_sphere({2.7f, -1.5f, 1.5f}, 0.5f,
        shader_t{metal_t{.reflectance = {0.88f, 0.92f, 0.98f}}});

    const auto make_sor = [](const sor_t& surface, const vector_t& origin, const shader_t& shader) {
        auto object = object_t{
            .surface = surface_t{surface},
            .shader = shader,
            // Map the profile's local z axis to world +y.
            .scene_object = matrix_t{{
                {1, 0, 0, 0},
                {0, 0, 1, 0},
                {0, -1, 0, 0},
                {0, 0, 0, 1}
            }}
        };
        object.translate(origin);
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
        {4.8f, -2, -3.5f},
        shader_t{metal_t{.reflectance = {0.8f, 0.85f, 0.92f}}});

    const auto make_wall = [](const vector_t& origin, const vector_t& normal, const vector_t& color) {
        const vector_t reference = normal.a[1] == 0
            ? vector_t{0, 1, 0} : vector_t{1, 0, 0};
        const vector_t right = boost::qvm::normalized(boost::qvm::cross(reference, normal));
        const vector_t up = boost::qvm::cross(normal, right);
        auto wall = object_t{
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

    auto scene = scene_t{
        .objects = {object1, object2, copper, blue, coral, silver, vase, spindle,
                    floor, ceiling, back, left, right},
        .lights = {light1, light2, light3},
        .camera = camera_t{
            .origin = {0.0f, 3.0f, 12.0f},
            .direction = {0.0f, -0.20f, -1.0f},
            .up = {0.0f, 1.0f, 0.0f},
            .fov = 48.0f
        }
    };

    if (many_objects) {
        objects_t objects;
        objects.reserve(401);
        for (int row = 0; row < 20; ++row) {
            for (int column = 0; column < 20; ++column) {
                const vector_t origin{
                    (static_cast<scalar_t>(column) - 9.5f) * 2.2f,
                    -2,
                    (static_cast<scalar_t>(row) - 9.5f) * 2.2f};
                const int kind = (row * 20 + column) % 10;
                if (kind == 0) {
                    auto object = make_sor(
                        sor_t{{{0, 0.55f}, {0.3f, 0.8f}, {1.1f, 0.9f},
                               {1.8f, 0.4f}, {2.5f, 0.3f}, {2.8f, 0.55f}}},
                        {0, 0, 0}, vase.shader);
                    object.scale({0.7f, 0.7f, 0.7f}).translate(origin);
                    objects.push_back(object);
                } else if (kind == 1) {
                    auto object = object_t{
                        .surface = surface_t{torus_t{4, 0.25f}},
                        .shader = object2.shader};
                    object.scale({0.4f, 0.4f, 0.4f})
                        .translate(origin + vector_t{0, 1, 0});
                    objects.push_back(object);
                } else {
                    const auto& shader = kind % 3 == 0 ? copper.shader
                        : kind % 3 == 1 ? blue.shader : coral.shader;
                    objects.push_back(make_sphere(origin + vector_t{0, 0.7f, 0},
                        0.7f, shader));
                }
            }
        }
        objects.push_back(floor);
        scene = scene_t{
            .objects = std::move(objects),
            .lights = {
                light_t{.origin = {-20, 30, 20}, .intensity = {0.85f, 0.68f, 0.50f}},
                light_t{.origin = {20, 25, 0}, .intensity = {0.25f, 0.48f, 0.75f}},
                light_t{.origin = {0, 20, -20}, .intensity = {0.55f, 0.65f, 0.85f}}},
            .camera = camera_t{
                .origin = {0, 32, 48},
                .direction = {0, -0.6f, -1},
                .up = {0, 1, 0},
                .fov = 55}
        };
    }

    constexpr int width = 3840;
    constexpr int height = 2160;
    image_t image(width, height);
    std::println("Scene: {} ({} bounded, {} unbounded objects)",
        many_objects ? "many" : "demo", scene.spatial_index.bounded.size(),
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
    const char* output = many_objects ? "output-many.png" : "output.png";
    bg::write_view(output, bg::const_view(image), bg::png_tag{});
    std::println("Saved {} ({}x{})", output, width, height);


    // const auto ray = ray_t{
    //     .origin = {0.0, 0.0, 0.0},
    //     .direction = {0.0, 0.0, -1.0},
    //     .min = 0.0,
    //     .max = 100.0
    // };

    // if (const auto hit = scene.intersect(ray)) {
    //     std::print("distance: {}, point: ({}, {}, {}), normal: ({}, {}, {})\n",
    //                hit->distance,
    //                hit->point.a[0], hit->point.a[1], hit->point.a[2],
    //                hit->normal.a[0], hit->normal.a[1], hit->normal.a[2]);

    //     const object_t& object = *hit->object;
    //     const vector_t color = object.eluminate(*hit, ray, scene, 10);
    //     std::print("color: ({}, {}, {})\n", color.a[0], color.a[1], color.a[2]);
    // }

    return 0;
} catch (const std::exception& error) {
    std::println(stderr, "Render failed: {}", error.what());
    return 1;
}
