#include <array>
#include <numbers>
#include <span>
#include <variant>
#include <boost/qvm.hpp>

#include "../support.hpp"

namespace rt2::tests {

void test_difference() {
    const primitive_t sphere{.surface = surface_t{sphere_t{}},
        .shader = shader_t{phong_t{}}};
    auto other = sphere;
    other.translate({1, 0, 0});
    difference_t difference{sphere, other};
    const auto incoming = ray({-3, 0, 0}, {1, 0, 0});
    check_hit_distances(difference, incoming, {2, 3});
    check(difference.inside({-0.5f, 0, 0}) && !difference.inside({0.5f, 0, 0}),
        "Difference containment");
    hit_buffer_t buffer;
    const auto hits = buffer.intersect(difference, incoming);
    close(hits[1].normal.a[0], 1, "Difference reverses cutter normal");
    check(hits[1].object == &std::get<primitive_t>(difference.object2.variant),
        "Difference retains cut primitive for shading");
    check_hit_distances(difference, ray({-0.5f, 0, 0}, {1, 0, 0}), {0.5f});
    const difference_t identical{sphere, sphere};
    check_hit_distances(identical, incoming, {});
    check_hit_distances(identical, ray({-3, 1, 0}, {1, 0, 0}), {});
    check(!difference.inside({0, 0, 0}), "Cutter boundary excluded from difference interior");
    auto small = sphere;
    small.scale({0.5f, 0.5f, 0.5f});
    difference_t shell{sphere, small};
    check_hit_distances(shell, incoming, {2, 2.5f, 3.5f, 4});
    check_hit_distances(shell, ray({-3, 0, 0}, {1, 0, 0}, 2.75f, 3.75f), {3.5f});
    auto distant = sphere;
    distant.translate({10, 0, 0});
    check_hit_distances(difference_t{sphere, distant}, incoming, {2, 4});
    difference.translate({0, 2, 0}).scale({2, 1, 1})
        .rotate_x(0).rotate_y(0).rotate_z(0);
    check_hit_distances(difference, ray({-6, 2, 0}, {1, 0, 0}), {4, 6});
    const scalar_t angle = std::numbers::pi_v<scalar_t> / 2;
    difference.rotate_x(angle).rotate_y(angle).rotate_z(angle);
    check_hit_distances(difference, ray({0, 2, 6}, {0, 0, -1}), {4, 6});
    check_inside_rejects_nonfinite(difference);

    const primitive_t mesh{
        .surface = surface_t{mesh_t{
            {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}},
            {{2, 4, 0}, {2, 1, 4}, {2, 5, 1}, {2, 0, 5},
             {3, 0, 4}, {3, 4, 1}, {3, 1, 5}, {3, 5, 0}}}},
        .shader = shader_t{phong_t{}}};
    const primitive_t torus{
        .surface = surface_t{torus_t{0.36f, 0.04f}}, .shader = shader_t{phong_t{}}};
    check_hit_distances(difference_t{mesh, torus},
        ray({-2, 0, 0}, {1, 0, 0}), {1, 1.2f, 1.6f, 2.4f, 2.8f, 3});

    const primitive_t cylinder{
        .surface = surface_t{sor_t{{{0, 1}, {0.3f, 1}, {0.7f, 1}, {1, 1}}}},
        .shader = shader_t{phong_t{}}};
    const primitive_t text{
        .surface = surface_t{text_t{RT2_FONT_FILE, "H", 0.25f}},
        .shader = shader_t{phong_t{}}};
    check_hit_distances(difference_t{cylinder, text},
        ray({0.15f, 0.2f, -1}, {0, 0, 1}), {1.25f, 2});
    check_hit_distances(difference_t{cylinder, text},
        ray({0.4f, 0.2f, -1}, {0, 0, 1}), {1, 2});
}

}
