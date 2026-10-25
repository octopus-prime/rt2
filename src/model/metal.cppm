export module model:metal;

import :math;
import :ray;
import :hit;
import :fwd;

export namespace rt2::model {

struct metal_t {
    // Constant RGB reflectance: polished mirror approximation, no transmission.
    vector_t reflectance;

    vector_t eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth) const;
};

}
