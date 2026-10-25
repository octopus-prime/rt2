module;

#include <optional>
#include <span>
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
        if (const auto bounds = objects[i].bounds()) {
            bounded.emplace_back(*bounds, i);
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

bool scene_t::occluded(const ray_t& ray, hit_buffer_t& buffer) const {
    const auto segment = ray.segment();
    if (ray.min > ray.max) {
        return false;
    }
    const auto predicate = boost::geometry::index::intersects(segment);
    for (auto it = spatial_index.bounded.qbegin(predicate);
         it != spatial_index.bounded.qend(); ++it) {
        if (!buffer.intersect(objects[it->second], ray).empty()) {
            return true;
        }
    }
    return std::ranges::any_of(spatial_index.unbounded, [&](std::size_t i) {
        return !buffer.intersect(objects[i], ray).empty();
    });
}

vector_t scene_t::trace(const ray_t& ray, unsigned remaining_depth, hit_buffer_t& buffer) const {
    const auto hits = buffer.intersect(*this, ray);
    if (hits.empty()) {
        return {0, 0, 0};
    }
    const hit_t nearest = hits.front();
    return nearest.object->eluminate(nearest, ray, *this, remaining_depth, buffer);
}

std::size_t scene_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    const auto segment = ray.segment();
    if (ray.min > ray.max) {
        return 0;
    }
    std::size_t count = 0;
    const auto test = [&](std::size_t i) {
        // Keep numerical surface solving independent of candidate traversal order.
        const auto added = objects[i].intersect_surface(ray, hits.subspan(count));
        for (auto& hit : hits.subspan(count, added)) {
            hit.source_index = i;
        }
        count += added;
    };
    const auto predicate = boost::geometry::index::intersects(segment);
    for (auto it = spatial_index.bounded.qbegin(predicate);
         it != spatial_index.bounded.qend(); ++it) {
        test(it->second);
    }
    for (const auto i : spatial_index.unbounded) {
        test(i);
    }
    // Preserve scene object order for exact-distance ties, independent of R-tree order.
    std::sort(hits.begin(), hits.begin() + count, [](const auto& a, const auto& b) {
        return a.distance < b.distance || (a.distance == b.distance
            && a.source_index < b.source_index);
    });
    return count;
}

}
