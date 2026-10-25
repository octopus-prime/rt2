export module model:dielectric;

import :math;
import :ray;
import :hit;
import :fwd;

export namespace rt2::model {

struct dielectric_t {
    scalar_t refractive_index;

    vector_t eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth, hit_buffer_t& buffer) const;
};

}
