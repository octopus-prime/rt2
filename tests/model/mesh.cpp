#include <array>
#include <limits>
#include <utility>
#include <vector>
#include <boost/geometry/core/access.hpp>
#include <boost/qvm.hpp>

#include "../support.hpp"

namespace rt2::tests {

void test_mesh() {
    const mesh_t triangle{{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {{0, 1, 2}}};
    const auto bounds = triangle.bounds();
    check(bounds.has_value(), "Mesh bounds required");
    check(boost::geometry::get<boost::geometry::min_corner, 0>(*bounds) <= 0
        && boost::geometry::get<boost::geometry::max_corner, 0>(*bounds) >= 1,
        "Mesh bounds must enclose vertices");
    for (const auto& incoming : std::array{
        ray({0.25f, 0.25f, 2}, {0, 0, -1}),
        ray({0.25f, 0.25f, -2}, {0, 0, 1}),
        ray({0, 0, 2}, {0, 0, -1}),
        ray({0.5f, 0.5f, 2}, {0, 0, -1})}) {
        const auto hit = triangle.intersect(incoming);
        check(hit.has_value(), "Two-sided triangle, edge and vertex hits");
        close(hit->distance, 2, "Triangle distance");
        close(hit->normal.a[2], 1, "Winding normal");
        check(hit->object == nullptr, "Local mesh hit object");
    }
    check(!triangle.intersect(ray({1, 1, 2}, {0, 0, -1})), "Outside triangle");
    check(!triangle.intersect(ray({0.25f, 0.25f, 2}, {1, 0, 0})), "Parallel ray");
    check(!triangle.intersect(ray({0.25f, 0.25f, 0}, {1, 0, 0})), "Coplanar ray");
    check(!triangle.intersect(ray({0.25f, 0.25f, 2}, {0, 0, 0})), "Zero direction");
    check(!triangle.intersect(ray({0.25f, 0.25f, 2}, {0, 0, -1}, 0, 1)), "Finite max");
    check(triangle.intersect(ray({0.25f, 0.25f, 2}, {0, 0, -1}, 2, 2)).has_value(),
        "Inclusive degenerate ray interval");
    const auto scaled_ray_hit = triangle.intersect(ray({0.25f, 0.25f, 2}, {0, 0, -2}));
    check(scaled_ray_hit.has_value(), "Non-normalized direction");
    close(scaled_ray_hit->distance, 1, "Non-normalized ray parameter");

    const mesh_t smooth{{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 2}},
        {{0, 1, 2}, {0, 3, 1}}};
    const auto hit = smooth.intersect(ray({0.25f, 0.25f, 2}, {0, 0, -1}));
    check(hit.has_value(), "Smooth triangle hit");
    const vector_t shared = boost::qvm::normalized(vector_t{0, 2, 1});
    const vector_t expected = boost::qvm::normalized(
        scalar_t{0.75f} * shared + vector_t{0, 0, scalar_t{0.25f}});
    for (int axis = 0; axis < 3; ++axis) {
        close(hit->normal.a[axis], expected.a[axis], "Area-weighted interpolated normal");
    }

    std::vector<vector_t> vertices;
    std::vector<mesh_t::face_t> faces;
    for (int i = 0; i < 40; ++i) {
        const scalar_t z = static_cast<scalar_t>(i);
        const auto start = vertices.size();
        vertices.insert(vertices.end(), {{0, 0, z}, {1, 0, z}, {0, 1, z}});
        faces.push_back({start, start + 1, start + 2});
    }
    const mesh_t layered{std::move(vertices), std::move(faces)};
    close(layered.intersect(ray({0.25f, 0.25f, 50}, {0, 0, -1}))->distance, 11,
        "Multi-node triangle R-tree nearest hit");
    const auto copy = layered;
    close(copy.intersect(ray({0.25f, 0.25f, 50}, {0, 0, -1}, 12))->distance, 12,
        "Copied mesh and ray minimum");

    object_t object{.surface = surface_t{triangle},
        .shader = shader_t{phong_t{
            .ambient = {1, 0, 0}, .diffuse = {0, 0, 0}, .specular = {0, 0, 0}}}};
    object.scale({2, 3, 1}).rotate_x(std::acos(scalar_t{-1}) / 2).translate({0, 2, 0});
    const auto incoming = ray({0.5f, 4, 0.75f}, {0, -1, 0});
    const auto transformed = object.intersect(incoming);
    check(transformed.has_value() && transformed->object == &object, "Mesh object integration");
    close(transformed->distance, 2, "Transformed mesh distance");
    close(transformed->normal.a[1], -1, "Transformed mesh normal");
    const scene_t scene{.objects = {object}};
    check(scene.occluded(incoming), "Scene to mesh nested R-tree query");
    close(scene.trace(incoming, 0).a[0], 1, "Mesh shading through scene");

    const auto rejects = [](std::vector<vector_t> points, std::vector<mesh_t::face_t> triangles) {
        try {
            const mesh_t invalid{std::move(points), std::move(triangles)};
        } catch (const std::invalid_argument&) {
            return;
        }
        throw std::runtime_error("Invalid mesh accepted");
    };
    rejects({}, {});
    rejects({{0, 0, 0}}, {});
    rejects({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {{0, 1, 3}});
    rejects({{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}, {{0, 1, 2}});
    rejects({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {{0, 1, 2}, {0, 2, 1}});
    rejects({{0, 0, 0}, {1, 0, 0}, {0, std::numeric_limits<scalar_t>::infinity(), 0}},
        {{0, 1, 2}});
}

}
