module;

#include <optional>

export module model:torus;

import :math;
import :ray;
import :hit;

export namespace rt2::model {

struct torus_t {
    scalar_t major_radius_squared;
    scalar_t minor_radius_squared;
    std::optional<hit_t> intersect(const ray_t& ray) const;
};

}
