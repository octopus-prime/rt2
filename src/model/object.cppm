module;

#include <optional>
#include <span>
#include <cstddef>
#include <variant>
#include <vector>
#include <boost/variant/recursive_wrapper.hpp>

export module model:object;

import :math;
import :ray;
import :hit;
export import :primitive;

export namespace rt2::model {

struct intersection_t;
struct union_t;
struct difference_t;

struct object_t {
    std::variant<primitive_t, boost::recursive_wrapper<intersection_t>,
        boost::recursive_wrapper<union_t>, boost::recursive_wrapper<difference_t>> variant;

    object_t(primitive_t primitive);
    object_t(intersection_t intersection);
    object_t(union_t united);
    object_t(difference_t difference);
    object_t(const object_t&);
    object_t(object_t&&);
    object_t& operator=(const object_t&);
    object_t& operator=(object_t&&);
    ~object_t();
    std::optional<box3_t> bounds() const;
    void update_transform();
    object_t& translate(const vector_t& offset);
    object_t& scale(const vector_t& factors);
    object_t& rotate_x(scalar_t angle);
    object_t& rotate_y(scalar_t angle);
    object_t& rotate_z(scalar_t angle);
    std::size_t intersect(const ray_t& ray, std::span<hit_t> hits) const;
    bool inside(const vector_t& point) const;

private:
    friend struct scene_t;
    std::size_t intersect_surface(const ray_t& ray, std::span<hit_t> hits) const;
};

using objects_t = std::vector<object_t>;

}
