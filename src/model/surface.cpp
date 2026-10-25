module;

#include <optional>
#include <span>
#include <variant>

module model;

import :ray;
import :hit;

namespace rt2::model {

bool surface_t::inside(const vector_t& point) const {
    return std::visit([&](const auto& surface) { return surface.inside(point); }, variant);
}

std::optional<box3_t> surface_t::bounds() const {
    return std::visit([](const auto& surface) { return surface.bounds(); }, variant);
}

std::size_t surface_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    return std::visit([&](const auto& surface){ return surface.intersect(ray, hits); }, variant);
}

}
