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

add_executable(SSRT main.cc "rtw_exr_image.h" "perlin.h" "quad.h" "image_io.h" "constant_medium.h" "fast_rng.h" "thread_pool.h" "xoshiro128.h" "morton2D.h" "simd_vec3.h" "OIDN.h" "packet.h" "aabb_packet.h"  "bvh_sah.h" "onb.h" "cameraMT.h"  "GGX.h" "mesh.h" "triangle.h" "transform.h"    "fbx_loader.h"  "abc_loader.h")

find_package(tinyexr CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE unofficial::tinyexr::tinyexr)

find_package(OpenColorIO CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE OpenColorIO::OpenColorIO)



# === OIDN ===
set(OIDN_ROOT "${PROJECT_SOURCE_DIR}/external/oidn")

include_directories("${PROJECT_SOURCE_DIR}/external/oidn/include")
link_directories("${PROJECT_SOURCE_DIR}/external/oidn/build/Release")
target_link_libraries(SSRT PRIVATE OpenImageDenoise)

# === FBXSDK ===
set(FBXSDK_DIR "C:/Program Files/Autodesk/FBX/FBX SDK/2020.3.7/")

include_directories("${FBXSDK_DIR}/include")

# Release / Debug lib folders
link_directories("${FBXSDK_DIR}/lib/x64/release")
link_directories("${FBXSDK_DIR}/lib/x64/debug")

# FBX Base lib
target_link_libraries(SSRT PRIVATE libfbxsdk-md.lib)

# === REQUIRED BY FBXSDK ===
# libxml2 + zlib !!!!!
target_link_libraries(SSRT PRIVATE
    libxml2-md.lib
    zlib-md.lib
)



# === Alembic ===
find_package(Alembic CONFIG REQUIRED)
target_link_libraries(SSRT PRIVATE Alembic::Alembic)

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

add_compile_definitions(USE_PACKET_TRACKING)