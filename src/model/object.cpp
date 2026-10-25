module;

#include <optional>
#include <boost/qvm.hpp>

module model;

import :math;
import :ray;
import :hit;
import :surface;
import :shader;

namespace rt2::model {

object_t& object_t::translate(const vector_t& offset) {
    scene_object = boost::qvm::translation_mat(offset) * scene_object;
    object_scene = boost::qvm::inverse(scene_object);
    return *this;
}

object_t& object_t::scale(const vector_t& factors) {
    scene_object = boost::qvm::diag_mat(boost::qvm::XYZ1(factors)) * scene_object;
    object_scene = boost::qvm::inverse(scene_object);
    return *this;
}

std::optional<hit_t> object_t::intersect(const ray_t& ray) const {
    const ray_t transformed_ray{
        .origin = boost::qvm::transform_point(object_scene, ray.origin),
        .direction = boost::qvm::transform_vector(object_scene, ray.direction),
        .min = ray.min,
        .max = ray.max
    };
    auto nearest = surface.intersect(transformed_ray);
    if (!nearest) {
        return std::nullopt;
    }
    nearest->point = boost::qvm::transform_point(scene_object, nearest->point);
    nearest->normal = boost::qvm::normalized(boost::qvm::transform_vector(boost::qvm::transposed(object_scene), nearest->normal));
    nearest->object = this;
    return nearest;
}

vector_t object_t::eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth) const {
    return shader.eluminate(hit, incoming, scene, remaining_depth);
}

}
