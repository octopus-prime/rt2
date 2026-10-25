#include <array>
#include <limits>
#include <thread>
#include <utility>
#include <vector>
#include <boost/geometry/core/access.hpp>
#include <boost/qvm.hpp>

#include "../support.hpp"

namespace rt2::tests {
namespace {

mesh_t cube(bool reverse = false, bool open = false, bool cavity = false) {
    std::vector<vector_t> vertices{
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}};
    std::vector<mesh_t::face_t> faces{
        {0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7},
        {0, 1, 5}, {0, 5, 4}, {3, 7, 6}, {3, 6, 2},
        {0, 4, 7}, {0, 7, 3}, {1, 2, 6}, {1, 6, 5}};
    if (open) {
        faces.pop_back();
    }
    if (reverse) {
        for (auto& face : faces) {
            std::swap(face[1], face[2]);
        }
    }
    if (cavity) {
        const auto outer_vertices = vertices;
        const auto outer_faces = faces;
        for (const auto& vertex : outer_vertices) {
            vertices.push_back(vertex * scalar_t{0.5f});
        }
        for (const auto& face : outer_faces) {
            faces.push_back({face[0] + 8, face[2] + 8, face[1] + 8});
        }
    }
    return {std::move(vertices), std::move(faces)};
}

void test_mesh_inside() {
    const auto mesh = cube();
    check_hit_distances(mesh, ray({0, 0, -3}, {0, 0, 1}), {2, 4});
    const auto reversed = cube(true);
    for (const auto& point : std::array<vector_t, 4>{
            vector_t{0, 0, 0}, {0.2f, 0.3f, 0.4f}, {-0.9f, 0.5f, 0.5f}, {0, -0.5f, 0.5f}}) {
        check(mesh.inside(point) && reversed.inside(point), "Mesh interior and reversed winding");
    }
    for (const auto& point : std::array<vector_t, 9>{
            vector_t{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, 0, -1},
            {1, 1, 0}, {1, 1, 1}, {-2, 1, 1}, {-2, 1, 0}, {-2, 0, 0}}) {
        check(!mesh.inside(point), "Mesh faces, edges, vertices and tangent rays excluded");
    }
    const auto open = cube(false, true);
    for (int i = 0; i < 2; ++i) {
        bool threw = false;
        try {
            open.inside({0, 0, 0});
        } catch (const std::logic_error&) {
            threw = true;
        }
        check(threw, "Open mesh reports unsupported inside, including cached validation");
    }
    check(nearest_hit(open, ray({0, 0, -3}, {0, 0, 1})).has_value(),
        "Open mesh still renders");
    const auto hollow = cube(false, false, true);
    check(!hollow.inside({0, 0, 0}) && hollow.inside({0.75f, 0, 0})
        && !hollow.inside({0.5f, 0, 0}), "Mesh cavity and inner boundary");
    for (int i = -13; i <= 13; ++i) {
        for (int j = -13; j <= 13; ++j) {
            const vector_t point{static_cast<scalar_t>(i) / 10,
                static_cast<scalar_t>(j) / 10, scalar_t{0.137f}};
            check(mesh.inside(point) == (std::abs(i) < 10 && std::abs(j) < 10),
                "Mesh containment matches analytical box");
        }
    }
    const auto shared = cube();
    const auto shared_copy = shared;
    std::array<bool, 2> answers{};
    std::thread first{[&] { answers[0] = shared.inside({0, 0, 0}); }};
    std::thread second{[&] { answers[1] = shared_copy.inside({0, 0, 0}); }};
    first.join();
    second.join();
    check(answers[0] && answers[1], "Shared mesh solid validation is thread-safe");
    check_inside_rejects_nonfinite(mesh);
}

}

void test_mesh() {
    test_mesh_inside();
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
        const auto hit = nearest_hit(triangle, incoming);
        check(hit.has_value(), "Two-sided triangle, edge and vertex hits");
        close(hit->distance, 2, "Triangle distance");
        close(hit->normal.a[2], 1, "Winding normal");
        check(hit->object == nullptr, "Local mesh hit object");
    }
    check(!nearest_hit(triangle, ray({1, 1, 2}, {0, 0, -1})), "Outside triangle");
    check(!nearest_hit(triangle, ray({0.25f, 0.25f, 2}, {1, 0, 0})), "Parallel ray");
    check(!nearest_hit(triangle, ray({0.25f, 0.25f, 0}, {1, 0, 0})), "Coplanar ray");
    check(!nearest_hit(triangle, ray({0.25f, 0.25f, 2}, {0, 0, 0})), "Zero direction");
    check(!nearest_hit(triangle, ray({0.25f, 0.25f, 2}, {0, 0, -1}, 0, 1)), "Finite max");
    check(nearest_hit(triangle, ray({0.25f, 0.25f, 2}, {0, 0, -1}, 2, 2)).has_value(),
        "Inclusive degenerate ray interval");
    const auto scaled_ray_hit = nearest_hit(triangle, ray({0.25f, 0.25f, 2}, {0, 0, -2}));
    check(scaled_ray_hit.has_value(), "Non-normalized direction");
    close(scaled_ray_hit->distance, 1, "Non-normalized ray parameter");

    const mesh_t smooth{{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 2}},
        {{0, 1, 2}, {0, 3, 1}}};
    const auto hit = nearest_hit(smooth, ray({0.25f, 0.25f, 2}, {0, 0, -1}));
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
    close(nearest_hit(layered, ray({0.25f, 0.25f, 50}, {0, 0, -1}))->distance, 11,
        "Multi-node triangle R-tree nearest hit");
    const auto copy = layered;
    close(nearest_hit(copy, ray({0.25f, 0.25f, 50}, {0, 0, -1}, 12))->distance, 12,
        "Copied mesh and ray minimum");

    primitive_t object{.surface = surface_t{triangle},
        .shader = shader_t{phong_t{
            .ambient = {1, 0, 0}, .diffuse = {0, 0, 0}, .specular = {0, 0, 0}}}};
    object.scale({2, 3, 1}).rotate_x(std::acos(scalar_t{-1}) / 2).translate({0, 2, 0});
    const auto incoming = ray({0.5f, 4, 0.75f}, {0, -1, 0});
    const auto transformed = nearest_hit(object, incoming);
    check(transformed.has_value() && transformed->object == &object, "Mesh object integration");
    close(transformed->distance, 2, "Transformed mesh distance");
    close(transformed->normal.a[1], -1, "Transformed mesh normal");
    const scene_t scene{.objects = {object}};
    check(occluded(scene, incoming), "Scene to mesh nested R-tree query");
    close(trace(scene, incoming, 0).a[0], 1, "Mesh shading through scene");
    auto rotated_copy = object;
    rotated_copy.rotate_y(std::acos(scalar_t{-1}) / 2).translate({4, 0, 0});
    const auto copy_ray = ray({scalar_t{4.75f}, 4, scalar_t{-0.5f}}, {0, -1, 0});
    const auto copy_hit = nearest_hit(rotated_copy, copy_ray);
    check(copy_hit.has_value() && copy_hit->object == &rotated_copy,
        "Shared mesh instance has independent transform");
    close(copy_hit->distance, 2, "Rotated shared mesh distance");
    check(nearest_hit(object, incoming).has_value(), "Original mesh instance is unchanged");
    const scene_t instances{.objects = {object, rotated_copy}};
    check(occluded(instances, incoming) && occluded(instances, copy_ray),
        "Scene indexes both shared mesh instances");

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
