module;

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <numbers>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include <boost/qvm.hpp>
#include <boost/gil.hpp>

module renderer;

namespace rt2::renderer {

using pixel_t = boost::gil::rgb8_pixel_t;

void render(const model::scene_t& scene, boost::gil::rgb8_view_t pixels, unsigned depth) {
    using model::scalar_t;
    using model::vector_t;
    if (pixels.width() <= 0 || pixels.height() <= 0
        || pixels.width() > std::numeric_limits<int>::max()
        || pixels.height() > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Render dimensions must be positive and fit in int");
    }
    const int width = static_cast<int>(pixels.width());
    const int height = static_cast<int>(pixels.height());
    const auto& camera = scene.camera;
    const vector_t forward = boost::qvm::normalized(camera.direction);
    const vector_t right = boost::qvm::normalized(boost::qvm::cross(forward, camera.up));
    const vector_t up = boost::qvm::cross(right, forward);
    if (!std::isfinite(camera.fov) || camera.fov <= 0 || camera.fov >= 180) {
        throw std::invalid_argument("Camera vertical field of view must be between 0 and 180 degrees");
    }
    const scalar_t half_height = std::tan(camera.fov * std::numbers::pi_v<scalar_t> / 360);
    const scalar_t half_width = half_height * static_cast<scalar_t>(width) / height;
    const auto channel = [](scalar_t linear) {
        if (!std::isfinite(linear)) {
            throw std::runtime_error("Tracing produced a non-finite color");
        }
        linear = std::clamp(linear, scalar_t{0}, scalar_t{1});
        // Convert linear RGB to sRGB for display.
        const scalar_t encoded = linear <= 0.0031308f
            ? 12.92f * linear : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
        return static_cast<std::uint8_t>(std::lround(encoded * 255));
    };

    const auto render_row = [&](int y) {
        constexpr std::array<std::array<scalar_t, 2>, 4> samples{{
            {scalar_t{0.375f}, scalar_t{0.125f}},
            {scalar_t{0.875f}, scalar_t{0.375f}},
            {scalar_t{0.125f}, scalar_t{0.625f}},
            {scalar_t{0.625f}, scalar_t{0.875f}}
        }};
        for (int x = 0; x < width; ++x) {
            vector_t color{0, 0, 0};
            for (const auto& sample : samples) {
                const scalar_t screen_x = (2 * (x + sample[0]) / width - 1) * half_width;
                const scalar_t screen_y = (1 - 2 * (y + sample[1]) / height) * half_height;
                const vector_t direction = boost::qvm::normalized(
                    forward + screen_x * right + screen_y * up);
                const model::ray_t ray{
                    .origin = camera.origin,
                    .direction = direction,
                    .min = 0,
                    .max = model::ray_max
                };
                color += scene.trace(ray, depth);
            }
            color /= static_cast<scalar_t>(samples.size());
            pixels(x, y) = pixel_t(channel(color.a[0]), channel(color.a[1]), channel(color.a[2]));
        }
    };
    const unsigned thread_count = std::min(
        static_cast<unsigned>(height), std::max(1u, std::thread::hardware_concurrency()));
    std::atomic<int> next_row{0};
    std::atomic<bool> failed{false};
    std::mutex error_mutex;
    std::exception_ptr error;
    {
        std::vector<std::jthread> workers;
        workers.reserve(thread_count);
        for (unsigned i = 0; i < thread_count; ++i) {
            workers.emplace_back([&] {
                try {
                    while (!failed.load(std::memory_order_relaxed)) {
                        const int y = next_row.fetch_add(1, std::memory_order_relaxed);
                        if (y >= height) {
                            break;
                        }
                        render_row(y);
                    }
                } catch (...) {
                    std::lock_guard lock(error_mutex);
                    if (!error) {
                        error = std::current_exception();
                    }
                    failed.store(true, std::memory_order_relaxed);
                }
            });
        }
    }
    if (error) {
        std::rethrow_exception(error);
    }
}

} // namespace rt2::renderer
    