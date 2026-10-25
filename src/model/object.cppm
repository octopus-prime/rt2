module;

#include <optional>
#include <vector>
#include <boost/qvm.hpp>

export module model:object;

import :math;
import :ray;
import :hit;
import :surface;
import :shader;
import :fwd;

export namespace rt2::model {

struct object_t {
    surface_t surface;
    shader_t shader;
    matrix_t scene_object = boost::qvm::identity_mat<scalar_t, 4>();
    matrix_t object_scene = boost::qvm::identity_mat<scalar_t, 4>();

    object_t& translate(const vector_t& offset);
    object_t& scale(const vector_t& factors);

    std::optional<hit_t> intersect(const ray_t& ray) const;
    vector_t eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth) const;
};

using objects_t = std::vector<object_t>;

}
