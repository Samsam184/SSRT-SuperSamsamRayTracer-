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

add_executable(SSRT main.cc "rtw_exr_image.h" "perlin.h" "quad.h" "image_io.h" "constant_medium.h"   "fast_rng.h" "thread_pool.h" "xoshiro128.h" "morton2D.h"  "simd_vec3.h"  "OIDN.h")

find_package(tinyexr CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE unofficial::tinyexr::tinyexr)

find_package(OpenColorIO CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE OpenColorIO::OpenColorIO)

# === OIDN ===
set(OIDN_ROOT "${PROJECT_SOURCE_DIR}/external/oidn")

include_directories("${PROJECT_SOURCE_DIR}/external/oidn/include")
link_directories("${PROJECT_SOURCE_DIR}/external/oidn/build/Release")
target_link_libraries(SSRT PRIVATE OpenImageDenoise)


# Copier automatiquement les DLL nécessaires à côté de l'exécutable
add_custom_command(TARGET SSRT POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OIDN_ROOT}/build/Release/OpenImageDenoise.dll"
        "${OIDN_ROOT}/build/Release/OpenImageDenoise_core.dll"
        "${OIDN_ROOT}/build/Release/OpenImageDenoise_device_cpu.dll"
        "${OIDN_ROOT}/build/Release/tbb12.dll"
        $<TARGET_FILE_DIR:SSRT>
)

target_include_directories(SSRT PRIVATE "./external/pcg")