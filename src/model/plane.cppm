module;

#include <optional>

export module model:plane;

import :ray;
import :hit;

export namespace rt2::model {

struct plane_t {
    // Local XY plane: z = 0, with normal pointing along +Z.
    std::optional<hit_t> intersect(const ray_t& ray) const;
};

} // namespace rt2::model
    