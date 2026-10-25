module;

#include <ranges>
#include <boost/qvm.hpp>
#include <boost/math/tools/roots.hpp>

module model;

import :math;

namespace rt2::model {
bool sphere_t::inside(const vector_t& point) const {
    validate_point(point);
    return std::hypot(point.a[0], point.a[1], point.a[2]) < 1;
}

std::optional<box3_t> sphere_t::bounds() const {
    return box3_t{point3_t{-1, -1, -1}, point3_t{1, 1, 1}};
}

// static_assert(Intersectable<sphere_t>);

std::optional<hit_t> sphere_t::intersect(const ray_t& ray) const {
    const scalar_t a = boost::qvm::dot(ray.direction, ray.direction);
    const scalar_t b = 2 * boost::qvm::dot(ray.origin, ray.direction);
    const scalar_t c = boost::qvm::dot(ray.origin, ray.origin) - 1;
    const auto [t1, t2] = boost::math::tools::quadratic_roots(a, b, c);
    const auto ts = {t1, t2};
    auto valid = ts | std::views::filter([&](auto t){ return ray[t]; });
    auto min = std::ranges::min_element(valid, {}, {});
    if (min == valid.end()) {
        return std::nullopt;
    }
    const vector_t point = ray(*min);
    return hit_t{
        .point = point,
        .normal = boost::qvm::normalized(point),
        .distance = *min
    };
}

}
