#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include "../support.hpp"

namespace rt2::tests {
namespace {

struct fixture_t {
    std::filesystem::path directory;

    fixture_t() {
        const auto id = std::chrono::steady_clock::now().time_since_epoch().count();
        directory = std::filesystem::temp_directory_path() / ("rt2-ply-" + std::to_string(id));
        if (!std::filesystem::create_directory(directory)) {
            throw std::runtime_error("Cannot create PLY fixture directory");
        }
    }

    ~fixture_t() {
        std::error_code error;
        std::filesystem::remove(directory / "mesh.ply", error);
        std::filesystem::remove(directory, error);
    }

    std::filesystem::path write(const std::string& contents) const {
        const auto file = directory / "mesh.ply";
        std::ofstream stream(file, std::ios::binary);
        stream.exceptions(std::ios::badbit | std::ios::failbit);
        stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        stream.close();
        return file;
    }
};

std::string header(std::string_view format) {
    return "ply\nformat " + std::string(format) + " 1.0\ncomment reader test\n"
        "element vertex 3\nproperty float x\nproperty float y\nproperty float z\n"
        "element face 1\nproperty list uchar int vertex_indices\nend_header\n";
}

void verify(const mesh_t& mesh) {
    const auto hit = nearest_hit(mesh, ray({0.25f, 0.25f, 2}, {0, 0, -1}));
    check(hit.has_value(), "Loaded PLY triangle intersection");
    close(hit->distance, 2, "PLY distance");
    close(hit->normal.a[2], 1, "PLY winding");
}

}

void test_ply() {
    fixture_t fixture;
    verify(read_ply(fixture.write(header("ascii") + "0 0 0\n1 0 0\n0 1 0\n3 0 1 2\n")));
    for (const bool little : {true, false}) {
        std::string contents = header(little ? "binary_little_endian" : "binary_big_endian");
        const auto word = [&](std::uint32_t bits) {
            for (int i = 0; i < 4; ++i) {
                const int shift = (little ? i : 3 - i) * 8;
                contents.push_back(static_cast<char>((bits >> shift) & 255));
            }
        };
        for (float value : std::array<float, 9>{0, 0, 0, 1, 0, 0, 0, 1, 0}) {
            word(std::bit_cast<std::uint32_t>(value));
        }
        contents.push_back(3);
        word(0);
        word(1);
        word(2);
        verify(read_ply(fixture.write(contents)));
        contents.pop_back();
        bool rejected = false;
        try {
            read_ply(fixture.write(contents));
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        check(rejected, "Truncated binary PLY must fail");
    }
    verify(read_ply(fixture.write(
        "ply\nformat ascii 1.0\nelement face 1\n"
        "property list uchar int vertex_indices\nelement vertex 3\n"
        "property float x\nproperty float y\nproperty float z\nend_header\n"
        "3 0 1 2\n0 0 0\n1 0 0\n0 1 0\n")));
    verify(read_ply(fixture.write(
        "ply\nformat ascii 1.0\nelement vertex 3\n"
        "property uchar red\nproperty float z\nproperty float x\nproperty float y\n"
        "element face 1\nproperty list uchar int vertex_indices\n"
        "element extra 1\nproperty float ignored\nend_header\n"
        "255 0 0 0\n255 0 1 0\n255 0 0 1\n3 0 1 2\n42\n")));
    const auto rejects = [&](const std::string& contents) {
        const auto path = fixture.write(contents);
        try {
            read_ply(path);
        } catch (const std::runtime_error& error) {
            check(std::string_view(error.what()).find(path.string()) != std::string_view::npos,
                "PLY error must include filename");
            return;
        }
        throw std::runtime_error("Malformed PLY accepted");
    };
    rejects("not ply\n");
    rejects(header("ascii") + "0 0 0\n1 0 0\n");
    rejects(header("ascii") + "0 0 0\n1 0 0\n0 1 0\n4 0 1 2 0\n");
    rejects(header("ascii") + "0 0 0\n1 0 0\n0 1 0\n2 0 1\n");
    rejects(header("ascii") + "0 0 0\n1 0 0\n0 1 0\n3 0 1 3\n");
    rejects(header("ascii") + "0 0 0\n1 0 0\n0 1 0\n3 0 -1 2\n");
    rejects(header("ascii") + "nan 0 0\n1 0 0\n0 1 0\n3 0 1 2\n");
    rejects(header("ascii") + "0 0 0\n1 0 0\n2 0 0\n3 0 1 2\n");
    rejects("ply\nformat ascii 1.0\nelement vertex 1\nproperty float x\nend_header\n0\n");
    auto scalar_face = header("ascii");
    scalar_face.replace(scalar_face.find("property list uchar int vertex_indices"),
        std::string("property list uchar int vertex_indices").size(), "property int vertex_indices");
    rejects(scalar_face + "0 0 0\n1 0 0\n0 1 0\n0\n");
    auto list_vertex = header("ascii");
    list_vertex.replace(list_vertex.find("property float x"),
        std::string("property float x").size(), "property list uchar float x");
    rejects(list_vertex);
    bool missing = false;
    try {
        read_ply(fixture.directory / "missing.ply");
    } catch (const std::runtime_error&) {
        missing = true;
    }
    check(missing, "Missing PLY must fail");
}

}
