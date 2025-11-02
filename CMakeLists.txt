cmake_minimum_required(VERSION 3.16)
project(SSRT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
	add_compile_options(/O2 /fp:fast /Ob2 /Oi /Ot /GL /GS- /arch:AVX2 /DNDEBUG)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
else()
	add_compile_options(-03 -march=native -ffast-math -funroll-loops)
endif()

add_executable(SSRT main.cc "rtw_exr_image.h" "perlin.h" "quad.h" "image_io.h" "constant_medium.h"   "fast_rng.h" "thread_pool.h" "xoshiro128.h" "morton2D.h"  "simd_vec3.h")

find_package(tinyexr CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE unofficial::tinyexr::tinyexr)

find_package(OpenColorIO CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE OpenColorIO::OpenColorIO)

#faudra set sa destination a oidn soi même pour l'instant ptdrrr'
include_directories("D:/SSRT/external/oidn-2.3.3.x64.windows/include")
link_directories("D:/SSRT/external/oidn-2.3.3.x64.windows/lib")
target_link_libraries(SSRT PRIVATE "D:/SSRT/external/oidn-2.3.3.x64.windows/lib/OpenImageDenoise.lib")

target_include_directories(SSRT PRIVATE "./external/pcg")