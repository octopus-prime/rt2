module;

#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <type_traits>

module model;

namespace rt2::model {
namespace {

template<class T>
decltype(auto) node(T& value) {
    if constexpr (std::is_same_v<std::remove_cv_t<T>, primitive_t>) {
        return (value);
    } else {
        return value.get();
    }
}

}

object_t::object_t(primitive_t primitive) : variant(std::move(primitive)) {}
object_t::object_t(intersection_t intersection) : variant(std::move(intersection)) {}
object_t::object_t(union_t united) : variant(std::move(united)) {}
object_t::object_t(difference_t difference) : variant(std::move(difference)) {}
object_t::object_t(const object_t&) = default;
object_t::object_t(object_t&&) = default;
object_t& object_t::operator=(const object_t&) = default;
object_t& object_t::operator=(object_t&&) = default;
object_t::~object_t() = default;

std::optional<box3_t> object_t::bounds() const {
    return std::visit([](const auto& value) -> std::optional<box3_t> {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, primitive_t>) {
            return value.scene_bounds;
        } else {
            return value.get().bounds();
        }
    }, variant);
}

void object_t::update_transform() {
    std::visit([](auto& value) { node(value).update_transform(); }, variant);
}

object_t& object_t::translate(const vector_t& offset) {
    std::visit([&](auto& value) { node(value).translate(offset); }, variant);
    return *this;
}

object_t& object_t::scale(const vector_t& factors) {
    std::visit([&](auto& value) { node(value).scale(factors); }, variant);
    return *this;
}

object_t& object_t::rotate_x(scalar_t angle) {
    std::visit([&](auto& value) { node(value).rotate_x(angle); }, variant);
    return *this;
}

object_t& object_t::rotate_y(scalar_t angle) {
    std::visit([&](auto& value) { node(value).rotate_y(angle); }, variant);
    return *this;
}

object_t& object_t::rotate_z(scalar_t angle) {
    std::visit([&](auto& value) { node(value).rotate_z(angle); }, variant);
    return *this;
}

std::size_t object_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    return std::visit([&](const auto& value) { return node(value).intersect(ray, hits); }, variant);
}

std::size_t object_t::intersect_surface(const ray_t& ray, std::span<hit_t> hits) const {
    return std::visit([&](const auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, primitive_t>) {
            return value.intersect_surface(ray, hits);
        } else {
            return value.get().intersect(ray, hits);
        }
    }, variant);
}

bool object_t::inside(const vector_t& point) const {
    return std::visit([&](const auto& value) { return node(value).inside(point); }, variant);
}

}
