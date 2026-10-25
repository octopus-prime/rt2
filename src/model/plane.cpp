module;

#include <optional>
#include <cmath>
#include <span>

module model;

namespace rt2::model {
bool plane_t::inside(const vector_t& point) const {
    validate_point(point);
    return point.a[2] < 0;
}

std::optional<box3_t> plane_t::bounds() const {
    return std::nullopt;
}

std::optional<hit_t> plane_t::intersect(const ray_t& ray) const {
    auto denom = ray.direction.a[2];
    if (std::abs(denom) < 1e-6f) {
        return std::nullopt;
    }
    auto t = -ray.origin.a[2] / denom;
    if (!ray[t]) {
        return std::nullopt;
    }
    return hit_t{
        .point = ray(t),
        .normal = {0, 0, 1},
        .distance = t
    };
}

} // namespace rt2::model
