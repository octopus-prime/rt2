module;

#include <boost/qvm/vec.hpp>
#include <boost/qvm/mat.hpp>

export module model:math;

export namespace rt2::model {

using scalar_t = float;
using vector_t = boost::qvm::vec<scalar_t, 3>;
using matrix_t = boost::qvm::mat<scalar_t, 4, 4>;

constexpr vector_t operator*(const vector_t& lhs, const vector_t rhs) {
    return vector_t{lhs.a[0] * rhs.a[0], lhs.a[1] * rhs.a[1], lhs.a[2] * rhs.a[2]};
}

}
