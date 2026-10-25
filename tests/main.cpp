#include <array>
#include <cstdio>
#include <exception>
#include <print>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace rt2::tests {

void test_sor();
void test_mesh();
void test_ply();
void test_object();
void test_torus_precision();
void test_scene_index();
void test_rotated_grid_aa();

} // namespace rt2::tests

int main(int argc, char** argv) {
    using namespace rt2::tests;
    const std::array<std::pair<std::string_view, void (*)()>, 7> suites{{
        {"model/sor", test_sor},
        {"model/mesh", test_mesh},
        {"model/ply", test_ply},
        {"model/object", test_object},
        {"model/torus", test_torus_precision},
        {"model/scene", test_scene_index},
        {"renderer/renderer", test_rotated_grid_aa}
    }};
    std::string_view current;
    try {
        if (argc > 2) {
            throw std::invalid_argument("Usage: rt2_tests [suite]");
        }
        bool matched = false;
        for (const auto& [name, run] : suites) {
            if (argc == 1 || name == argv[1]) {
                matched = true;
                current = name;
                run();
                std::println("{} tests passed", name);
            }
        }
        if (!matched) {
            throw std::invalid_argument("Unknown test suite");
        }
    } catch (const std::exception& error) {
        std::println(stderr, "Test failure [{}]: {}", current, error.what());
        return 1;
    }
}
