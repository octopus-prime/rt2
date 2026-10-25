module;

#include <optional>
#include <algorithm>

module model;

import :math;
import :ray;
import :hit;
import :object;
import :light;

namespace rt2::model {

bool scene_t::occluded(const ray_t& ray) const {
    return std::ranges::any_of(objects, [&](const object_t& object) {
        return object.intersect(ray).has_value();
    });
}

vector_t scene_t::trace(const ray_t& ray, unsigned remaining_depth) const {
    return intersect(ray).transform([&](const auto& hit) {
        return hit.object->eluminate(hit, ray, *this, remaining_depth);
    }).value_or(vector_t{0, 0, 0});
}

std::optional<hit_t> scene_t::intersect(const ray_t& ray) const {
    std::optional<hit_t> nearest_hit;
    ray_t clipped_ray = ray;
    for (const auto& object : objects) {
        if (const auto hit = object.intersect(clipped_ray)) {
            if (!nearest_hit || hit->distance < nearest_hit->distance) {
                nearest_hit = hit;
                clipped_ray.max = hit->distance;
            }
        }
    }
    return nearest_hit;
}

}
