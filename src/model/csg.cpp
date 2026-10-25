module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <span>
#include <optional>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/qvm.hpp>

module model;

import :object;

namespace rt2::model {
namespace {

enum class operation_t { difference, intersection, union_ };

bool on_boundary(const object_t& object, const vector_t& point) {
    std::array<hit_t, 1> hit;
    for (int axis = 0; axis < 3; ++axis) {
        vector_t direction{0, 0, 0};
        direction.a[axis] = 1;
        try {
            if (object.intersect(ray_t{.origin = point, .direction = direction,
                    .min = 0, .max = 0}, hit) != 0) {
                return true;
            }
        } catch (const hit_buffer_overflow&) {
            // Even one boundary hit suffices; multiple incident faces also qualify.
            return true;
        }
    }
    return false;
}

bool combine(operation_t operation, bool first, bool second) {
    switch (operation) {
    case operation_t::difference: return first && !second;
    case operation_t::intersection: return first && second;
    case operation_t::union_: return first || second;
    }
    return false;
}

template<class Csg>
std::size_t intersect_csg(const Csg& csg, operation_t operation,
                         const ray_t& ray, std::span<hit_t> hits) {
    const auto first_count = csg.object1.intersect(ray, hits);
    const auto second_count = csg.object2.intersect(ray, hits.subspan(first_count));
    const auto count = first_count + second_count;
    if (count == 0) {
        return 0;
    }
    for (auto& hit : hits.first(first_count)) {
        hit.source_index = 0;
    }
    for (auto& hit : hits.subspan(first_count, second_count)) {
        hit.source_index = 1;
    }
    std::sort(hits.begin(), hits.begin() + count,
        [](const auto& a, const auto& b) {
            return a.distance < b.distance || (a.distance == b.distance
                && a.source_index < b.source_index);
        });
    bool first_inside = csg.object1.inside(ray(ray.min));
    bool second_inside = csg.object2.inside(ray(ray.min));
    std::size_t output = 0;
    scalar_t previous = ray.min;
    for (std::size_t i = 0; i < count;) {
        std::size_t end = i + 1;
        while (end < count && hits[end].distance == hits[i].distance) {
            ++end;
        }
        bool before1 = first_inside;
        bool before2 = second_inside;
        bool after1 = first_inside;
        bool after2 = second_inside;
        // Apply coincident events together. In particular A-A has no boundary,
        // and shared internal faces of a union disappear.
        for (std::size_t j = i; j < end; ++j) {
            const scalar_t direction = boost::qvm::dot(ray.direction, hits[j].normal);
            if (direction == 0) {
                continue;
            }
            const bool first = hits[j].source_index == 0;
            (first ? before1 : before2) = direction > 0;
            (first ? after1 : after2) = direction < 0;
        }
        const scalar_t distance = hits[i].distance;
        if (previous < distance) {
            const auto sample = ray(std::midpoint(previous, distance));
            before1 = csg.object1.inside(sample);
            before2 = csg.object2.inside(sample);
        }
        const scalar_t next = end < count ? hits[end].distance : ray.max;
        if (distance < next) {
            const auto sample = ray(std::midpoint(distance, next));
            after1 = csg.object1.inside(sample);
            after2 = csg.object2.inside(sample);
        }
        const bool before = combine(operation, before1, before2);
        const bool after = combine(operation, after1, after2);
        if (before != after) {
            for (std::size_t j = i; j < end; ++j) {
                auto hit = hits[j];
                if (operation == operation_t::difference && hit.source_index == 1) {
                    hit.normal = -hit.normal;
                }
                const bool changed = hit.source_index == 0 ? before1 != after1 : before2 != after2;
                if (changed) {
                    hits[output++] = hit;
                    break;
                }
            }
        } else {
            // Preserve isolated tangent contacts when the other operand admits
            // this boundary; tangent events do not change occupancy.
            for (std::size_t j = i; j < end; ++j) {
                auto hit = hits[j];
                if (boost::qvm::dot(ray.direction, hit.normal) != 0) {
                    continue;
                }
                const bool first = hit.source_index == 0;
                const bool other = first ? second_inside : first_inside;
                bool coincident = false;
                for (std::size_t k = i; k < end; ++k) {
                    coincident = coincident || (hits[k].source_index != hit.source_index
                        && boost::qvm::dot(hit.normal, hits[k].normal) > 0);
                }
                const bool exposed = coincident ? operation != operation_t::difference
                    : operation == operation_t::union_ ? !other
                    : operation == operation_t::intersection ? other : first ? !other : other;
                if (exposed) {
                    if (operation == operation_t::difference && !first) {
                        hit.normal = -hit.normal;
                    }
                    hits[output++] = hit;
                    break;
                }
            }
        }
        first_inside = after1;
        second_inside = after2;
        previous = distance;
        i = end;
    }
    return output;
}

}

std::optional<box3_t> difference_t::bounds() const {
    return object1.bounds();
}

std::optional<box3_t> intersection_t::bounds() const {
    const auto first = object1.bounds();
    const auto second = object2.bounds();
    if (!first) {
        return second;
    }
    if (!second) {
        return first;
    }
    const auto lower = [](const box3_t& box) {
        return vector_t{
            boost::geometry::get<boost::geometry::min_corner, 0>(box),
            boost::geometry::get<boost::geometry::min_corner, 1>(box),
            boost::geometry::get<boost::geometry::min_corner, 2>(box)};
    };
    const auto upper = [](const box3_t& box) {
        return vector_t{
            boost::geometry::get<boost::geometry::max_corner, 0>(box),
            boost::geometry::get<boost::geometry::max_corner, 1>(box),
            boost::geometry::get<boost::geometry::max_corner, 2>(box)};
    };
    auto min = lower(*first);
    auto max = upper(*first);
    const auto other_min = lower(*second);
    const auto other_max = upper(*second);
    for (int axis = 0; axis < 3; ++axis) {
        min.a[axis] = std::max(min.a[axis], other_min.a[axis]);
        max.a[axis] = std::min(max.a[axis], other_max.a[axis]);
        if (min.a[axis] > max.a[axis]) {
            // No empty-box state in the bounds API: keep a conservative operand box.
            return first;
        }
    }
    return box3_t{{min.a[0], min.a[1], min.a[2]}, {max.a[0], max.a[1], max.a[2]}};
}

std::optional<box3_t> union_t::bounds() const {
    auto first = object1.bounds();
    const auto second = object2.bounds();
    if (!first || !second) {
        return std::nullopt;
    }
    boost::geometry::expand(*first, *second);
    return first;
}

void difference_t::update_transform() {
    object1.update_transform();
    object2.update_transform();
}

void intersection_t::update_transform() {
    object1.update_transform();
    object2.update_transform();
}

void union_t::update_transform() {
    object1.update_transform();
    object2.update_transform();
}

bool difference_t::inside(const vector_t& point) const {
    validate_point(point);
    return object1.inside(point) && !object2.inside(point) && !on_boundary(object2, point);
}

bool intersection_t::inside(const vector_t& point) const {
    validate_point(point);
    return object1.inside(point) && object2.inside(point);
}

bool union_t::inside(const vector_t& point) const {
    validate_point(point);
    return object1.inside(point) || object2.inside(point);
}

std::size_t difference_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    return intersect_csg(*this, operation_t::difference, ray, hits);
}

