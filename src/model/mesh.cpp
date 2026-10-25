module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/qvm.hpp>

module model;

namespace rt2::model {

struct mesh_t::data_t {
    std::vector<vector_t> vertices;
    std::vector<face_t> faces;
    std::vector<vector_t> normals;
    boost::geometry::index::rtree<std::pair<box3_t, std::size_t>,
        boost::geometry::index::quadratic<8>> triangles;
    box3_t bounds;
    mutable std::once_flag solid_check;
    mutable bool closed = false;
};

bool finite(const vector_t& value) {
    return std::isfinite(value.a[0]) && std::isfinite(value.a[1])
        && std::isfinite(value.a[2]);
}

box3_t padded_box(const vector_t& lower, const vector_t& upper) {
    vector_t min = lower;
    vector_t max = upper;
    for (int axis = 0; axis < 3; ++axis) {
        const scalar_t padding = 16 * std::numeric_limits<scalar_t>::epsilon()
            * std::max({scalar_t{1}, std::abs(min.a[axis]), std::abs(max.a[axis])});
        min.a[axis] -= padding;
        max.a[axis] += padding;
    }
    return {point3_t{min.a[0], min.a[1], min.a[2]},
        point3_t{max.a[0], max.a[1], max.a[2]}};
}

mesh_t::mesh_t(std::vector<vector_t> vertices, std::vector<face_t> faces) {
    if (vertices.empty() || faces.empty()) {
        throw std::invalid_argument("Mesh requires vertices and triangle faces");
    }
    for (const auto& vertex : vertices) {
        if (!finite(vertex)) {
            throw std::invalid_argument("Mesh vertices must be finite");
        }
    }
    auto result = std::make_shared<data_t>();
    result->vertices = std::move(vertices);
    result->faces = std::move(faces);
    result->normals.resize(result->vertices.size(), vector_t{0, 0, 0});
    std::vector<bool> used(result->vertices.size(), false);
    std::vector<std::pair<box3_t, std::size_t>> boxes;
    boxes.reserve(result->faces.size());
    vector_t lower = result->vertices.front();
    vector_t upper = lower;
    for (std::size_t i = 0; i < result->faces.size(); ++i) {
        const auto& face = result->faces[i];
        for (auto index : face) {
            if (index >= result->vertices.size()) {
                throw std::invalid_argument("Mesh face index is out of range");
            }
        }
        const auto& p0 = result->vertices[face[0]];
        const auto& p1 = result->vertices[face[1]];
        const auto& p2 = result->vertices[face[2]];
        const vector_t normal = boost::qvm::cross(p1 - p0, p2 - p0);
        const scalar_t length = boost::qvm::mag(normal);
        if (!finite(normal) || !std::isfinite(length) || length == 0) {
            throw std::invalid_argument("Mesh faces must have finite nonzero area");
        }
        vector_t triangle_min = p0;
        vector_t triangle_max = p0;
        for (auto index : face) {
            used[index] = true;
            result->normals[index] += normal;
            for (int axis = 0; axis < 3; ++axis) {
                triangle_min.a[axis] = std::min(triangle_min.a[axis], result->vertices[index].a[axis]);
                triangle_max.a[axis] = std::max(triangle_max.a[axis], result->vertices[index].a[axis]);
            }
        }
        for (int axis = 0; axis < 3; ++axis) {
            lower.a[axis] = std::min(lower.a[axis], triangle_min.a[axis]);
            upper.a[axis] = std::max(upper.a[axis], triangle_max.a[axis]);
        }
        boxes.emplace_back(padded_box(triangle_min, triangle_max), i);
    }
    for (std::size_t i = 0; i < result->normals.size(); ++i) {
        if (!used[i]) {
            continue;
        }
        const scalar_t length = boost::qvm::mag(result->normals[i]);
        if (!std::isfinite(length) || length == 0) {
            throw std::invalid_argument("Mesh vertex normals cancel or overflow; check face winding");
        }
        result->normals[i] /= length;
    }
    result->bounds = padded_box(lower, upper);
    result->triangles = decltype(result->triangles)(boxes.begin(), boxes.end());
    data = std::move(result);
}

std::optional<box3_t> mesh_t::bounds() const {
    return data->bounds;
}

bool mesh_t::inside(const vector_t& point) const {
    validate_point(point);
    std::call_once(data->solid_check, [&] {
        // Do not impose solid topology on meshes used only for rendering.
        std::vector<std::array<std::size_t, 3>> edges;
        edges.reserve(data->faces.size() * 3);
        for (const auto& face : data->faces) {
            for (int i = 0; i < 3; ++i) {
                const auto a = face[i];
                const auto b = face[(i + 1) % 3];
                edges.push_back({std::min(a, b), std::max(a, b), a < b ? 0u : 1u});
            }
        }
        std::ranges::sort(edges);
        bool closed = edges.size() % 2 == 0;
        for (std::size_t i = 0; closed && i < edges.size(); i += 2) {
            closed = edges[i][0] == edges[i + 1][0]
                && edges[i][1] == edges[i + 1][1]
                && edges[i][2] != edges[i + 1][2]
                && (i + 2 == edges.size() || edges[i][0] != edges[i + 2][0]
                    || edges[i][1] != edges[i + 2][1]);
        }
        data->closed = closed;
    });
    if (!data->closed) {
        throw std::logic_error("Mesh inside requires closed, consistently wound indexed edges");
    }

    const auto near = boost::geometry::index::intersects(padded_box(point, point));
    for (auto it = data->triangles.qbegin(near); it != data->triangles.qend(); ++it) {
        const auto& face = data->faces[it->second];
        const auto& a = data->vertices[face[0]];
        const vector_t u = data->vertices[face[1]] - a;
        const vector_t v = data->vertices[face[2]] - a;
        const vector_t w = point - a;
        const vector_t normal = boost::qvm::cross(u, v);
        const scalar_t length = boost::qvm::mag(normal);
        const scalar_t epsilon = 16 * std::numeric_limits<scalar_t>::epsilon();
        const scalar_t distance_tolerance = epsilon * std::max({scalar_t{1},
            boost::qvm::mag(u), boost::qvm::mag(v), boost::qvm::mag(a)});
        if (std::abs(boost::qvm::dot(w, normal)) > distance_tolerance * length) {
            continue;
        }
        const scalar_t denominator = boost::qvm::mag_sqr(normal);
        const scalar_t s = boost::qvm::dot(boost::qvm::cross(w, v), normal) / denominator;
        const scalar_t t = boost::qvm::dot(boost::qvm::cross(u, w), normal) / denominator;
        if (s >= -epsilon && t >= -epsilon && s + t <= 1 + epsilon) {
            return false;
        }
    }

    const scalar_t end_x =
        boost::geometry::get<boost::geometry::max_corner, 0>(data->bounds);
    if (point.a[0] >= end_x) {
        return false;
    }
    const boost::geometry::model::segment<point3_t> segment{
        {point.a[0], point.a[1], point.a[2]}, {end_x, point.a[1], point.a[2]}};
    const auto candidates = boost::geometry::index::intersects(segment);
    bool parity = false;
    for (auto it = data->triangles.qbegin(candidates); it != data->triangles.qend(); ++it) {
        const auto& face = data->faces[it->second];
        auto a = data->vertices[face[0]];
        auto b = data->vertices[face[1]];
        auto c = data->vertices[face[2]];
        const auto edge = [](const vector_t& p, const vector_t& q, const vector_t& r) {
            return (q.a[1] - p.a[1]) * (r.a[2] - p.a[2])
                - (q.a[2] - p.a[2]) * (r.a[1] - p.a[1]);
        };
        scalar_t area = edge(a, b, c);
        if (area == 0) {
            continue;
        }
        if (area < 0) {
            std::swap(b, c);
            area = -area;
        }
        // Rasterization's top-left rule assigns projected shared edges/vertices
        // consistently, including silhouette tangencies, without merging hits.
        const auto accepts = [](scalar_t value, const vector_t& p, const vector_t& q) {
            return value > 0 || (value == 0 && (q.a[2] > p.a[2]
                || (q.a[2] == p.a[2] && q.a[1] < p.a[1])));
        };
        const scalar_t ab = edge(a, b, point);
        const scalar_t bc = edge(b, c, point);
        const scalar_t ca = edge(c, a, point);
        if (accepts(ab, a, b) && accepts(bc, b, c) && accepts(ca, c, a)) {
            const scalar_t x = (bc * a.a[0] + ca * b.a[0] + ab * c.a[0]) / area;
            if (x > point.a[0]) {
                parity = !parity;
            }
        }
    }
    return parity;
}

std::optional<hit_t> mesh_t::intersect(const ray_t& ray) const {
    const auto segment = ray.segment();
    if (ray.min > ray.max) {
        return std::nullopt;
    }
    std::optional<hit_t> nearest;
    std::size_t nearest_face = data->faces.size();
    const auto predicate = boost::geometry::index::intersects(segment);
    for (auto it = data->triangles.qbegin(predicate); it != data->triangles.qend(); ++it) {
        const auto index = it->second;
        const auto& face = data->faces[index];
        const auto& p0 = data->vertices[face[0]];
        const vector_t e1 = data->vertices[face[1]] - p0;
        const vector_t e2 = data->vertices[face[2]] - p0;
        const vector_t p = boost::qvm::cross(ray.direction, e2);
        const scalar_t determinant = boost::qvm::dot(e1, p);
        if (determinant == 0) {
            continue;
        }
        const vector_t offset = ray.origin - p0;
        const scalar_t u = boost::qvm::dot(offset, p) / determinant;
        const vector_t q = boost::qvm::cross(offset, e1);
        const scalar_t v = boost::qvm::dot(ray.direction, q) / determinant;
        if (!(u >= 0 && v >= 0 && u + v <= 1)) {
            continue;
        }
        const scalar_t t = boost::qvm::dot(e2, q) / determinant;
        if (!std::isfinite(t) || !ray[t] || (nearest && (t > nearest->distance
            || (t == nearest->distance && index >= nearest_face)))) {
            continue;
        }
        const vector_t normal = (1 - u - v) * data->normals[face[0]]
            + u * data->normals[face[1]] + v * data->normals[face[2]];
        if (!finite(normal) || boost::qvm::mag_sqr(normal) == 0) {
            throw std::runtime_error("Mesh interpolated normal is invalid; check face winding");
        }
        nearest = hit_t{.point = ray(t), .normal = boost::qvm::normalized(normal),
            .distance = t, .object = nullptr};
        nearest_face = index;
    }
    return nearest;
}

}
