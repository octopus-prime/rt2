module;

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <cstddef>
#include <vector>

export module model:mesh;

import :math;
import :ray;
import :hit;

export namespace rt2::model {

struct mesh_t {
    using face_t = std::array<std::size_t, 3>;

    // Copies share immutable vertices, normals and the triangle index.
    // Counterclockwise winding defines outward normals. Shared vertices are
    // smoothed with area-weighted normals; duplicate vertices for sharp edges.
    // Empty, non-finite, degenerate or inconsistently cancelling geometry is rejected.
    mesh_t(std::vector<vector_t> vertices, std::vector<face_t> faces);

    std::optional<box3_t> bounds() const;
    // Strict interior of a non-self-intersecting closed solid (even-odd fill).
    // Lazily checks indexed edges for paired opposite winding; otherwise throws
    // std::logic_error. Self-intersections are a caller precondition, not checked.
    // Duplicate seam vertices must be welded for solid queries, not for rendering.
    bool inside(const vector_t& point) const;
    // Two-sided intersection; normals retain the mesh's winding orientation.
    std::size_t intersect(const ray_t& ray, std::span<hit_t> hits) const;

private:
    struct data_t;
    std::shared_ptr<const data_t> data;
};

}
