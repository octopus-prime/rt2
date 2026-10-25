module;

#include <optional>
#include <span>
#include <cstddef>
#include <utility>
#include <vector>
#include <boost/geometry/index/rtree.hpp>

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
    struct index_t {
        index_t();
        index_t(const index_t&);
        index_t(index_t&&);
        index_t& operator=(const index_t&);
        index_t& operator=(index_t&&);
        ~index_t();

        boost::geometry::index::rtree<std::pair<box3_t, std::size_t>,
            boost::geometry::index::quadratic<16>> bounded;
        std::vector<std::size_t> unbounded;
    };

    objects_t objects;
    lights_t lights;
    camera_t camera;
    index_t spatial_index = build_index();

    // Call after adding/removing/reordering objects or editing their geometry/transforms.
    // Do not mutate the scene while rendering.
    void rebuild_index();

    bool occluded(const ray_t& ray, hit_buffer_t& buffer) const;
    vector_t trace(const ray_t& ray, unsigned remaining_depth, hit_buffer_t& buffer) const;
    std::size_t intersect(const ray_t& ray, std::span<hit_t> hits) const;

private:
    index_t build_index() const;
};

}
