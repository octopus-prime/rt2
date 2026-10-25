#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <exception>
#include <limits>
#include <print>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <boost/math/interpolators/pchip.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/qvm.hpp>

import model;

using namespace rt2::model;

namespace {

constexpr scalar_t tolerance = 256 * std::numeric_limits<scalar_t>::epsilon();

void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void close(scalar_t actual, scalar_t expected, const char* message) {
    if (!std::isfinite(actual)
        || std::abs(actual - expected) > tolerance * std::max(scalar_t{1}, std::abs(expected))) {
        throw std::runtime_error(std::string(message) + ": expected " + std::to_string(expected)
            + ", got " + std::to_string(actual));
    }
}

ray_t ray(vector_t origin, vector_t direction, scalar_t min = 0,
          scalar_t max = ray_max) {
    return {.origin = origin, .direction = direction, .min = min, .max = max};
}

void check_hit(const sor_t& surface, const ray_t& incoming, scalar_t distance, vector_t normal) {
    const auto hit = surface.intersect(incoming);
    check(hit.has_value(), "Expected a surface hit");
    close(hit->distance, distance, "Hit distance");
    const auto point = incoming(distance);
    for (int i = 0; i < 3; ++i) {
        close(hit->point.a[i], point.a[i], "Hit position");
        close(hit->normal.a[i], normal.a[i], "Hit normal");
    }
    check(hit->object == nullptr, "Local hit must not refer to an object");
}

void test_cylinder() {
    const sor_t cylinder{{{0, 1}, {1, 1}, {2, 1}, {3, 1}}};
    check_hit(cylinder, ray({2, 0, scalar_t{1.5f}}, {-1, 0, 0}), 1, {1, 0, 0});
    check_hit(cylinder, ray({0, 0, scalar_t{1.5f}}, {1, 0, 0}), 1, {1, 0, 0});
    check_hit(cylinder, ray({2, 0, 1}, {-2, 0, 0}), scalar_t{0.5f}, {1, 0, 0});
    check_hit(cylinder, ray({2, 0, 1}, {-1, 0, 0}, 2), 3, {-1, 0, 0});
    check_hit(cylinder, ray({0, 0, -1}, {0, 0, 1}), 1, {0, 0, -1});
    check_hit(cylinder, ray({0, 0, 4}, {0, 0, -1}), 1, {0, 0, 1});
    check_hit(cylinder, ray({0, 0, 1}, {0, 0, 1}), 2, {0, 0, 1});
    check_hit(cylinder, ray({-2, 1, scalar_t{1.5f}}, {1, 0, 0}), 2, {0, 1, 0});
    check_hit(cylinder, ray({2, 0, 1}, {-1, 0, 0}, 1, 1), 1, {1, 0, 0});
    check(!cylinder.intersect(ray({2, 0, 1}, {-1, 0, 0}, 0, scalar_t{0.9f})),
        "Ray maximum must exclude a farther hit");
    check(!cylinder.intersect(ray({2, 0, 1}, {1, 0, 0})), "Ray points away");
    check(!cylinder.intersect(ray({-2, scalar_t{1.01f}, 1}, {1, 0, 0})),
        "Near-tangent miss");
    check(!cylinder.intersect(ray({2, 0, 4}, {-1, 0, 0})), "Outside height range");

    const sor_t cone{{{0, 1}, {1, 2}, {2, 3}, {3, 4}}};
    const scalar_t inv_sqrt_two = 1 / std::sqrt(scalar_t{2});
    check_hit(cone, ray({5, 0, scalar_t{1.5f}}, {-1, 0, 0}), scalar_t{2.5f},
        {inv_sqrt_two, 0, -inv_sqrt_two});
}

void test_curved_profile() {
    std::vector<scalar_t> heights{0, scalar_t{0.3f}, scalar_t{1.1f},
        scalar_t{1.8f}, scalar_t{2.5f}, scalar_t{2.8f}};
    std::vector<scalar_t> radii{scalar_t{0.55f}, scalar_t{0.8f}, scalar_t{0.9f},
        scalar_t{0.4f}, scalar_t{0.3f}, scalar_t{0.55f}};
    std::vector<std::array<scalar_t, 2>> profile;
    for (std::size_t i = 0; i < heights.size(); ++i) {
        profile.push_back({heights[i], radii[i]});
    }
    const sor_t surface{profile};
    const boost::math::interpolators::pchip<std::vector<scalar_t>> spline(
        std::move(heights), std::move(radii));
    for (int i = 1; i < 28; ++i) {
        const scalar_t height = static_cast<scalar_t>(i) / 10;
        const scalar_t radius = spline(height);
        const scalar_t slope = spline.prime(height);
        const scalar_t length = std::sqrt(1 + slope * slope);
        check_hit(surface, ray({2, 0, height}, {-1, 0, 0}), 2 - radius,
            {1 / length, 0, -slope / length});
        const vector_t direction{-1, 0, scalar_t{0.2f}};
        const vector_t point{radius, 0, height};
        const vector_t origin{point.a[0] - 2 * direction.a[0], 0,
            point.a[2] - 2 * direction.a[2]};
        check_hit(surface, ray(origin, direction, scalar_t{1.9f}, scalar_t{2.1f}), 2,
            {1 / length, 0, -slope / length});
    }
    object_t object{
        .surface = surface_t{surface},
        .shader = shader_t{phong_t{}}
    };
    object.scale({2, 1, 1}).translate({3, 0, 0});
    const auto hit = object.intersect(ray({7, 0, 1}, {-1, 0, 0}));
    check(hit.has_value() && hit->object == &object, "Transformed SOR integration");
    close(hit->distance, 4 - 2 * spline(1), "Transformed SOR distance");
}

void test_invalid_profiles() {
    const auto rejects = [](const std::vector<std::array<scalar_t, 2>>& profile) {
        try {
            const sor_t surface{profile};
        } catch (const std::invalid_argument&) {
            return;
        }
        throw std::runtime_error("Invalid profile was accepted");
    };
    rejects({});
    rejects({{0, 1}, {1, 1}, {2, 1}});
    rejects({{0, 1}, {1, 1}, {1, 1}, {3, 1}});
    rejects({{0, 1}, {2, 1}, {1, 1}, {3, 1}});
    rejects({{0, 1}, {1, 0}, {2, 1}, {3, 1}});
    rejects({{0, 1}, {1, -1}, {2, 1}, {3, 1}});
    rejects({{0, 1}, {1, std::numeric_limits<scalar_t>::quiet_NaN()}, {2, 1}, {3, 1}});
    rejects({{0, 1}, {1, 1}, {2, 1}, {std::numeric_limits<scalar_t>::infinity(), 1}});
}

void test_object_bounds() {
    check(ray_t{}.max == ray_max, "Default ray limit");
    check(!plane_t{}.bounds(), "Infinite plane must be unbounded");
    const auto sphere_box = sphere_t{}.bounds();
    check(sphere_box.has_value(), "Sphere bounds");
    close(boost::geometry::get<boost::geometry::min_corner, 0>(*sphere_box), -1,
        "Sphere lower bound");
    const auto torus_box = torus_t{4, scalar_t{0.25f}}.bounds();
    close(boost::geometry::get<boost::geometry::max_corner, 0>(*torus_box),
        scalar_t{2.5f}, "Torus uses square roots of squared radii");
    close(boost::geometry::get<boost::geometry::max_corner, 2>(*torus_box),
        scalar_t{0.5f}, "Torus minor extent");
    const sor_t sor{{{0, 1}, {1, 2}, {2, 1}, {3, 1}}};
    const auto sor_box = sor.bounds();
    close(boost::geometry::get<boost::geometry::max_corner, 0>(*sor_box), 2,
        "SOR radial extent");
    close(boost::geometry::get<boost::geometry::max_corner, 2>(*sor_box), 3,
        "SOR height extent");

    object_t sphere{.surface = surface_t{sphere_t{}}, .shader = shader_t{phong_t{}}};
    check(sphere.scene_bounds.has_value(), "Bounds initialized for identity object");
    sphere.scale({2, 1, 1}).translate({3, 0, 0});
    const auto hit = sphere.intersect(ray({7, 0, 0}, {-1, 0, 0}));
    check(hit.has_value(), "Updated bounds must retain scaled translated sphere");
    close(hit->distance, 2, "Scaled translated sphere distance");
    check(!sphere.intersect(ray({0, 0, 0}, {-1, 0, 0})), "Box behind ray");
    check(!sphere.intersect(ray({7, 0, 0}, {-1, 0, 0}, 0, 1)), "Box beyond ray maximum");
    check(sphere.intersect(ray({7, 0, 0}, {-1, 0, 0}, 2, 2)).has_value(),
        "Degenerate segment on surface");
    check(sphere.intersect(ray({3, 0, 0}, {1, 0, 0})).has_value(), "Origin inside box");
    check(sphere.intersect(ray({3, -2, 1}, {0, 1, 0})).has_value(), "Tangent ray");

    sphere.surface = surface_t{plane_t{}};
    sphere.update_transform();
    check(!sphere.scene_bounds, "Changing to unbounded surface clears cache");
    check(sphere.intersect(ray({500, 500, 2}, {0, 0, -1})).has_value(),
        "Infinite plane bypasses bounds");
    sphere.surface = surface_t{sphere_t{}};
    sphere.scene_object = boost::qvm::identity_mat<scalar_t, 4>();
    sphere.update_transform();
    close(sphere.intersect(ray({0, 0, 3}, {0, 0, -1}))->distance, 2,
        "Explicit cache refresh after direct changes");

    // Compare broad-phase results with exact-only queries under rotation, shear,
    // reflection, nonuniform scale and translation.
    const matrix_t transform{{
        {0, -2, scalar_t{0.25f}, 3},
        {1, 0, scalar_t{0.5f}, -2},
        {0, 0, -scalar_t{1.5f}, 1},
        {0, 0, 0, 1}
    }};
    const std::array<surface_t, 4> surfaces{
        surface_t{sphere_t{}}, surface_t{torus_t{4, scalar_t{0.25f}}},
        surface_t{sor}, surface_t{plane_t{}}};
    for (const auto& surface : surfaces) {
        const object_t bounded{
            .surface = surface, .shader = shader_t{phong_t{}}, .scene_object = transform};
        auto exact = bounded;
        exact.scene_bounds.reset();
        for (int x = -6; x <= 6; ++x) {
            for (int y = -6; y <= 6; ++y) {
                for (int axis = 0; axis < 3; ++axis) {
                    vector_t origin{3, -2, 1};
                    origin.a[axis] += 12;
                    origin.a[(axis + 1) % 3] += static_cast<scalar_t>(x) / 2;
                    origin.a[(axis + 2) % 3] += static_cast<scalar_t>(y) / 2;
                    vector_t direction{0, 0, 0};
                    direction.a[axis] = -1;
                    const auto incoming = ray(origin, direction);
                    const auto expected = exact.intersect(incoming);
                    const auto actual = bounded.intersect(incoming);
                    check(actual.has_value() == expected.has_value(), "AABB changed hit/miss");
                    if (actual) {
                        close(actual->distance, expected->distance, "AABB changed nearest hit");
                    }
                }
            }
        }
    }
}

} // namespace

int main() {
    try {
        test_cylinder();
        test_curved_profile();
        test_invalid_profiles();
        test_object_bounds();
        std::println("SOR tests passed (scalar_t: {} bits)", sizeof(scalar_t) * 8);
    } catch (const std::exception& error) {
        std::println(stderr, "SOR test failed: {}", error.what());
        return 1;
    }
}
