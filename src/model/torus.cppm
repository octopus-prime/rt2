module;

#include <optional>
#include <span>
#include <cstddef>

export module model:torus;

import :math;
import :ray;
import :hit;

export namespace rt2::model {

struct torus_t {
    scalar_t major_radius_squared;
    scalar_t minor_radius_squared;
    bool inside(const vector_t& point) const;
    std::optional<box3_t> bounds() const;
    std::size_t intersect(const ray_t& ray, std::span<hit_t> hits) const;
};

}
