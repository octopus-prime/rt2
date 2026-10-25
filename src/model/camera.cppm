export module model:camera;

import :math;

export namespace rt2::model {

struct camera_t {
    vector_t origin;
    vector_t direction;
    vector_t up;
    scalar_t fov;
};

} // namespace rt2::model
