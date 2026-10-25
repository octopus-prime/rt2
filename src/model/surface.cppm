module;

#include <optional>
#include <variant>

export module model:surface;

import :ray;
import :hit;
export import :plane;
export import :sphere;
export import :torus;
export import :sor;

export namespace rt2::model {

struct surface_t {
    std::variant<plane_t, sphere_t, torus_t, sor_t> variant;

    std::optional<hit_t> intersect(const ray_t& ray) const;
};

}
