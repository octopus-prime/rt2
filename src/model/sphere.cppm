module;

#include <optional>

export module model:sphere;

import :ray;
import :hit;
import :math;

export namespace rt2::model {

struct sphere_t {
    bool inside(const vector_t& point) const;
    std::optional<box3_t> bounds() const;
    std::optional<hit_t> intersect(const ray_t& ray) const;
};

}
