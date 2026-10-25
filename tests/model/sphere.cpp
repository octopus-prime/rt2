#include "../support.hpp"

namespace rt2::tests {

void test_sphere() {
    const sphere_t sphere;
    check(sphere.inside({0, 0, 0}) && sphere.inside({0.9f, 0, 0}), "Sphere interior");
    check(!sphere.inside({1, 0, 0}) && !sphere.inside({2, 0, 0}), "Sphere exterior/boundary");
    check_inside_rejects_nonfinite(sphere);

    const auto front = sphere.intersect(ray({0, 0, 3}, {0, 0, -1}));
    check(front.has_value(), "Sphere intersection");
    close(front->distance, 2, "Sphere entry distance");
    close(front->point.a[2], 1, "Sphere entry point");
    close(front->normal.a[2], 1, "Sphere outward normal");
    const auto exit = sphere.intersect(ray({0, 0, 0}, {0, 0, 2}));
    check(exit.has_value(), "Sphere interior exit");
    close(exit->distance, 0.5f, "Sphere nonunit ray");
    check(!sphere.intersect(ray({2, 0, 3}, {0, 0, -1})), "Sphere miss");
    check(!sphere.intersect(ray({0, 0, 3}, {0, 0, -1}, 0, 1)), "Sphere finite maximum");
}

}
