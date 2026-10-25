module;

#include <cstddef>
#include <span>
#include <optional>

export module model:csg_union;

import :math;
import :ray;
import :hit;
import :object;

export namespace rt2::model {

struct union_t {
    object_t object1;
    object_t object2;

    std::optional<box3_t> bounds() const;
    void update_transform();
    bool inside(const vector_t& point) const;
    std::size_t intersect(const ray_t& ray, std::span<hit_t> hits) const;
    union_t& translate(const vector_t& offset);
    union_t& scale(const vector_t& factors);
    union_t& rotate_x(scalar_t angle);
    union_t& rotate_y(scalar_t angle);
    union_t& rotate_z(scalar_t angle);
};

}
