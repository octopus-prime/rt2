module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>
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
    const primitive_t* object;
    // Scratch provenance for sorting scene candidates or CSG operands.
    std::size_t source_index = 0;
};

struct hit_buffer_overflow : std::length_error {
    hit_buffer_overflow() : std::length_error("Intersection hit buffer is too small") {}
};

inline void append_hit(std::span<hit_t> hits, std::size_t& count, const hit_t& hit) {
    if (count == hits.size()) {
        throw hit_buffer_overflow{};
    }
    hits[count++] = hit;
}

// Sort in ray order, removing duplicate roots/triangle-edge contacts.
inline std::size_t finish_hits(std::span<hit_t> hits, std::size_t count) {
    auto filled = hits.first(count);
    std::sort(filled.begin(), filled.end(),
        [](const auto& a, const auto& b) {
            return a.distance < b.distance || (a.distance == b.distance
                && a.source_index < b.source_index);
        });
    const auto end = std::unique(filled.begin(), filled.end(), [](const auto& a, const auto& b) {
        return a.distance == b.distance;
    });
    return static_cast<std::size_t>(end - filled.begin());
}

// Owned by the renderer worker and reused by primary, shadow and recursive rays.
// Callers must copy hits they retain before another query reuses this storage.
struct hit_buffer_t {
    std::vector<hit_t> storage = std::vector<hit_t>(64);

    template<class Intersectable, class Ray>
    std::span<hit_t> intersect(const Intersectable& object, const Ray& ray) {
        for (;;) {
            try {
                const auto count = object.intersect(ray, std::span<hit_t>{storage});
                return std::span<hit_t>{storage}.first(count);
            } catch (const hit_buffer_overflow&) {
                if (storage.size() > storage.max_size() / 2) {
                    throw std::length_error("Intersection hit buffer cannot grow further");
                }
                storage.resize(storage.empty() ? 64 : storage.size() * 2);
            }
        }
    }
};

// template <typename T>
// concept Transformable = requires(T t, const vector_t& vector) {
//     { t.translate(vector) } -> std::same_as<void>;
//     { t.scale(vector) } -> std::same_as<void>;
// };

}
