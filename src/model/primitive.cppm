module;

#include <optional>
#include <span>
#include <cstddef>
#include <boost/qvm.hpp>

export module model:primitive;

import :math;
import :ray;
import :hit;
import :surface;
import :shader;
import :fwd;

export namespace rt2::model {

struct primitive_t {
    surface_t surface;
    shader_t shader;
    matrix_t scene_object = boost::qvm::identity_mat<scalar_t, 4>();
    matrix_t object_scene = boost::qvm::inverse(scene_object);
    std::optional<box3_t> scene_bounds = transformed_bounds();

    // Call after directly changing surface or scene_object; transform methods do this automatically.
    void update_transform();
    primitive_t& translate(const vector_t& offset);
    primitive_t& scale(const vector_t& factors);
    // Angles are in radians; rotations are about scene-space axes through the origin.
    primitive_t& rotate_x(const scalar_t angle);
    primitive_t& rotate_y(const scalar_t angle);
    primitive_t& rotate_z(const scalar_t angle);

    std::size_t intersect(const ray_t& ray, std::span<hit_t> hits) const;
    bool inside(const vector_t& point) const;
    vector_t eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth, hit_buffer_t& buffer) const;

private:
    friend struct object_t;
    std::size_t intersect_surface(const ray_t& ray, std::span<hit_t> hits) const;
    std::optional<box3_t> transformed_bounds() const;
};

}
