#include <array>
#include <span>
#include <vector>

#include "../support.hpp"

namespace rt2::tests {

void test_hit() {
    const sphere_t sphere;
    const auto incoming = ray({0, 0, -3}, {0, 0, 1});
    std::array<hit_t, 3> storage{};
    storage[2].distance = -123;
    const auto count = sphere.intersect(incoming, std::span<hit_t>{storage}.first(2));
    check(count == 2 && storage[2].distance == -123, "Only filled span is written");
    bool overflow = false;
    try {
        sphere.intersect(incoming, std::span<hit_t>{storage}.first(1));
    } catch (const hit_buffer_overflow&) {
        overflow = true;
    }
    check(overflow && storage[2].distance == -123, "Overflow is explicit and bounded");
    check(sphere.intersect(ray({3, 0, -3}, {0, 0, 1}), {}) == 0,
        "Empty span accepts a miss");
    hit_buffer_t buffer;
    buffer.storage.resize(1);
    check(buffer.intersect(sphere, incoming).size() == 2, "Workspace grows and retries");
    const auto* pointer = buffer.storage.data();
    for (int i = 0; i < 20; ++i) {
        check(buffer.intersect(sphere, incoming).size() == 2, "Repeated full results");
        check(pointer == buffer.storage.data(), "Workspace reuses its allocation");
    }
    const primitive_t primitive{.surface = surface_t{sphere},
        .shader = shader_t{phong_t{}}};
    const union_t united{primitive, primitive};
    buffer.storage.resize(1);
    check(buffer.intersect(united, incoming).size() == 2,
        "CSG workspace grows for both operands before filtering");
    std::array<hit_t, 2> too_small{};
    overflow = false;
    try {
        united.intersect(incoming, too_small);
    } catch (const hit_buffer_overflow&) {
        overflow = true;
    }
    check(overflow, "CSG reports insufficient intermediate storage");
    const scene_t scene{.objects = {primitive, primitive}};
    check(buffer.intersect(scene, incoming).size() == 4, "Scene retains every object's hits");
    close(scene.trace(incoming, 0, buffer).a[0], 0, "Trace may reuse hit buffer recursively");
}

}
