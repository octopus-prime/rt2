#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

import model;

namespace rt2::tests {

using namespace rt2::model;

inline constexpr scalar_t tolerance = 256 * std::numeric_limits<scalar_t>::epsilon();

inline void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

inline void close(scalar_t actual, scalar_t expected, const char* message) {
    if (!std::isfinite(actual)
        || std::abs(actual - expected) > tolerance * std::max(scalar_t{1}, std::abs(expected))) {
        throw std::runtime_error(std::string(message) + ": expected " + std::to_string(expected)
            + ", got " + std::to_string(actual));
    }
}

inline ray_t ray(vector_t origin, vector_t direction, scalar_t min = 0,
                 scalar_t max = ray_max) {
    return {.origin = origin, .direction = direction, .min = min, .max = max};
}

template<class Surface>
void check_inside_rejects_nonfinite(const Surface& surface) {
    for (const auto value : {std::numeric_limits<scalar_t>::quiet_NaN(),
             std::numeric_limits<scalar_t>::infinity()}) {
        bool threw = false;
        try {
            surface.inside({0, value, 0});
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw, "Inside must reject nonfinite points");
    }
}

} // namespace rt2::tests
