#include "../support.hpp"

namespace rt2::tests {

void test_union() {
    const primitive_t sphere{.surface = surface_t{sphere_t{}},
        .shader = shader_t{phong_t{}}};
    auto other = sphere;
    other.translate({1, 0, 0});
    union_t united{sphere, other};
    const auto incoming = ray({-3, 0, 0}, {1, 0, 0});
    check_hit_distances(united, incoming, {2, 5});
    check(united.inside({-0.5f, 0, 0}) && united.inside({1.5f, 0, 0})
        && !united.inside({4, 0, 0}), "Union containment");
    check_hit_distances(union_t{sphere, sphere}, incoming, {2, 4});
    check_hit_distances(union_t{sphere, sphere}, ray({-3, 1, 0}, {1, 0, 0}), {3});
    auto small = sphere;
    small.scale({0.5f, 0.5f, 0.5f});
    check_hit_distances(union_t{sphere, small}, incoming, {2, 4});
    auto distant = sphere;
    distant.translate({4, 0, 0});
    check_hit_distances(union_t{sphere, distant}, incoming, {2, 4, 6, 8});
    auto touching = sphere;
    touching.translate({2, 0, 0});
    check_hit_distances(union_t{sphere, touching}, incoming, {2, 6});
    united.translate({0, 2, 0}).scale({2, 1, 1})
        .rotate_x(0).rotate_y(0).rotate_z(0);
    check_hit_distances(united, ray({-6, 2, 0}, {1, 0, 0}), {4, 10});
    const scalar_t angle = std::numbers::pi_v<scalar_t> / 2;
    united.rotate_x(angle).rotate_y(angle).rotate_z(angle).scale({1, -1, 1});
    check_hit_distances(united, ray({0, -2, 6}, {0, 0, -1}), {4, 10});
    check_inside_rejects_nonfinite(united);

    const primitive_t cylinder{
        .surface = surface_t{sor_t{{{0, 1}, {0.3f, 1}, {0.7f, 1}, {1, 1}}}},
        .shader = shader_t{phong_t{}}};
    auto text = primitive_t{
        .surface = surface_t{text_t{RT2_FONT_FILE, "H", 1}},
        .shader = shader_t{phong_t{}}};
    text.translate({0, 0, 0.5f});
    check_hit_distances(union_t{cylinder, text},
        ray({0.15f, 0.2f, -1}, {0, 0, 1}), {1, 2.5f});
    check_hit_distances(union_t{cylinder, text},
        ray({0.4f, 0.2f, -1}, {0, 0, 1}), {1, 2});
}

}
#include <numbers>
