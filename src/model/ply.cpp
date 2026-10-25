module;

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "rply.h"

module model;

import :math;
import :mesh;

namespace rt2::model {
namespace {

struct context_t {
    std::vector<vector_t> vertices;
    std::vector<mesh_t::face_t> faces;
    char error[512]{};
    const char* validation_error = nullptr;
};

void error_callback(p_ply ply, const char* message) {
    void* pointer = nullptr;
    ply_get_ply_user_data(ply, &pointer, nullptr);
    if (pointer) {
        auto& context = *static_cast<context_t*>(pointer);
        std::snprintf(context.error, sizeof(context.error), "%s", message);
    }
}

int vertex_callback(p_ply_argument argument) {
    void* pointer = nullptr;
    long axis = 0;
    long index = 0;
    ply_get_argument_user_data(argument, &pointer, &axis);
    ply_get_argument_element(argument, nullptr, &index);
    auto& context = *static_cast<context_t*>(pointer);
    // RPly's file-format API returns double; geometry storage uses scalar_t.
    const auto value = ply_get_argument_value(argument);
    if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<scalar_t>::max()) {
        context.validation_error = "Vertex coordinate is not finite or representable in scalar_t";
        return 0;
    }
    context.vertices[static_cast<std::size_t>(index)].a[axis] = static_cast<scalar_t>(value);
    return 1;
}

int face_callback(p_ply_argument argument) {
    void* pointer = nullptr;
    long index = 0;
    long length = 0;
    long item = 0;
    ply_get_argument_user_data(argument, &pointer, nullptr);
    ply_get_argument_element(argument, nullptr, &index);
    ply_get_argument_property(argument, nullptr, &length, &item);
    auto& context = *static_cast<context_t*>(pointer);
    if (length != 3) {
        context.validation_error = "PLY faces must contain exactly three vertex indices";
        return 0;
    }
    if (item == -1) {
        return 1;
    }
    const auto value = ply_get_argument_value(argument);
    if (!std::isfinite(value) || value < 0 || std::trunc(value) != value
        || value >= static_cast<double>(context.vertices.size())) {
        context.validation_error = "PLY face vertex index is invalid";
        return 0;
    }
    context.faces[static_cast<std::size_t>(index)][static_cast<std::size_t>(item)]
        = static_cast<std::size_t>(value);
    return 1;
}

void validate_properties(p_ply ply) {
    for (auto element = ply_get_next_element(ply, nullptr); element;
         element = ply_get_next_element(ply, element)) {
        const char* element_name = nullptr;
        ply_get_element_info(element, &element_name, nullptr);
        for (auto property = ply_get_next_property(element, nullptr); property;
             property = ply_get_next_property(element, property)) {
            const char* name = nullptr;
            e_ply_type type{};
            ply_get_property_info(property, &name, &type, nullptr, nullptr);
            const std::string_view element_view{element_name};
            const std::string_view property_view{name};
            if (element_view == "vertex"
                && (property_view == "x" || property_view == "y" || property_view == "z")
                && type == PLY_LIST) {
                throw std::runtime_error("Vertex coordinates must be scalar properties");
            }
            if (element_view == "face" && property_view == "vertex_indices" && type != PLY_LIST) {
                throw std::runtime_error("Face vertex_indices must be a list property");
            }
        }
    }
}

}

mesh_t read_ply(const std::filesystem::path& file) {
    const auto filename = file.string();
    context_t context;
    try {
        const std::unique_ptr<t_ply_, decltype(&ply_close)> ply{
            ply_open(filename.c_str(), error_callback, 0, &context), ply_close};
        if (!ply || !ply_read_header(ply.get())) {
            throw std::runtime_error(context.error[0] ? context.error : "Cannot open/read PLY header");
        }
        validate_properties(ply.get());
        const long vertices = ply_set_read_cb(ply.get(), "vertex", "x", vertex_callback, &context, 0);
        const long ys = ply_set_read_cb(ply.get(), "vertex", "y", vertex_callback, &context, 1);
        const long zs = ply_set_read_cb(ply.get(), "vertex", "z", vertex_callback, &context, 2);
        const long faces = ply_set_read_cb(ply.get(), "face", "vertex_indices", face_callback, &context, 0);
        if (vertices <= 0 || ys != vertices || zs != vertices || faces <= 0) {
            throw std::runtime_error("PLY requires nonempty vertex x/y/z and face vertex_indices");
        }
        context.vertices.resize(static_cast<std::size_t>(vertices));
        context.faces.resize(static_cast<std::size_t>(faces));
        if (!ply_read(ply.get())) {
            throw std::runtime_error(context.validation_error ? context.validation_error
                : context.error[0] ? context.error : "Cannot read PLY data");
        }
        return mesh_t{std::move(context.vertices), std::move(context.faces)};
    } catch (const std::exception& error) {
        throw std::runtime_error("PLY '" + filename + "': " + error.what());
    }
}

}
