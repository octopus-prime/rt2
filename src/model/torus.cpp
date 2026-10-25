module;

#include <algorithm>
#include <optional>
#include <algorithm>
#include <ranges>
#include <boost/qvm.hpp>
#include <boost/math/tools/quartic_roots.hpp>

module model;

import :math;

namespace rt2::model {

std::optional<hit_t> torus_t::intersect(const ray_t& ray) const {
    const scalar_t a = boost::qvm::mag_sqr(ray.direction);
    const scalar_t b = 2 * boost::qvm::dot(ray.origin, ray.direction);
    const scalar_t c = boost::qvm::mag_sqr(ray.origin) + major_radius_squared - minor_radius_squared;
    const auto roots = boost::math::tools::quartic_roots(
        a * a, 2 * a * b,
        b * b + 2 * a * c - 4 * major_radius_squared * boost::qvm::mag_sqr(boost::qvm::XY(ray.direction)),
        2 * b * c - 8 * major_radius_squared * boost::qvm::dot(boost::qvm::XY(ray.origin), boost::qvm::XY(ray.direction)),
        c * c - 4 * major_radius_squared * boost::qvm::mag_sqr(boost::qvm::XY(ray.origin)));
    auto ts = roots | std::views::filter([&](auto t){ return ray[t]; });
    auto min = std::ranges::min_element(ts, {}, {});
    if (min == ts.end()) {
        return std::nullopt;
    }
    const vector_t point = ray(*min);
    const scalar_t s = boost::qvm::mag_sqr(point) + major_radius_squared - minor_radius_squared;
    const vector_t normal = boost::qvm::normalized(vector_t{
        point.a[0] * (s - 2 * major_radius_squared),
        point.a[1] * (s - 2 * major_radius_squared), 
        point.a[2] * s
    });
    return hit_t{
        .point = point,
        .normal = normal, // Use the computed normal for the torus
        .distance = *min
    };
}

}