module;

#include <optional>

export module model:sphere;

import :ray;
import :hit;

export namespace rt2::model {

struct sphere_t {
    std::optional<hit_t> intersect(const ray_t& ray) const;
};

}
