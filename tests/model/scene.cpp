#include "../support.hpp"

namespace rt2::tests {

void test_scene_index() {
    const auto material = [](vector_t color) {
        return shader_t{phong_t{.ambient = color, .diffuse = {0, 0, 0},
            .specular = {0, 0, 0}, .shininess = 1}};
    };
    primitive_t sphere{.surface = surface_t{sphere_t{}}, .shader = material({1, 0, 0})};
    primitive_t torus{.surface = surface_t{torus_t{4, scalar_t{0.25f}}},
        .shader = material({0, 1, 0})};
    torus.translate({4, 0, -2});
    primitive_t sor{.surface = surface_t{sor_t{{{0, 1}, {1, 2}, {2, 1}, {3, 1}}}},
        .shader = material({0, 0, 1})};
    sor.translate({-4, 0, -2});
    primitive_t plane{.surface = surface_t{plane_t{}}, .shader = material({1, 1, 0})};
    plane.translate({0, 0, -5});
    scene_t scene{.objects = {sphere, torus, sor, plane}};
    check(scene.spatial_index.bounded.size() == 3 && scene.spatial_index.unbounded.size() == 1,
        "Index must partition bounded and unbounded objects");
    const auto compare = [&](const scene_t& current, const ray_t& incoming) {
        std::optional<hit_t> nearest;
        for (const auto& object : current.objects) {
            if (const auto hit = nearest_hit(object, incoming)) {
                if (!nearest || hit->distance < nearest->distance) {
                    nearest = hit;
                }
            }
        }
        check(occluded(current, incoming) == nearest.has_value(), "Indexed occlusion differs");
        const auto actual = trace(current, incoming, 0);
        hit_buffer_t buffer;
        const vector_t expected = nearest
            ? nearest->object->eluminate(*nearest, incoming, current, 0, buffer) : vector_t{0, 0, 0};
        for (int i = 0; i < 3; ++i) {
            close(actual.a[i], expected.a[i], "Indexed nearest-hit color differs");
        }
    };
    for (int x = -16; x <= 16; ++x) {
        for (int y = -8; y <= 8; ++y) {
            compare(scene, ray({static_cast<scalar_t>(x) / 2,
                static_cast<scalar_t>(y) / 2, 8}, {0, 0, -1}));
        }
    }
    compare(scene, ray({0, 0, 0}, {1, 0, 0}));
    compare(scene, ray({1, -3, 0}, {0, 1, 0}));
    compare(scene, ray({0, 0, 4}, {0, 0, -1}, 3, 3));
    compare(scene, ray({0, 0, 4}, {0, 0, -1}, 0, 2));
    compare(scene, ray({0, 0, 4}, {0, 0, -1}, 5, 4));
    const scene_t empty{};
    compare(empty, ray({0, 0, 4}, {0, 0, -1}));
    const scene_t planes{.objects = {plane}};
    compare(planes, ray({500, 500, 4}, {0, 0, -1}));

    // Equal-distance hits retain the original object-order preference.
    primitive_t coincident_plane{.surface = surface_t{plane_t{}},
        .shader = material({0, 1, 0})};
    coincident_plane.translate({0, 0, 1});
    const scene_t tied{.objects = {coincident_plane, sphere}};
    compare(tied, ray({0, 0, 4}, {0, 0, -1}));

    auto copy = scene;
    copy.objects[0].translate({10, 0, 0});
    copy.objects.erase(copy.objects.begin() + 1);
    copy.objects.push_back(sphere);
    copy.rebuild_index();
    compare(copy, ray({10, 0, 4}, {0, 0, -1}));
    compare(copy, ray({0, 0, 4}, {0, 0, -1}));
    compare(scene, ray({0, 0, 4}, {0, 0, -1}));
    copy.objects.clear();
    copy.rebuild_index();
    compare(copy, ray({0, 0, 4}, {0, 0, -1}));

    scene_t many{};
    for (int i = 0; i < 80; ++i) {
        auto object = sphere;
        object.translate({static_cast<scalar_t>(i % 10) * 3,
            static_cast<scalar_t>(i / 10) * 3, -static_cast<scalar_t>(i % 3)});
        many.objects.push_back(object);
    }
    many.rebuild_index();
    for (int i = 0; i < 80; ++i) {
        compare(many, ray({static_cast<scalar_t>(i % 10) * 3,
            static_cast<scalar_t>(i / 10) * 3, 8}, {0, 0, -1}));
    }
}

} // namespace rt2::tests
