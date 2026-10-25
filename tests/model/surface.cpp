#include <array>

#include "../support.hpp"

namespace rt2::tests {

void test_surface() {
    const std::array<surface_t, 6> surfaces{
        surface_t{sphere_t{}}, surface_t{plane_t{}}, surface_t{torus_t{4, 0.25f}},
        surface_t{sor_t{{{0, 1}, {1, 1}, {2, 1}, {3, 1}}}},
        surface_t{mesh_t{
            {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}},
            {{2, 4, 0}, {2, 1, 4}, {2, 5, 1}, {2, 0, 5},
             {3, 0, 4}, {3, 4, 1}, {3, 1, 5}, {3, 5, 0}}}},
        surface_t{text_t{RT2_FONT_FILE, "H", 0.25f}}};
    const std::array<vector_t, 6> interior{
        vector_t{0, 0, 0}, {0, 0, -1}, {2, 0, 0}, {0, 0, 1},
        {0, 0, 0}, {0.15f, 0.2f, 0.125f}};
    for (std::size_t i = 0; i < surfaces.size(); ++i) {
        check(surfaces[i].inside(interior[i]), "Variant forwards interior query");
        check(!surfaces[i].inside({10, 10, 10}), "Variant forwards exterior query");
        check(surfaces[i].intersect(ray(interior[i], {0, 0, 1})).has_value(),
            "Variant forwards intersection");
        check_inside_rejects_nonfinite(surfaces[i]);
    }
}

}
