module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <boost/math/interpolators/pchip.hpp>
#include <boost/math/tools/polynomial.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/qvm.hpp>

module model;

namespace rt2::model {

using polynomial_t = boost::container::static_vector<scalar_t, 7>;
using roots_t = boost::container::static_vector<scalar_t, 13>;

bool sor_t::inside(const vector_t& point) const {
    validate_point(point);
    const scalar_t z = point.a[2];
    if (z <= segments.front().lower || z >= segments.back().upper) {
        return false;
    }
    const auto segment = std::ranges::find_if(segments,
        [z](const auto& value) { return z <= value.upper; });
    const scalar_t radius = boost::math::tools::evaluate_polynomial(
        segment->radius.data(), z - segment->lower, segment->radius.size());
    return std::hypot(point.a[0], point.a[1]) < radius;
}

constexpr scalar_t root_tolerance = 8 * std::numeric_limits<scalar_t>::epsilon();

// Derivative roots split [0, 1] into monotone intervals. Testing the split
// points also captures even-multiplicity roots that have no sign change.
roots_t isolate_roots(polynomial_t coefficients) {
    while (coefficients.size() > 1 && coefficients.back() == 0) {
        coefficients.pop_back();
    }
    scalar_t scale = 0;
    for (auto c : coefficients) {
        scale += std::abs(c);
    }
    if (scale == 0 || coefficients.size() == 1) {
        return {};
    }
    for (auto& c : coefficients) {
        c /= scale;
    }
    if (coefficients.size() == 2) {
        const scalar_t root = -coefficients[0] / coefficients[1];
        return root >= 0 && root <= 1 ? roots_t{root} : roots_t{};
    }
    polynomial_t derivative;
    for (std::size_t i = 1; i < coefficients.size(); ++i) {
        derivative.push_back(static_cast<scalar_t>(i) * coefficients[i]);
    }
    auto splits = isolate_roots(std::move(derivative));
    splits.insert(splits.begin(), 0);
    splits.push_back(1);
    roots_t roots;
    const auto f = [&](scalar_t x) {
        return boost::math::tools::evaluate_polynomial(coefficients.data(), x, coefficients.size());
    };
    for (auto x : splits) {
        if (std::abs(f(x)) <= root_tolerance) {
            roots.push_back(x);
        }
    }
    for (std::size_t i = 1; i < splits.size(); ++i) {
        const scalar_t left = splits[i - 1];
        const scalar_t right = splits[i];
        if (left == right || f(left) == 0 || f(right) == 0
            || std::signbit(f(left)) == std::signbit(f(right))) {
            continue;
        }
        boost::uintmax_t iterations = std::numeric_limits<scalar_t>::digits + 4;
        const auto bracket = boost::math::tools::bisect(f, left, right,
            [](scalar_t a, scalar_t b) {
                return b - a <= std::numeric_limits<scalar_t>::epsilon();
            }, iterations);
        // if (bracket.second - bracket.first > std::numeric_limits<scalar_t>::epsilon()) {
        //     throw std::runtime_error("SoR root refinement did not converge");
        // }
        roots.push_back(std::midpoint(bracket.first, bracket.second));
    }
    std::ranges::sort(roots);
    roots.erase(std::unique(roots.begin(), roots.end(),
        [](scalar_t a, scalar_t b) { return std::abs(a - b) <= root_tolerance; }), roots.end());
    return roots;
}

sor_t::sor_t(const std::vector<std::array<scalar_t, 2>>& profile) : max_radius(0) {
    if (profile.size() < 4) {
        throw std::invalid_argument("SoR requires at least four <height, radius> samples");
    }
    std::vector<scalar_t> heights;
    std::vector<scalar_t> radii;
    for (const auto& point : profile) {
        if (!std::isfinite(point[0]) || !std::isfinite(point[1]) || point[1] <= 0
            || (!heights.empty() && point[0] <= heights.back())) {
            throw std::invalid_argument("SoR requires increasing finite heights and positive finite radii");
        }
        heights.push_back(point[0]);
        radii.push_back(point[1]);
        max_radius = std::max(max_radius, point[1]);
    }
    auto spline_heights = heights;
    auto spline_radii = radii;
    const boost::math::interpolators::pchip<std::vector<scalar_t>> spline(
        std::move(spline_heights), std::move(spline_radii));
    for (std::size_t i = 1; i < heights.size(); ++i) {
        const scalar_t h = heights[i] - heights[i - 1];
        const scalar_t slope = (radii[i] - radii[i - 1]) / h;
        const scalar_t left = spline.prime(heights[i - 1]);
        const scalar_t right = spline.prime(heights[i]);
        segments.push_back({heights[i - 1], heights[i],
            {radii[i - 1], left, (3 * slope - 2 * left - right) / h,
                (left + right - 2 * slope) / (h * h)}});
    }
}

std::optional<box3_t> sor_t::bounds() const {
    // PCHIP preserves the sample range.
    return box3_t{point3_t{-max_radius, -max_radius, segments.front().lower},
        point3_t{max_radius, max_radius, segments.back().upper}};
}

std::size_t sor_t::intersect(const ray_t& ray, std::span<hit_t> hits) const {
    const auto& origin = ray.origin;
    const auto& direction = ray.direction;
    const scalar_t lower = segments.front().lower;
    const scalar_t upper = segments.back().upper;
    const scalar_t middle = std::midpoint(lower, upper);
    const vector_t relative = origin - vector_t{0, 0, middle};
    const scalar_t bound_squared = (max_radius * max_radius
        + (upper - middle) * (upper - middle)) * (1 + root_tolerance);
    const auto [near, far] = boost::math::tools::quadratic_roots(
        boost::qvm::mag_sqr(direction), 2 * boost::qvm::dot(relative, direction),
        boost::qvm::mag_sqr(relative) - bound_squared);
    if (!std::isfinite(near) || !std::isfinite(far)) {
        return 0;
    }
    const scalar_t first = std::max(ray.min, near);
    const scalar_t last = std::min(ray.max, far);
    if (first > last) {
        return 0;
    }
    std::size_t count = 0;
    const auto add = [&](scalar_t t, const vector_t& normal) {
        if (std::isfinite(t) && ray[t]) {
            append_hit(hits, count, hit_t{
                .point = ray(t),
                .normal = boost::qvm::normalized(normal),
                .distance = t
            });
        }
    };
    if (direction.a[2] != 0) {
        for (auto z : {lower, upper}) {
            const auto& segment = z == lower ? segments.front() : segments.back();
            const scalar_t h = z - segment.lower;
            const auto& c = segment.radius;
            const scalar_t radius = ((c[3] * h + c[2]) * h + c[1]) * h + c[0];
            const scalar_t t = (z - origin.a[2]) / direction.a[2];
            const vector_t p = ray(t);
            if (t >= first && t <= last
                && p.a[0] * p.a[0] + p.a[1] * p.a[1] <= radius * radius) {
                add(t, {0, 0, z == lower ? scalar_t{-1} : scalar_t{1}});
            }
        }
    }
    for (const auto& segment : segments) {
        scalar_t begin = first;
        scalar_t end = last;
        if (direction.a[2] == 0) {
            if (origin.a[2] < segment.lower || origin.a[2] > segment.upper) {
                continue;
            }
        } else {
            const scalar_t a = (segment.lower - origin.a[2]) / direction.a[2];
            const scalar_t b = (segment.upper - origin.a[2]) / direction.a[2];
            begin = std::max(begin, std::min(a, b));
            end = std::min(end, std::max(a, b));
        }
        if (begin > end) {
            continue;
        }
        // Solve in normalized u in [0, 1] to keep the polynomial well scaled.
        const vector_t p = ray(begin);
        const vector_t d = (end - begin) * direction;
        const scalar_t z = p.a[2] - segment.lower;
        const scalar_t dz = d.a[2];
        const auto& c = segment.radius;
        const std::array<scalar_t, 4> r{
            ((c[3] * z + c[2]) * z + c[1]) * z + c[0],
            (c[1] + 2 * c[2] * z + 3 * c[3] * z * z) * dz,
            (c[2] + 3 * c[3] * z) * dz * dz,
            c[3] * dz * dz * dz
        };
        if (begin == end) {
            const scalar_t radial_squared = p.a[0] * p.a[0] + p.a[1] * p.a[1];
            if (std::abs(radial_squared - r[0] * r[0])
                <= root_tolerance * std::max(radial_squared, r[0] * r[0])) {
                const scalar_t derivative = (3 * c[3] * z + 2 * c[2]) * z + c[1];
                add(begin, {p.a[0], p.a[1], -r[0] * derivative});
            }
            continue;
        }
        polynomial_t polynomial(7, 0);
        polynomial[0] = p.a[0] * p.a[0] + p.a[1] * p.a[1];
        polynomial[1] = 2 * (p.a[0] * d.a[0] + p.a[1] * d.a[1]);
        polynomial[2] = d.a[0] * d.a[0] + d.a[1] * d.a[1];
        for (std::size_t i = 0; i < r.size(); ++i) {
            for (std::size_t j = 0; j < r.size(); ++j) {
                polynomial[i + j] -= r[i] * r[j];
            }
        }
        for (auto u : isolate_roots(std::move(polynomial))) {
            const scalar_t t = begin + u * (end - begin);
            const vector_t point = ray(t);
            const scalar_t h = point.a[2] - segment.lower;
            const scalar_t radius = ((c[3] * h + c[2]) * h + c[1]) * h + c[0];
            const scalar_t derivative = (3 * c[3] * h + 2 * c[2]) * h + c[1];
            add(t, {point.a[0], point.a[1], -radius * derivative});
        }
    }
    return finish_hits(hits, count);
}

} // namespace rt2::model
