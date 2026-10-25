module;

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <boost/qvm.hpp>

module model;

import :scene;

namespace rt2::model {

vector_t phong_t::eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth) const {
    vector_t color = ambient;
    const vector_t view_dir = boost::qvm::normalized(-incoming.direction);
    for (const auto& light : scene.lights) {
        const vector_t to_light = light.origin - hit.point;
        // if (boost::qvm::mag_sqr(to_light) == 0) {
        //     throw std::invalid_argument("Point light must not coincide with the shaded position");
        // }
        const vector_t light_dir = boost::qvm::normalized(to_light);
        const scalar_t diff = boost::qvm::dot(hit.normal, light_dir);
        if (diff <= 0) {
            continue;
        }
        const ray_t shadow_ray{
            .origin = hit.point,
            .direction = light_dir,
            .min = ray_epsilon,
            .max = boost::qvm::mag(to_light) - ray_epsilon
        };
        if (scene.occluded(shadow_ray)) {
            continue;
        }
        const vector_t diffuse_ = diff * diffuse * light.intensity;
        const vector_t reflect_dir = 2 * diff * hit.normal - light_dir;
        const scalar_t spec = std::pow(std::max(boost::qvm::dot(view_dir, reflect_dir), scalar_t{0}), shininess);
        const vector_t specular_ = spec * specular * light.intensity;
        color += diffuse_ + specular_;
    }
    return color;
}

}
