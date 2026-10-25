#include "../support.hpp"

namespace rt2::tests {

void test_torus() {
    const torus_t torus{4, scalar_t{0.25f}};
    check_hit_distances(torus, ray({-4, 0, 0}, {1, 0, 0}), {1.5f, 2.5f, 5.5f, 6.5f});
    check(torus.inside({2, 0, 0}), "Torus tube interior");
    check(!torus.inside({0, 0, 0}) && !torus.inside({2.5f, 0, 0})
        && !torus.inside({2, 0, 0.5f}), "Torus hole and boundaries");
    check_inside_rejects_nonfinite(torus);
    for (int i = 0; i < 40; ++i) {
        const scalar_t distance = 20 + static_cast<scalar_t>(i) * 5;
        const auto hit = nearest_hit(torus, ray({2, 0, distance}, {0, 0, -1}));
        check(hit.has_value(), "Distant torus tube must be hit");
        close(hit->distance, distance - scalar_t{0.5f}, "Distant torus distance");
        close(hit->point.a[2], scalar_t{0.5f}, "Distant torus surface point");
        close(hit->normal.a[2], 1, "Distant torus normal");
        check(!nearest_hit(torus, ray({0, 0, distance}, {0, 0, -1})),
            "Distant torus hole must remain empty");
    }
}

} // namespace rt2::tests
