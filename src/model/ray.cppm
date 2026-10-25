module;

// #include <boost/qvm/vec.hpp>
// #include <boost/qvm/mat.hpp>
#include <boost/qvm/vec_operations.hpp>

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
};

inline constexpr scalar_t ray_epsilon = 1e-2f;

}
