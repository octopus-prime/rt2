module;

#include <variant>

export module model:shader;

import :math;
import :ray;
import :hit;
import :fwd;
import :phong;
import :dielectric;
import :metal;

export namespace rt2::model {

struct shader_t {
    std::variant<phong_t, dielectric_t, metal_t> variant;

    vector_t eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth) const;
};

}
