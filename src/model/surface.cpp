module;

#include <optional>
#include <variant>

module model;

import :ray;
import :hit;

namespace rt2::model {

std::optional<hit_t> surface_t::intersect(const ray_t& ray) const {
    return std::visit([&](const auto& surface){ return surface.intersect(ray); }, variant);
}

}
