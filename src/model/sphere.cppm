module;

#include <optional>
#include <span>
#include <cstddef>

export module model:sphere;

import :ray;
import :hit;
import :math;

export namespace rt2::model {

struct sphere_t {
    bool inside(const vector_t& point) const;
    std::optional<box3_t> bounds() const;
    std::size_t intersect(const ray_t& ray, std::span<hit_t> hits) const;
};

}
