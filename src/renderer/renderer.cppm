module;

#include <boost/gil.hpp>

export module renderer;

import model;

export namespace rt2::renderer {

// Fills an RGB8 view with sRGB pixels using all reported hardware threads.
void render(const model::scene_t& scene, boost::gil::rgb8_view_t pixels, unsigned depth = 8);

}
