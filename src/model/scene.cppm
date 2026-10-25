module;

#include <optional>

export module model:scene;

import :fwd;
import :math;
import :ray;
import :hit;
import :object;
import :light;
import :camera;

export namespace rt2::model {

struct scene_t {
    objects_t objects;
    lights_t lights;
    camera_t camera;

    bool occluded(const ray_t& ray) const;
    vector_t trace(const ray_t& ray, unsigned remaining_depth) const;

private:
    std::optional<hit_t> intersect(const ray_t& ray) const;
};

}
