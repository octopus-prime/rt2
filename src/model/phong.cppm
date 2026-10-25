export module model:phong;

import :math;
import :ray;
import :hit;
import :fwd;

export namespace rt2::model {

struct phong_t {
    vector_t ambient;
    vector_t diffuse;
    vector_t specular;
    scalar_t shininess;

    vector_t eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth, hit_buffer_t& buffer) const;
};

}
