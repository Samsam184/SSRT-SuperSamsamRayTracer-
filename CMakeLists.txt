cmake_minimum_required(VERSION 3.16)
project(SSRT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(SSRT main.cc "rtw_exr_image.h" "perlin.h" "quad.h")

find_package(tinyexr CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE unofficial::tinyexr::tinyexr)

