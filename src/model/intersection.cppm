module;

#include <cstddef>
#include <span>
#include <optional>

export module model:intersection;

import :math;
import :ray;
import :hit;
import :object;

export namespace rt2::model {

struct intersection_t {
    object_t object1;
    object_t object2;

    std::optional<box3_t> bounds() const;
    void update_transform();
    bool inside(const vector_t& point) const;
    std::size_t intersect(const ray_t& ray, std::span<hit_t> hits) const;
    intersection_t& translate(const vector_t& offset);
    intersection_t& scale(const vector_t& factors);
    intersection_t& rotate_x(scalar_t angle);
    intersection_t& rotate_y(scalar_t angle);
    intersection_t& rotate_z(scalar_t angle);
};

}
