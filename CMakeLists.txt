cmake_minimum_required(VERSION 3.30)

project(rt2 LANGUAGES C CXX)

# set(CMAKE_CXX_COMPILER clang++)
# set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

find_package(PNG REQUIRED)
find_package(Threads REQUIRED)
find_package(Freetype REQUIRED)
find_file(RT2_FONT_FILE NAMES DejaVuSans.ttf
    PATHS /usr/share/fonts/truetype/dejavu
    DOC "DejaVu Sans outline font used by the demo and text geometry tests" REQUIRED)

add_library(rply STATIC third_party/rply/rply.c)
target_include_directories(rply PUBLIC third_party/rply)

file(GLOB_RECURSE module_sources CONFIGURE_DEPENDS src/*.cppm)
file(GLOB_RECURSE source_files CONFIGURE_DEPENDS src/*.cpp)

add_executable(rt2)

target_sources(rt2
    PRIVATE
        FILE_SET CXX_MODULES
        FILES ${module_sources}
    PRIVATE
        ${source_files}
)

target_compile_features(rt2 PRIVATE cxx_std_23)
target_compile_definitions(rt2 PRIVATE RT2_MESH_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/mesh")
target_compile_definitions(rt2 PRIVATE RT2_FONT_FILE="${RT2_FONT_FILE}")
target_compile_options(rt2 PRIVATE -march=native)
target_link_libraries(rt2 PRIVATE PNG::PNG Threads::Threads Freetype::Freetype rply)

include(CTest)
if(BUILD_TESTING)
    set(test_sources ${source_files})
    list(REMOVE_ITEM test_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(rt2_tests
        tests/main.cpp
        tests/model/sor.cpp
        tests/model/mesh.cpp
        tests/model/ply.cpp
        tests/model/text.cpp
        tests/model/object.cpp
        tests/model/torus.cpp
        tests/model/scene.cpp
        tests/renderer/renderer.cpp
    )
    target_sources(rt2_tests
        PRIVATE FILE_SET CXX_MODULES FILES ${module_sources}
        PRIVATE ${test_sources}
    )
    target_compile_features(rt2_tests PRIVATE cxx_std_23)
    target_compile_options(rt2_tests PRIVATE -march=native)
    target_compile_definitions(rt2_tests PRIVATE RT2_FONT_FILE="${RT2_FONT_FILE}")
    target_link_libraries(rt2_tests PRIVATE Threads::Threads Freetype::Freetype rply)
    foreach(suite IN ITEMS model/sor model/mesh model/ply model/text model/object model/torus model/scene renderer/renderer)
        add_test(NAME ${suite} COMMAND rt2_tests ${suite})
    endforeach()
endif()
