#include <numbers>
#include <variant>
#include <boost/qvm.hpp>

#include "../support.hpp"

namespace rt2::tests {

void test_object() {
    object_t object{primitive_t{
        .surface = surface_t{sphere_t{}},
        .shader = shader_t{phong_t{
            .ambient = {1, 0, 0}, .diffuse = {0, 0, 0}, .specular = {0, 0, 0}}}}};
    check(std::holds_alternative<primitive_t>(object.variant), "Object holds primitive");
    check(object.bounds().has_value(), "Object forwards primitive bounds");
    check(&object.scale({2, 1, 1}).rotate_x(0).rotate_y(0)
        .rotate_z(std::numbers::pi_v<scalar_t> / 2).translate({3, 0, 0}) == &object,
        "Object transformations chain");
    check(object.inside({3, 1, 0}) && !object.inside({5, 0, 0}),
        "Object forwards transformed containment");
    const auto incoming = ray({3, 4, 0}, {0, -1, 0});
    const auto hit = nearest_hit(object, incoming);
    check(hit.has_value() && hit->object == &std::get<primitive_t>(object.variant),
        "Object hit refers to contained primitive");
    close(hit->distance, 2, "Object forwards intersection");
    auto copy = object;
    copy.translate({10, 0, 0});
    check(nearest_hit(object, incoming).has_value() && !nearest_hit(copy, incoming),
        "Object copies have independent transforms");
    auto& primitive = std::get<primitive_t>(copy.variant);
    primitive.scene_object = boost::qvm::identity_mat<scalar_t, 4>();
    copy.update_transform();
    check(copy.inside({0, 0, 0}), "Object refreshes primitive caches");
    const scene_t scene{.objects = {object, copy}};
    close(trace(scene, incoming, 0).a[0], 1, "Scene shades contained primitive");
    check_inside_rejects_nonfinite(object);
    object.variant = primitive_t{.surface = surface_t{plane_t{}},
        .shader = shader_t{phong_t{}}};
    check(!object.bounds(), "Object forwards unbounded primitive");

    const primitive_t solid{.surface = surface_t{sphere_t{}},
        .shader = shader_t{phong_t{
            .ambient = {1, 0, 0}, .diffuse = {0, 0, 0}, .specular = {0, 0, 0}}}};
    auto cutter = solid;
    cutter.scale({0.5f, 0.5f, 0.5f});
    auto distant = solid;
    distant.translate({4, 0, 0});
    auto envelope = solid;
    envelope.scale({10, 10, 10});
    object_t nested{intersection_t{
        union_t{difference_t{solid, cutter}, distant}, envelope}};
    const auto along_x = ray({-3, 0, 0}, {1, 0, 0});
    check_hit_distances(nested, along_x, {2, 2.5f, 3.5f, 4, 6, 8});
    check(nested.inside({0.75f, 0, 0}) && !nested.inside({0, 0, 0})
        && nested.inside({4, 0, 0}), "Nested CSG containment");
    const scene_t csg_scene{.objects = {nested}};
    check(csg_scene.spatial_index.bounded.size() == 1, "Nested CSG scene bounds");
    check_hit_distances(csg_scene, along_x, {2, 2.5f, 3.5f, 4, 6, 8});
    check(occluded(csg_scene, along_x), "Nested CSG shadows");
    close(trace(csg_scene, along_x, 0).a[0], 1, "Nested CSG leaf shading");
    auto nested_copy = nested;
    nested_copy.translate({0, 3, 0});
    check(!nested_copy.inside({0.75f, 0, 0}) && nested.inside({0.75f, 0, 0}),
        "Recursive wrapper deep copies CSG nodes");
    check_hit_distances(nested_copy, ray({-3, 3, 0}, {1, 0, 0}),
        {2, 2.5f, 3.5f, 4, 6, 8});
    object_t moved = std::move(nested_copy);
    auto moved_scene = scene_t{.objects = {moved}};
    auto copied_scene = moved_scene;
    copied_scene.objects[0].translate({0, 2, 0});
    copied_scene.rebuild_index();
    check_hit_distances(copied_scene, ray({-3, 5, 0}, {1, 0, 0}),
        {2, 2.5f, 3.5f, 4, 6, 8});
    check_hit_distances(moved_scene, ray({-3, 3, 0}, {1, 0, 0}),
        {2, 2.5f, 3.5f, 4, 6, 8});
    const primitive_t half_space{.surface = surface_t{plane_t{}},
        .shader = solid.shader};
    check(!object_t{union_t{solid, half_space}}.bounds(), "Unbounded union");
    check(object_t{intersection_t{solid, half_space}}.bounds().has_value(),
        "Bounded intersection with half-space");
    check(object_t{difference_t{solid, half_space}}.bounds().has_value(),
        "Difference retains minuend bounds");
    const scene_t unbounded_csg{.objects = {union_t{solid, half_space}}};
    check(unbounded_csg.spatial_index.unbounded.size() == 1,
        "Scene indexes unbounded CSG separately");
}

}
