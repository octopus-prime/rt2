#include <array>
#include <utility>
#include <vector>
#include <boost/math/interpolators/pchip.hpp>

#include "../support.hpp"

namespace rt2::tests {

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

void test_sor() {
    const sor_t cylinder{{{0, 1}, {1, 1}, {2, 1}, {3, 1}}};
    check(cylinder.inside({0.5f, 0, 1}) && cylinder.inside({0, 0, 2}), "SOR segments");
    check(!cylinder.inside({1, 0, 1}) && !cylinder.inside({0, 0, 0})
        && !cylinder.inside({0, 0, 3}) && !cylinder.inside({0, 0, 4}), "SOR boundaries");
    const sor_t tapered{{{0, 1}, {1, 2}, {2, 3}, {3, 4}}};
    check(tapered.inside({2.4f, 0, 1.5f}) && !tapered.inside({2.6f, 0, 1.5f}),
        "SOR interpolated radius");
    check_inside_rejects_nonfinite(cylinder);
    test_cylinder();
    test_curved_profile();
    test_invalid_profiles();
}

} // namespace rt2::tests
