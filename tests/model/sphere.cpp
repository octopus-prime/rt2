#include "../support.hpp"

namespace rt2::tests {

void test_sphere() {
    const sphere_t sphere;
    check_hit_distances(sphere, ray({0, 0, -3}, {0, 0, 1}), {2, 4});
    check_hit_distances(sphere, ray({0, 0, 0}, {0, 0, 1}), {1});
    check_hit_distances(sphere, ray({1, 0, -3}, {0, 0, 1}), {3});
    check(sphere.inside({0, 0, 0}) && sphere.inside({0.9f, 0, 0}), "Sphere interior");
    check(!sphere.inside({1, 0, 0}) && !sphere.inside({2, 0, 0}), "Sphere exterior/boundary");
    check_inside_rejects_nonfinite(sphere);

    const auto front = nearest_hit(sphere, ray({0, 0, 3}, {0, 0, -1}));
    check(front.has_value(), "Sphere intersection");
    close(front->distance, 2, "Sphere entry distance");
    close(front->point.a[2], 1, "Sphere entry point");
    close(front->normal.a[2], 1, "Sphere outward normal");
    const auto exit = nearest_hit(sphere, ray({0, 0, 0}, {0, 0, 2}));
    check(exit.has_value(), "Sphere interior exit");
    close(exit->distance, 0.5f, "Sphere nonunit ray");
    check(!nearest_hit(sphere, ray({2, 0, 3}, {0, 0, -1})), "Sphere miss");
    check(!nearest_hit(sphere, ray({0, 0, 3}, {0, 0, -1}, 0, 1)), "Sphere finite maximum");
}

}
