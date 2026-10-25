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

std::size_t plane_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    auto denom = ray.direction.a[2];
    if (std::abs(denom) < 1e-6f) {
        return 0;
    }
    auto t = -ray.origin.a[2] / denom;
    if (!ray[t]) {
        return 0;
    }
    std::size_t count = 0;
    append_hit(hits, count, hit_t{
        .point = ray(t),
        .normal = {0, 0, 1},
        .distance = t
    });
    return count;
}

} // namespace rt2::model
