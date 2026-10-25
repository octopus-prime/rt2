module;

#include <optional>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/qvm.hpp>

module model;

import :math;
import :ray;
import :hit;
import :surface;
import :shader;

namespace rt2::model {

std::optional<box3_t> object_t::transformed_bounds() const {
    const auto local = surface.bounds();
    if (!local) {
        return std::nullopt;
    }
    const vector_t minimum{
        boost::geometry::get<boost::geometry::min_corner, 0>(*local),
        boost::geometry::get<boost::geometry::min_corner, 1>(*local),
        boost::geometry::get<boost::geometry::min_corner, 2>(*local)};
    const vector_t maximum{
        boost::geometry::get<boost::geometry::max_corner, 0>(*local),
        boost::geometry::get<boost::geometry::max_corner, 1>(*local),
        boost::geometry::get<boost::geometry::max_corner, 2>(*local)};
    vector_t lower = boost::qvm::transform_point(scene_object, minimum);
    vector_t upper = lower;
    vector_t magnitude{0, 0, 0};
    for (unsigned corner = 0; corner < 8; ++corner) {
        const vector_t point{
            corner & 1 ? maximum.a[0] : minimum.a[0],
            corner & 2 ? maximum.a[1] : minimum.a[1],
            corner & 4 ? maximum.a[2] : minimum.a[2]};
        const auto transformed = boost::qvm::transform_point(scene_object, point);
        for (int axis = 0; axis < 3; ++axis) {
            lower.a[axis] = std::min(lower.a[axis], transformed.a[axis]);
            upper.a[axis] = std::max(upper.a[axis], transformed.a[axis]);
            scalar_t sum = std::abs(scene_object.a[axis][3]);
            for (int column = 0; column < 3; ++column) {
                sum += std::abs(scene_object.a[axis][column] * point.a[column]);
            }
            magnitude.a[axis] = std::max(magnitude.a[axis], sum);
        }
    }
    // Keep the broad phase conservative in the presence of transform rounding.
    for (int axis = 0; axis < 3; ++axis) {
        const scalar_t padding = 16 * std::numeric_limits<scalar_t>::epsilon()
            * std::max(scalar_t{1}, magnitude.a[axis]);
        lower.a[axis] -= padding;
        upper.a[axis] += padding;
    }
    return box3_t{point3_t{lower.a[0], lower.a[1], lower.a[2]},
        point3_t{upper.a[0], upper.a[1], upper.a[2]}};
}

void object_t::update_transform() {
    object_scene = boost::qvm::inverse(scene_object);
    scene_bounds = transformed_bounds();
}

object_t& object_t::translate(const vector_t& offset) {
    scene_object = boost::qvm::translation_mat(offset) * scene_object;
    update_transform();
    return *this;
}

object_t& object_t::scale(const vector_t& factors) {
    scene_object = boost::qvm::diag_mat(boost::qvm::XYZ1(factors)) * scene_object;
    update_transform();
    return *this;
}

std::optional<hit_t> object_t::intersect(const ray_t& ray) const {
    if (!std::isfinite(ray.min) || !std::isfinite(ray.max)) {
        throw std::invalid_argument("Object intersection requires finite ray limits");
    }
    if (ray.min > ray.max) {
        return std::nullopt;
    }
    if (scene_bounds) {
        const auto begin = ray(ray.min);
        const auto end = ray(ray.max);
        const boost::geometry::model::segment<point3_t> segment{
            point3_t{begin.a[0], begin.a[1], begin.a[2]},
            point3_t{end.a[0], end.a[1], end.a[2]}};
        if (!boost::geometry::intersects(segment, *scene_bounds)) {
            return std::nullopt;
        }
    }
    const ray_t transformed_ray{
        .origin = boost::qvm::transform_point(object_scene, ray.origin),
        .direction = boost::qvm::transform_vector(object_scene, ray.direction),
        .min = ray.min,
        .max = ray.max
    };
    auto nearest = surface.intersect(transformed_ray);
    if (!nearest) {
        return std::nullopt;
    }
    nearest->point = boost::qvm::transform_point(scene_object, nearest->point);
    nearest->normal = boost::qvm::normalized(boost::qvm::transform_vector(boost::qvm::transposed(object_scene), nearest->normal));
    nearest->object = this;
    return nearest;
}

vector_t object_t::eluminate(const hit_t& hit, const ray_t& incoming, const scene_t& scene, unsigned remaining_depth) const {
    return shader.eluminate(hit, incoming, scene, remaining_depth);
}

}
