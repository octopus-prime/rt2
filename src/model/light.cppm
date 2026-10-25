module;

#include <vector>

export module model:light;

import :math;

export namespace rt2::model {

struct light_t {
    vector_t origin;
    vector_t intensity;
};

using lights_t = std::vector<light_t>;

}
