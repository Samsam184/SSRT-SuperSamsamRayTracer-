cmake_minimum_required(VERSION 3.16)
project(SSRT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(SSRT main.cc "rtw_exr_image.h" "perlin.h" "quad.h" "image_io.h" "constant_medium.h"  "OIDN.h")

find_package(tinyexr CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE unofficial::tinyexr::tinyexr)

find_package(OpenColorIO CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE OpenColorIO::OpenColorIO)

#faudra set sa destination a oidn soi même pour l'instant ptdrrr'
include_directories("D:/SSRT/external/oidn-2.3.3.x64.windows/include")
link_directories("D:/SSRT/external/oidn-2.3.3.x64.windows/lib")
target_link_libraries(SSRT PRIVATE "D:/SSRT/external/oidn-2.3.3.x64.windows/lib/OpenImageDenoise.lib")
