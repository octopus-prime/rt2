module;

// #include <boost/qvm/vec.hpp>
// #include <boost/qvm/mat.hpp>
#include <boost/qvm/vec_operations.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <cmath>
#include <stdexcept>

export module model:ray;

import :math;

export namespace rt2::model {

// Maximum ray parameter; a world-space distance when direction is normalized.
inline constexpr scalar_t ray_max = 1000;

struct ray_t {
    vector_t origin;
    vector_t direction;
    scalar_t min;
    scalar_t max = ray_max;

    constexpr bool operator[](const scalar_t distance) const {
        return distance >= min && distance <= max;
    }

    constexpr vector_t operator()(const scalar_t distance) const {
        return origin + distance * direction;
    }

    boost::geometry::model::segment<point3_t> segment() const {
        // if (!std::isfinite(min) || !std::isfinite(max)) {
        //     throw std::invalid_argument("Ray segment requires finite ray limits");
        // }
        const auto begin = (*this)(min);
        const auto end = (*this)(max);
        return {point3_t{begin.a[0], begin.a[1], begin.a[2]},
            point3_t{end.a[0], end.a[1], end.a[2]}};
    }
};

inline constexpr scalar_t ray_epsilon = 1e-2f;

}
