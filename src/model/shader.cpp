module;

#include <variant>

module model;

import :math;
import :ray;
import :hit;
import :scene;
import :phong;
import :dielectric;
import :metal;

namespace rt2::model {

vector_t shader_t::eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth) const {
    return std::visit([&](const auto& shader) {
        return shader.eluminate(hit, incoming, scene, remaining_depth);
    }, variant);
}

}
