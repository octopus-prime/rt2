#include <array>
#include <numbers>
#include <boost/geometry/core/access.hpp>
#include <boost/qvm.hpp>

#include "../support.hpp"

namespace rt2::tests {

void test_object_bounds() {
    check(ray_t{}.max == ray_max, "Default ray limit");
    check(!plane_t{}.bounds(), "Infinite plane must be unbounded");
    const auto sphere_box = sphere_t{}.bounds();
    check(sphere_box.has_value(), "Sphere bounds");
    close(boost::geometry::get<boost::geometry::min_corner, 0>(*sphere_box), -1,
        "Sphere lower bound");
    const auto torus_box = torus_t{4, scalar_t{0.25f}}.bounds();
    close(boost::geometry::get<boost::geometry::max_corner, 0>(*torus_box),
        scalar_t{2.5f}, "Torus uses square roots of squared radii");
    close(boost::geometry::get<boost::geometry::max_corner, 2>(*torus_box),
        scalar_t{0.5f}, "Torus minor extent");
    const sor_t sor{{{0, 1}, {1, 2}, {2, 1}, {3, 1}}};
    const auto sor_box = sor.bounds();
    close(boost::geometry::get<boost::geometry::max_corner, 0>(*sor_box), 2,
        "SOR radial extent");
    close(boost::geometry::get<boost::geometry::max_corner, 2>(*sor_box), 3,
        "SOR height extent");

    object_t sphere{.surface = surface_t{sphere_t{}}, .shader = shader_t{phong_t{}}};
    check(sphere.scene_bounds.has_value(), "Bounds initialized for identity object");
    sphere.scale({2, 1, 1}).translate({3, 0, 0});
    const auto hit = sphere.intersect(ray({7, 0, 0}, {-1, 0, 0}));
    check(hit.has_value(), "Updated bounds must retain scaled translated sphere");
    close(hit->distance, 2, "Scaled translated sphere distance");
    check(!sphere.intersect(ray({0, 0, 0}, {-1, 0, 0})), "Box behind ray");
    check(!sphere.intersect(ray({7, 0, 0}, {-1, 0, 0}, 0, 1)), "Box beyond ray maximum");
    check(sphere.intersect(ray({7, 0, 0}, {-1, 0, 0}, 2, 2)).has_value(),
        "Degenerate segment on surface");
    check(sphere.intersect(ray({3, 0, 0}, {1, 0, 0})).has_value(), "Origin inside box");
    check(sphere.intersect(ray({3, -2, 1}, {0, 1, 0})).has_value(), "Tangent ray");

    sphere.surface = surface_t{plane_t{}};
    sphere.update_transform();
    check(!sphere.scene_bounds, "Changing to unbounded surface clears cache");
    check(sphere.intersect(ray({500, 500, 2}, {0, 0, -1})).has_value(),
        "Infinite plane bypasses bounds");
    sphere.surface = surface_t{sphere_t{}};
    sphere.scene_object = boost::qvm::identity_mat<scalar_t, 4>();
    sphere.update_transform();
    close(sphere.intersect(ray({0, 0, 3}, {0, 0, -1}))->distance, 2,
        "Explicit cache refresh after direct changes");

    // Compare broad-phase results with exact-only queries under rotation, shear,
    // reflection, nonuniform scale and translation.
    const matrix_t transform{{
        {0, -2, scalar_t{0.25f}, 3},
        {1, 0, scalar_t{0.5f}, -2},
        {0, 0, -scalar_t{1.5f}, 1},
        {0, 0, 0, 1}
    }};
    const std::array<surface_t, 4> surfaces{
        surface_t{sphere_t{}}, surface_t{torus_t{4, scalar_t{0.25f}}},
        surface_t{sor}, surface_t{plane_t{}}};
    for (const auto& surface : surfaces) {
        const object_t bounded{
            .surface = surface, .shader = shader_t{phong_t{}}, .scene_object = transform};
        auto exact = bounded;
        exact.scene_bounds.reset();
        for (int x = -6; x <= 6; ++x) {
            for (int y = -6; y <= 6; ++y) {
                for (int axis = 0; axis < 3; ++axis) {
                    vector_t origin{3, -2, 1};
                    origin.a[axis] += 12;
                    origin.a[(axis + 1) % 3] += static_cast<scalar_t>(x) / 2;
                    origin.a[(axis + 2) % 3] += static_cast<scalar_t>(y) / 2;
                    vector_t direction{0, 0, 0};
                    direction.a[axis] = -1;
                    const auto incoming = ray(origin, direction);
                    const auto expected = exact.intersect(incoming);
                    const auto actual = bounded.intersect(incoming);
                    check(actual.has_value() == expected.has_value(), "AABB changed hit/miss");
                    if (actual) {
                        close(actual->distance, expected->distance, "AABB changed nearest hit");
                    }
                }
            }
        }
    }
}

void test_rotations() {
    using rotation_t = object_t& (object_t::*)(scalar_t);
    const std::array<rotation_t, 3> rotations{
        &object_t::rotate_x, &object_t::rotate_y, &object_t::rotate_z};
    const scalar_t angle = std::numbers::pi_v<scalar_t> / 2;
    for (int axis = 0; axis < 3; ++axis) {
        const int source = (axis + 1) % 3;
        const int target = (axis + 2) % 3;
        vector_t point{0, 0, 0};
        point.a[source] = 1;
        object_t object{.surface = surface_t{sphere_t{}}, .shader = shader_t{phong_t{}}};
        vector_t scale{1, 1, 1};
        scale.a[source] = 2;
        object.scale(scale).translate(point);
        check(&(object.*rotations[axis])(angle) == &object, "Rotation must support chaining");
        const auto position = boost::qvm::transform_point(object.scene_object, point);
        for (int i = 0; i < 3; ++i) {
            close(position.a[i], i == target ? scalar_t{3} : scalar_t{0},
                "Right-handed scene-space rotation");
        }
        const auto restored = boost::qvm::transform_point(object.object_scene, position);
        for (int i = 0; i < 3; ++i) {
            close(restored.a[i], point.a[i], "Rotation inverse cache");
        }
        close(boost::geometry::get<boost::geometry::max_corner, 0>(*object.scene_bounds),
            target == 0 ? scalar_t{3} : scalar_t{1}, "Rotated bounds");
        vector_t origin{0, 0, 0};
        vector_t direction{0, 0, 0};
        origin.a[target] = 5;
        direction.a[target] = -1;
        const auto hit = object.intersect(ray(origin, direction));
        check(hit.has_value(), "Rotated ellipsoid intersection");
        close(hit->distance, 2, "Rotated ellipsoid distance");
        (object.*rotations[axis])(-angle);
        const auto unrotated = boost::qvm::transform_point(object.scene_object, point);
        close(unrotated.a[source], 3, "Negative rotation restores transform");
    }
}

void test_object() {
    test_object_bounds();
    test_rotations();
}

} // namespace rt2::tests
