module;

#include <cmath>
#include <stdexcept>
#include <boost/qvm/vec.hpp>
#include <boost/qvm/mat.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

export module model:math;

export namespace rt2::model {

using scalar_t = float;
using vector_t = boost::qvm::vec<scalar_t, 3>;
using matrix_t = boost::qvm::mat<scalar_t, 4, 4>;
using point3_t = boost::geometry::model::point<scalar_t, 3, boost::geometry::cs::cartesian>;
using box3_t = boost::geometry::model::box<point3_t>;

inline void validate_point(const vector_t& point) {
    for (const auto value : point.a) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Inside query requires a finite point");
        }
    }
}

constexpr vector_t operator*(const vector_t& lhs, const vector_t rhs) {
    return vector_t{lhs.a[0] * rhs.a[0], lhs.a[1] * rhs.a[1], lhs.a[2] * rhs.a[2]};
}

}
