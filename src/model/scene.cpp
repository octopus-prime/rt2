module;

#include <optional>
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/index/rtree.hpp>

module model;

import :math;
import :ray;
import :hit;
import :object;
import :light;

namespace rt2::model {

scene_t::index_t::index_t() = default;
scene_t::index_t::index_t(const index_t&) = default;
scene_t::index_t::index_t(index_t&&) = default;
scene_t::index_t& scene_t::index_t::operator=(const index_t&) = default;
scene_t::index_t& scene_t::index_t::operator=(index_t&&) = default;
scene_t::index_t::~index_t() = default;

scene_t::index_t scene_t::build_index() const {
    std::vector<std::pair<box3_t, std::size_t>> bounded;
    index_t result;
    for (std::size_t i = 0; i < objects.size(); ++i) {
        if (objects[i].scene_bounds) {
            bounded.emplace_back(*objects[i].scene_bounds, i);
        } else {
            result.unbounded.push_back(i);
        }
    }
    result.bounded = decltype(result.bounded)(bounded.begin(), bounded.end());
    return result;
}

void scene_t::rebuild_index() {
    for (auto& object : objects) {
        object.update_transform();
    }
    spatial_index = build_index();
}

bool scene_t::occluded(const ray_t& ray) const {
    const auto segment = ray.segment();
    if (ray.min > ray.max) {
        return false;
    }
    const auto predicate = boost::geometry::index::intersects(segment);
    for (auto it = spatial_index.bounded.qbegin(predicate);
         it != spatial_index.bounded.qend(); ++it) {
        if (objects[it->second].intersect_surface(ray)) {
            return true;
        }
    }
    return std::ranges::any_of(spatial_index.unbounded, [&](std::size_t i) {
        return objects[i].intersect_surface(ray).has_value();
    });
}

vector_t scene_t::trace(const ray_t& ray, unsigned remaining_depth) const {
    return intersect(ray).transform([&](const auto& hit) {
        return hit.object->eluminate(hit, ray, *this, remaining_depth);
    }).value_or(vector_t{0, 0, 0});
}

std::optional<hit_t> scene_t::intersect(const ray_t& ray) const {
    const auto segment = ray.segment();
    if (ray.min > ray.max) {
        return std::nullopt;
    }
    std::optional<hit_t> nearest_hit;
    std::size_t nearest_index = objects.size();
    const auto test = [&](std::size_t i) {
        // Keep numerical surface solving independent of candidate traversal order.
        if (const auto hit = objects[i].intersect_surface(ray)) {
            // Preserve linear traversal's object-order tie breaking.
            if (!nearest_hit || hit->distance < nearest_hit->distance
                || (hit->distance == nearest_hit->distance && i < nearest_index)) {
                nearest_hit = hit;
                nearest_index = i;
            }
        }
    };
    const auto predicate = boost::geometry::index::intersects(segment);
    for (auto it = spatial_index.bounded.qbegin(predicate);
         it != spatial_index.bounded.qend(); ++it) {
        test(it->second);
    }
    for (const auto i : spatial_index.unbounded) {
        test(i);
    }
    return nearest_hit;
}

}
