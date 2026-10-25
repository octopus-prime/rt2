module;

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>
#include <cmath>
#include <boost/qvm.hpp>
#include <boost/math/tools/quartic_roots.hpp>

module model;

import :math;

namespace rt2::model {

bool torus_t::inside(const vector_t& point) const {
    validate_point(point);
    return std::hypot(std::hypot(point.a[0], point.a[1])
        - std::sqrt(major_radius_squared), point.a[2]) < std::sqrt(minor_radius_squared);
}

std::optional<box3_t> torus_t::bounds() const {
    const scalar_t minor = std::sqrt(minor_radius_squared);
    const scalar_t radius = std::sqrt(major_radius_squared) + minor;
    return box3_t{point3_t{-radius, -radius, -minor}, point3_t{radius, radius, minor}};
}

std::size_t torus_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    const scalar_t a = boost::qvm::mag_sqr(ray.direction);
    if (a == 0) {
        return 0;
    }
    // Solve near the ray's closest approach to the origin, rather than using
    // large camera-distance terms whose cancellation destroys float precision.
    const scalar_t offset = -boost::qvm::dot(ray.origin, ray.direction) / a;
    const vector_t origin = ray(offset);
    const scalar_t b = 2 * boost::qvm::dot(origin, ray.direction);
    const scalar_t c = boost::qvm::mag_sqr(origin) + major_radius_squared - minor_radius_squared;
    const auto roots = boost::math::tools::quartic_roots(
        a * a, 2 * a * b,
        b * b + 2 * a * c - 4 * major_radius_squared * boost::qvm::mag_sqr(boost::qvm::XY(ray.direction)),
        2 * b * c - 8 * major_radius_squared * boost::qvm::dot(boost::qvm::XY(origin), boost::qvm::XY(ray.direction)),
        c * c - 4 * major_radius_squared * boost::qvm::mag_sqr(boost::qvm::XY(origin)));
    auto ts = roots | std::views::filter([&](auto t){ return ray[t + offset]; });
    std::size_t count = 0;
    for (const auto root : ts) {
        const vector_t point = origin + root * ray.direction;
        const scalar_t s = boost::qvm::mag_sqr(point) + major_radius_squared - minor_radius_squared;
        const vector_t normal = boost::qvm::normalized(vector_t{
            point.a[0] * (s - 2 * major_radius_squared),
            point.a[1] * (s - 2 * major_radius_squared),
            point.a[2] * s
        });
        append_hit(hits, count, hit_t{
            .point = point,
            .normal = normal,
            .distance = root + offset
        });
    }
    return finish_hits(hits, count);
}

}