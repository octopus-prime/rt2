module;

#include <optional>

export module model:plane;

import :ray;
import :hit;
import :math;

export namespace rt2::model {

struct plane_t {
    // An infinite plane has no finite bounding box.
    std::optional<box3_t> bounds() const;
    // Local XY plane: z = 0, with normal pointing along +Z.
    std::optional<hit_t> intersect(const ray_t& ray) const;
};

} // namespace rt2::model
    