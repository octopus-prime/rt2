module;

#include <array>
#include <optional>
#include <vector>

export module model:sor;

import :math;
import :ray;
import :hit;

export namespace rt2::model {

// Closed surface of revolution around the local z axis.
class sor_t {
public:
    // Samples are {height, radius}, with increasing heights and positive radii.
    explicit sor_t(const std::vector<std::array<scalar_t, 2>>& profile);

    std::optional<hit_t> intersect(const ray_t& ray) const;

private:
    struct segment_t {
        scalar_t lower;
        scalar_t upper;
        // Radius polynomial in z - lower, in ascending coefficient order.
        std::array<scalar_t, 4> radius;
    };

    std::vector<segment_t> segments;
    scalar_t max_radius;
};

} // namespace rt2::model
