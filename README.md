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

Both demo scenes require Artec's [Fantasy Dragon](https://www.artec3d.com/3d-models/fantasy-dragon)
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
./build/rt2 --many --benchmark # Warm-up + five renders; median/checksum, no PNG
```

Rendering is 3840 x 2160 with four samples per pixel. PNGs are written to the
current working directory. Benchmark timing excludes scene construction.
Mesh and text instances share immutable geometry and their internal R-trees.

## Licenses

rt2's own code is copyright (c) 2026 [octopus-prime](https://github.com/octopus-prime)
and contributors, licensed
under the [Boost Software License 1.0](LICENSE).
Dependencies and assets retain their own licenses; see
[third-party notices and image attribution](THIRD_PARTY_NOTICES.md).
In particular, published renders containing the dragons need their attribution.
