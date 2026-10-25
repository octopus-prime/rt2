#include <array>
#include <variant>
#include <boost/gil.hpp>

#include "../support.hpp"

import renderer;

namespace rt2::tests {

void test_rotated_grid_aa() {
    constexpr std::array<std::array<scalar_t, 2>, 4> positions{{
        {scalar_t{-0.5f}, scalar_t{1.5f}},
        {scalar_t{1.5f}, scalar_t{0.5f}},
        {scalar_t{-1.5f}, scalar_t{-0.5f}},
        {scalar_t{0.5f}, scalar_t{-1.5f}}
    }};
    for (const auto& position : positions) {
        primitive_t sphere{
            .surface = surface_t{sphere_t{}},
            .shader = shader_t{phong_t{
                .ambient = {1, 1, 1}, .diffuse = {0, 0, 0}, .specular = {0, 0, 0}}}};
        sphere.scale({0.1f, 0.1f, 0.1f}).translate({position[0], position[1], -2});
        scene_t scene{
            .objects = {sphere},
            .camera = camera_t{
                .origin = {0, 0, 0}, .direction = {0, 0, -1},
                .up = {0, 1, 0}, .fov = 90}};
        boost::gil::rgb8_image_t image(1, 1);
        rt2::renderer::render(scene, boost::gil::view(image), 0);
        const auto pixel = boost::gil::const_view(image)(0, 0);
        for (int channel = 0; channel < 3; ++channel) {
            check(pixel[channel] == 137, "One covered AA sample must encode linear 0.25 as sRGB 137");
        }
        std::get<primitive_t>(scene.objects[0].variant).shader = shader_t{phong_t{
            .ambient = {4, 4, 4}, .diffuse = {0, 0, 0}, .specular = {0, 0, 0}}};
        rt2::renderer::render(scene, boost::gil::view(image), 0);
        check(boost::gil::const_view(image)(0, 0) == boost::gil::rgb8_pixel_t{255, 255, 255},
            "AA must average before clamping");
    }
}

} // namespace rt2::tests
