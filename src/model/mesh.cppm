module;

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
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
    // Two-sided intersection; normals retain the mesh's winding orientation.
    std::optional<hit_t> intersect(const ray_t& ray) const;

private:
    struct data_t;
    std::shared_ptr<const data_t> data;
};

}
