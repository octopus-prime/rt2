#include <numbers>
#include <boost/qvm.hpp>

#include "../support.hpp"

namespace rt2::tests {

void test_intersection() {
    const primitive_t sphere{.surface = surface_t{sphere_t{}},
        .shader = shader_t{phong_t{}}};
    auto other = sphere;
    other.translate({1, 0, 0});
    intersection_t intersection{sphere, other};
    const auto incoming = ray({-3, 0, 0}, {1, 0, 0});
    check_hit_distances(intersection, incoming, {3, 4});
    check(intersection.inside({0.5f, 0, 0}) && !intersection.inside({-0.5f, 0, 0}),
        "Intersection containment");
    check_hit_distances(intersection_t{sphere, sphere}, incoming, {2, 4});
    check_hit_distances(intersection_t{sphere, sphere},
        ray({-3, 1, 0}, {1, 0, 0}), {3});
    auto small = sphere;
    small.scale({0.5f, 0.5f, 0.5f});
    check_hit_distances(intersection_t{sphere, small}, incoming, {2.5f, 3.5f});
    // The containing operand has no hit in this interval, but is still inside.
    check_hit_distances(intersection_t{sphere, small},
        ray({-3, 0, 0}, {1, 0, 0}, 2.25f, 3.75f), {2.5f, 3.5f});
    auto distant = sphere;
    distant.translate({10, 0, 0});
    check_hit_distances(intersection_t{sphere, distant}, incoming, {});
    intersection.translate({0, 2, 0}).scale({2, 1, 1})
        .rotate_x(0).rotate_y(0).rotate_z(0);
    check_hit_distances(intersection, ray({-6, 2, 0}, {1, 0, 0}), {6, 8});
    const scalar_t angle = std::numbers::pi_v<scalar_t> / 2;
    intersection.rotate_x(angle).rotate_y(angle).rotate_z(angle);
    check_hit_distances(intersection, ray({0, 2, 6}, {0, 0, -1}), {6, 8});
    check_inside_rejects_nonfinite(intersection);

    const primitive_t cylinder{
        .surface = surface_t{sor_t{{{-0.1f, 2}, {-0.03f, 2}, {0.03f, 2}, {0.1f, 2}}}},
        .shader = shader_t{phong_t{}}};
    const primitive_t torus{
        .surface = surface_t{torus_t{0.36f, 0.04f}}, .shader = shader_t{phong_t{}}};
    check_hit_distances(intersection_t{cylinder, torus},
        ray({0.6f, 0, -1}, {0, 0, 1}), {0.9f, 1.1f});
    check_hit_distances(intersection_t{cylinder, torus},
        ray({0, 0, -1}, {0, 0, 1}), {});
}

}
