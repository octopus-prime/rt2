module;

// #include <optional>
// #include <boost/qvm/vec.hpp>
// #include <boost/qvm/mat.hpp>
// #include <boost/qvm/vec_operations.hpp>

export module model:hit;

import :math;
import :fwd;

export namespace rt2::model {

struct hit_t {
    vector_t point;
    vector_t normal;
    scalar_t distance;
    const object_t* object;
};

// template <typename T>
// concept Intersectable = requires(T t, const ray_t& ray) {
//     { t.intersect(ray) } -> std::same_as<std::optional<hit_t>>;
// };

// template <typename T>
// concept Transformable = requires(T t, const vector_t& vector) {
//     { t.translate(vector) } -> std::same_as<void>;
//     { t.scale(vector) } -> std::same_as<void>;
// };

}
