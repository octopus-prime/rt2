module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include <boost/geometry/algorithms/intersects.hpp>
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
