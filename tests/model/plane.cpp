#include "../support.hpp"

namespace rt2::tests {

void test_plane() {
    const plane_t plane;
    check(plane.inside({100, -100, -1}), "Plane negative half-space");
    check(!plane.inside({0, 0, 0}) && !plane.inside({0, 0, 1}), "Plane boundary/exterior");
    check_inside_rejects_nonfinite(plane);

    const auto front = plane.intersect(ray({2, 3, 4}, {0, 0, -2}));
    check(front.has_value(), "Plane intersection");
    close(front->distance, 2, "Plane nonunit ray distance");
    close(front->point.a[2], 0, "Plane hit position");
    close(front->normal.a[2], 1, "Plane normal");
    const auto back = plane.intersect(ray({0, 0, -1}, {0, 0, 1}));
    check(back.has_value(), "Plane back intersection");
    close(back->distance, 1, "Plane back distance");
    check(!plane.intersect(ray({0, 0, 1}, {1, 0, 0})), "Plane parallel miss");
    check(!plane.intersect(ray({0, 0, 4}, {0, 0, -1}, 0, 3)), "Plane finite maximum");
}

}
