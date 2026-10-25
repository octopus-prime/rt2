# rt2

A C++23 CPU ray tracer using C++ modules and Boost. Features include affine
object instances, scene and geometry R-trees, spheres, planes, tori, surfaces
of revolution, PLY meshes, extruded FreeType text, reflective/refractive
materials, and four-sample rotated-grid antialiasing.

## Build

Requires CMake 3.30+, Ninja, a C++23 compiler and standard library supporting
modules and `std::print`, Boost headers, libpng, FreeType, and DejaVu Sans.
Tested with Clang 22, CMake 4.2 and Boost 1.90 on Linux.

On Debian/Ubuntu, the library/tool dependencies are:

```sh
sudo apt-get install cmake ninja-build libboost-dev libpng-dev libfreetype-dev fonts-dejavu-core
```

Use a sufficiently recent Clang/standard library, then:

```sh
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

If needed, set `-DRT2_FONT_FILE=/path/to/DejaVuSans.ttf` when configuring.
The tests expect DejaVu Sans, not an arbitrary replacement font.
The build enables native CPU optimization; binaries may not run on older CPUs.

## Demo assets

The default and many-object scenes require Artec's [Fantasy Dragon](https://www.artec3d.com/3d-models/fantasy-dragon)
and Dragon models, provided separately. Extract the PLY files and their
accompanying licenses to:

```text
mesh/fantasy-dragon-ply/Fantasy Dragon.ply
mesh/fantasy-dragon-ply/license.txt
mesh/dragon-ply/Dragon-ply.ply
mesh/dragon-ply/license.txt
```

The mesh directory is ignored by Git; assets are not downloaded automatically.
Its absolute source-directory path is configured at build time.

## Run

```sh
./build/rt2                    # Default scene -> output.png
./build/rt2 --many             # 100 x 100 objects + floor -> output-many.png
./build/rt2 --csg              # Mesh/SOR/text/torus CSG gallery -> output-csg.png
./build/rt2 --many --benchmark # Warm-up + five renders; median/checksum, no PNG
```

Rendering is 3840 x 2160 with four samples per pixel. PNGs are written to the
current working directory. Benchmark timing excludes scene construction.
Mesh and text instances share immutable geometry and their internal R-trees.
`--many` and `--csg` are mutually exclusive; either supports `--benchmark`.

The CSG gallery has six labeled exhibits: mesh minus torus, SOR minus text,
text intersected with torus, mesh intersected with SOR, SOR united with torus,
and text united with torus. Its closed, twisted six-lobed triangle mesh is
generated locally; no dragon downloads are required for this scene.

## Point containment

All surfaces expose `bool inside(const vector_t&) const` in local coordinates;
`primitive_t::inside` accepts scene coordinates and applies the cached inverse
transform. A primitive owns its surface, shader, transforms and cached bounds.
Scenes store `object_t` wrappers containing a `std::variant` of primitives,
intersections, unions and differences; they forward bounds, transformations,
intersections and containment to the selected node. CSG alternatives use
`boost::recursive_wrapper` to break the recursive value-type dependency;
copying an object deep-copies its CSG tree while mesh/text geometry stays shared.
Hits reference the concrete primitive for shading.
Boundary points are excluded. The plane's interior is `z < 0`;
SOR and text interiors lie strictly between their caps. Non-finite query
points throw `std::invalid_argument`.

Mesh containment uses even-odd fill and the triangle R-tree. Its first query
lazily checks that every indexed edge has exactly two oppositely wound faces;
the result is shared and thread-safe. Open, nonmanifold-edge or inconsistently
wound meshes throw `std::logic_error` from `inside()` but still support rendering.
Solid queries require non-self-intersecting geometry; self-intersections are
not validated. Weld duplicate seam vertices before solid queries.
Mesh/text boundary detection uses floating-point tolerances, so points
extremely close to a boundary may also be excluded.

## Intersections and prepared CSG types

All surfaces, primitives, object wrappers and scenes use the all-hit API:

```cpp
std::size_t count = object.intersect(ray, std::span<hit_t>{storage});
// Only storage[0..count) contains results, sorted by increasing ray parameter.
```

There is no nearest-hit intersection overload. Tangent contacts appear once;
duplicate equal-distance roots/triangle contacts within a surface are collapsed.
Scenes retain coincident hits belonging to distinct objects. An undersized span
throws `hit_buffer_overflow`, never silently truncates; its contents are
unspecified after an exception. Capacity must accommodate intermediate hits
as well as the final result (both operands together for CSG).

Each renderer worker owns one `hit_buffer_t`, reused for primary, shadow,
reflection and refraction queries. It grows and retries on overflow, retaining
the allocation for later rays. Scene tracing copies the selected hit before
shading reuses the buffer. `scene.trace(ray, depth, buffer)` and
`scene.occluded(ray, buffer)` require this caller-owned workspace.

`difference_t`, `intersection_t` and `union_t` each own `object1` and `object2`
and implement containment, all-hit intersections and chainable transforms
applied to both operands. Difference reverses the second operand's cut normals;
hits retain their leaf primitive for material lookup. Crossings are classified
using operand containment between sorted events; exactly coincident events
are handled together. Operands must describe valid solids for containment.
As with the individual surfaces, floating-point boundary tolerances limit
classification of extremely thin features.

CSG nodes can be nested in `object_t` and used directly in scenes. Union bounds
enclose both operands (unbounded if either operand is unbounded); intersection
bounds use their overlap, or the bounded operand when only one is bounded.
Disjoint intersection boxes conservatively use the first operand box because
the bounds API has no empty-solid representation. Difference uses the first
operand's bounds. Scene index rebuilding refreshes all leaf transform caches.
Both demos show an orange sphere with a blue spherical cut (difference),
a coral lens (intersection) and a green fused pair (union). The default
scene places one of each in the foreground; the many-demo repeats 1,000
of each with varying rotations, still totaling 10,000 objects plus the floor.

## Licenses

rt2's own code is copyright (c) 2026 [octopus-prime](https://github.com/octopus-prime)
and contributors, licensed
under the [Boost Software License 1.0](LICENSE).
Dependencies and assets retain their own licenses; see
[third-party notices and image attribution](THIRD_PARTY_NOTICES.md).
In particular, published renders containing the dragons need their attribution.