std::size_t intersection_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    return intersect_csg(*this, operation_t::intersection, ray, hits);
}

std::size_t union_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    return intersect_csg(*this, operation_t::union_, ray, hits);
}


difference_t& difference_t::translate(const vector_t& offset) {
    object1.translate(offset);
    object2.translate(offset);
    return *this;
}

difference_t& difference_t::scale(const vector_t& factors) {
    object1.scale(factors);
    object2.scale(factors);
    return *this;
}

difference_t& difference_t::rotate_x(scalar_t angle) {
    object1.rotate_x(angle);
    object2.rotate_x(angle);
    return *this;
}

difference_t& difference_t::rotate_y(scalar_t angle) {
    object1.rotate_y(angle);
    object2.rotate_y(angle);
    return *this;
}

difference_t& difference_t::rotate_z(scalar_t angle) {
    object1.rotate_z(angle);
    object2.rotate_z(angle);
    return *this;
}

intersection_t& intersection_t::translate(const vector_t& offset) {
    object1.translate(offset);
    object2.translate(offset);
    return *this;
}

intersection_t& intersection_t::scale(const vector_t& factors) {
    object1.scale(factors);
    object2.scale(factors);
    return *this;
}

intersection_t& intersection_t::rotate_x(scalar_t angle) {
    object1.rotate_x(angle);
    object2.rotate_x(angle);
    return *this;
}

intersection_t& intersection_t::rotate_y(scalar_t angle) {
    object1.rotate_y(angle);
    object2.rotate_y(angle);
    return *this;
}

intersection_t& intersection_t::rotate_z(scalar_t angle) {
    object1.rotate_z(angle);
    object2.rotate_z(angle);
    return *this;
}

union_t& union_t::translate(const vector_t& offset) {
    object1.translate(offset);
    object2.translate(offset);
    return *this;
}

union_t& union_t::scale(const vector_t& factors) {
    object1.scale(factors);
    object2.scale(factors);
    return *this;
}

union_t& union_t::rotate_x(scalar_t angle) {
    object1.rotate_x(angle);
    object2.rotate_x(angle);
    return *this;
}

union_t& union_t::rotate_y(scalar_t angle) {
    object1.rotate_y(angle);
    object2.rotate_y(angle);
    return *this;
}

union_t& union_t::rotate_z(scalar_t angle) {
    object1.rotate_z(angle);
    object2.rotate_z(angle);
    return *this;
}

}
