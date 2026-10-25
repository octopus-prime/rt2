module;

#include <optional>
#include <variant>

export module model:surface;

import :ray;
import :hit;
import :math;
export import :plane;
export import :sphere;
export import :torus;
export import :sor;
export import :mesh;
export import :text;

export namespace rt2::model {

struct surface_t {
    std::variant<plane_t, sphere_t, torus_t, sor_t, mesh_t, text_t> variant;

    // nullopt denotes an unbounded surface.
    std::optional<box3_t> bounds() const;
    std::optional<hit_t> intersect(const ray_t& ray) const;
};

}
