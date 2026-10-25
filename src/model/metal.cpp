module;

#include <cmath>
#include <limits>
#include <stdexcept>
#include <boost/qvm.hpp>

module model;

import :scene;

namespace rt2::model {

vector_t metal_t::eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth) const {
    // for (int i = 0; i < 3; ++i) {
    //     if (!std::isfinite(reflectance.a[i]) || reflectance.a[i] < 0 || reflectance.a[i] > 1) {
    //         throw std::invalid_argument("Metal reflectance channels must be finite and in [0, 1]");
    //     }
    // }
    if (remaining_depth == 0) {
        return {0, 0, 0};
    }
    const vector_t incident = boost::qvm::normalized(incoming.direction);
    const vector_t reflected = incident - 2 * boost::qvm::dot(incident, hit.normal) * hit.normal;
    const vector_t color = scene.trace(ray_t{
        .origin = hit.point,
        .direction = boost::qvm::normalized(reflected),
        .min = ray_epsilon,
        .max = std::numeric_limits<scalar_t>::infinity()
    }, remaining_depth - 1);
    return {reflectance.a[0] * color.a[0], reflectance.a[1] * color.a[1],
        reflectance.a[2] * color.a[2]};
}

}
