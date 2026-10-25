module;

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <boost/qvm.hpp>

module model;

import :scene;

namespace rt2::model {

vector_t dielectric_t::eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth, hit_buffer_t& buffer) const {
    // if (!std::isfinite(refractive_index) || refractive_index <= 0) {
    //     throw std::invalid_argument("Dielectric refractive index must be finite and positive");
    // }
    if (remaining_depth == 0) {
        return {0, 0, 0};
    }
    const vector_t incident = boost::qvm::normalized(incoming.direction);
    const bool entering = boost::qvm::dot(incident, hit.normal) < 0;
    const vector_t normal = entering ? hit.normal : -hit.normal;
    // Air outside, homogeneous dielectric inside; nested media are not tracked.
    const scalar_t eta_i = entering ? scalar_t{1} : refractive_index;
    const scalar_t eta_t = entering ? refractive_index : scalar_t{1};
    const scalar_t eta = eta_i / eta_t;
    const scalar_t cos_i = std::clamp(-boost::qvm::dot(incident, normal), scalar_t{0}, scalar_t{1});
    const auto trace_direction = [&](const vector_t& direction) {
        return scene.trace(ray_t{
            .origin = hit.point,
            .direction = boost::qvm::normalized(direction),
            .min = ray_epsilon,
            .max = ray_max
        }, remaining_depth - 1, buffer);
    };
    if (refractive_index == 1) {
        return trace_direction(incident);
    }
    const vector_t reflected = incident + 2 * cos_i * normal;
    const scalar_t sin_t_squared = eta * eta * std::max(scalar_t{0}, 1 - cos_i * cos_i);
    if (sin_t_squared >= 1) {
        return trace_direction(reflected);
    }
    const scalar_t cos_t = std::sqrt(1 - sin_t_squared);
    const scalar_t rs = (eta_i * cos_i - eta_t * cos_t) / (eta_i * cos_i + eta_t * cos_t);
    const scalar_t rp = (eta_t * cos_i - eta_i * cos_t) / (eta_t * cos_i + eta_i * cos_t);
    const scalar_t reflectance = (rs * rs + rp * rp) / 2;
    const vector_t refracted = eta * incident + (eta * cos_i - cos_t) * normal;
    return reflectance * trace_direction(reflected)
        + (1 - reflectance) * trace_direction(refracted);
}

}
