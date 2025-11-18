#ifndef CAMERAMT_H
#define CAMERAMT_H 

#include "hittable.h"
#include <ostream>
#include <fstream>
#include <algorithm>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "material.h"
#include <thread>
#include <atomic>
#include <random>
#include <mutex>
#include "image_io.h"
#include <chrono>
#include "xoshiro128.h"
#include "fast_rng.h"
#include "thread_pool.h"
#include "morton2D.h"
#include "OIDN.h"
#include "fbx_loader.h"

#ifdef USE_PACKET_TRACKING
#include "packet.h"
#endif

class camera {
public:
    bool use_denoiser = true;
    double aspect_ratio = 16.0 / 9.0;
    int image_width = 100;
    int samples_per_pixel = 10;
    int max_depth = 550;
    color background;
    double vfov = 90;
    vec3 lookfrom = vec3(0, 0, 0);
    vec3 lookat = vec3(0, 0, 0);
    vec3 vup = vec3(0, 1, 0);
    double defocus_angle = 0;
    double focus_dist = 10;
    double near_plane = .1;
    double far_plane = 555;

    void hideCursor() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hOut, &cursorInfo);
    }
    void showCursor() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = TRUE;
        SetConsoleCursorInfo(hOut, &cursorInfo);
    }

    void render(const hittable& world) {
#ifdef _WIN32 
        DWORD_PTR mask = 0xFF;
        SetThreadAffinityMask(GetCurrentThread(), 0xFFFF);
#endif

        int nthreads = std::thread::hardware_concurrency();
        if (nthreads == 0) nthreads = 4;
        ThreadPool pool(nthreads);
        int tileSize = 128;

        initialize();

        // --- Buffers AOV ---
        std::vector<color> beautybuffer(image_width * image_height);
        std::vector<color> albedobuffer(image_width * image_height);
        std::vector<color> normalbuffer(image_width * image_height);
        std::vector<color> positionbuffer(image_width * image_height);
        std::vector<color> depthbuffer(image_width * image_height);
        std::vector<color> roughnessbuffer(image_width * image_height);
        std::vector<color> metallicbuffer(image_width * image_height);
        std::vector<color> emissionbuffer(image_width * image_height);
        std::vector<color> aobuffer(image_width * image_height);
        std::vector<color> idbuffer(image_width * image_height);
        std::vector<color> uvbuffer(image_width * image_height);

        std::vector<std::pair<uint32_t, std::pair<int, int>>> tiles;
        for (int y = 0; y < image_height; y += tileSize)
            for (int x = 0; x < image_width; x += tileSize)
                tiles.push_back({ morton2D(x / tileSize, y / tileSize), {x, y} });

        std::sort(tiles.begin(), tiles.end(),
            [](auto& a, auto& b) { return a.first < b.first; });

#pragma omp parallel for schedule(dynamic)
        for (int tile_idx = 0; tile_idx < (int)tiles.size(); ++tile_idx) {
            const auto& [code, xy] = tiles[tile_idx];
            int x = xy.first;
            int y = xy.second;

            uint64_t rng_state = 1337u + y * image_width + x;

            for (int j = y; j < std::min(y + tileSize, image_height); ++j) {
                for (int i = x; i < std::min(x + tileSize, image_width); ++i) {
                    const int idx = j * image_width + i;

                    color pixel_beauty(0, 0, 0);
                    color pixel_albedo(0, 0, 0);
                    color pixel_normal(0, 0, 0);
                    color pixel_position(0, 0, 0);
                    float pixel_depth = 0.0f;
                    color pixel_roughness(0, 0, 0);
                    color pixel_metallic(0, 0, 0);
                    color pixel_emission(0, 0, 0);
                    color pixel_ao(0, 0, 0);
                    color pixel_id(0, 0, 0);
                    color pixel_uv(0, 0, 0);

                    const vec3 pixel_base = pixel00_location + i * pixel_delta_u + j * pixel_delta_v;

                    for (int sample = 0; sample < samples_per_pixel; ++sample) {
                        float rx = randf(rng_state);
                        float ry = randf(rng_state);

                        const vec3 pixel_sample =
                            pixel_base + (rx - 0.5f) * pixel_delta_u + (ry - 0.5f) * pixel_delta_v;

                        const vec3 ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
                        const vec3 ray_direction = pixel_sample - ray_origin;

                        ray r(ray_origin, ray_direction);

                        color alb_out, nrm_out, pos_out, emit_out;
                        float depth_out = 0.0f, rough_out = 0.0f, metal_out = 0.0f, ao_out = 0.0f;
                        color id_out, uv_out;

                        color sample_color = ray_color(
                            r, max_depth, world,
                            &alb_out, &nrm_out, &depth_out, &pos_out,
                            &rough_out, &metal_out, &emit_out,
                            &ao_out, &id_out, &uv_out
                        );

                        pixel_beauty += sample_color;
                        pixel_albedo += alb_out;
                        pixel_normal += nrm_out;
                        pixel_position += pos_out;
                        pixel_depth += depth_out;
                        pixel_roughness += color(rough_out, rough_out, rough_out);
                        pixel_metallic += color(metal_out, metal_out, metal_out);
                        pixel_emission += emit_out;
                        pixel_ao += color(ao_out, ao_out, ao_out);
                        pixel_id += id_out;
                        pixel_uv += uv_out;
                    }

                    const float inv_spp = 1.0f / float(samples_per_pixel);
                    pixel_beauty *= inv_spp;
                    pixel_albedo *= inv_spp;
                    pixel_normal *= inv_spp;
                    pixel_position *= inv_spp;
                    pixel_depth *= inv_spp;
                    pixel_roughness *= inv_spp;
                    pixel_metallic *= inv_spp;
                    pixel_emission *= inv_spp;
                    pixel_ao *= inv_spp;
                    pixel_id *= inv_spp;
                    pixel_uv *= inv_spp;

                    beautybuffer[idx] = pixel_beauty;
                    albedobuffer[idx] = pixel_albedo;
                    normalbuffer[idx] = pixel_normal;
                    positionbuffer[idx] = pixel_position;

                    float depth_norm = (pixel_depth - near_plane) / (far_plane - near_plane);
                    if (!std::isfinite(depth_norm)) depth_norm = 0.0f;
                    depthbuffer[idx] = color(depth_norm, depth_norm, depth_norm);

                    roughnessbuffer[idx] = pixel_roughness;
                    metallicbuffer[idx] = pixel_metallic;
                    emissionbuffer[idx] = pixel_emission;
                    aobuffer[idx] = pixel_ao;
                    idbuffer[idx] = pixel_id;
                    uvbuffer[idx] = pixel_uv;
                }
            }

#pragma omp critical
            {
                static int done = 0;
                double progress = 100.0 * (++done) / ((image_height / tileSize) * (image_width / tileSize));
                if (done % 3 == 0) std::clog << "\rProgress: " << (int)progress << "% ";
            }
        }

        pool.wait();

        if (use_denoiser) {
            std::cout << "\n\n[OIDN] Denoising beauty (RGBA)...\n";
            denoise_with_oidn(beautybuffer, albedobuffer, normalbuffer, image_width, image_height);
        }

        std::vector<AOVLayer> layers = {
            {"RGBA", beautybuffer},
            {"albedo", albedobuffer},
            {"normal", normalbuffer},
            {"position", positionbuffer},
            {"depth", depthbuffer},
            {"roughness", roughnessbuffer},
            {"metallic", metallicbuffer},
            {"emission", emissionbuffer},
            {"AO", aobuffer},
            {"ID", idbuffer},
            {"UV", uvbuffer}
        };

        std::string target_dcc = "natron";
        auto sort_layers_for_dcc = [&](std::string mode) {
            std::vector<AOVLayer> sorted;

            if (mode == "natron") {
                auto order = std::vector<std::string>{
                    "AO", "ID", "RGBA", "UV",
                    "albedo", "depth", "emission", "metallic",
                    "normal", "position", "roughness"
                };
                for (auto& name : order) {
                    auto it = std::find_if(layers.begin(), layers.end(),
                        [&](const AOVLayer& l) { return l.name == name; });
                    if (it != layers.end()) sorted.push_back(*it);
                }
            }
            return sorted;
            };

        layers = sort_layers_for_dcc(target_dcc);
        write_exr_multilayer("renders/SSRT_Linear_v001.exr", layers, image_width, image_height);
        std::clog << "\nRender ended correctly!\n\n";
    }

private:
    int image_height;
    double pixel_samples_scale;
    vec3 center;
    vec3 pixel00_location;
    vec3 pixel_delta_u, pixel_delta_v;
    vec3 u, v, w;
    vec3 defocus_disk_u, defocus_disk_v;

    void initialize() {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;
        pixel_samples_scale = 1.0 / samples_per_pixel;
        center = lookfrom;

        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2 * h * focus_dist;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        auto viewport_u = viewport_width * u;
        auto viewport_v = viewport_height * -v;

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;
        auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
        pixel00_location = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    vec3 defocus_disk_sample() const {
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    
    inline __forceinline color ray_color(
        const ray& r_in, int max_depth, const hittable& world,
        color* out_albedo = nullptr,
        color* out_normal = nullptr,
        float* out_depth = nullptr,
        color* out_position = nullptr,
        float* out_roughness = nullptr,
        float* out_mettalic = nullptr,
        color* out_emission = nullptr,
        float* out_AO = nullptr,
        color* out_ID = nullptr,
        color* out_uv = nullptr
    ) const noexcept
    {
        ray current = r_in;
        color attenuation(1, 1, 1);
        color accumulated(0, 0, 0);
        bool first_hit = true;

        for (int depth = 0; depth < max_depth; depth++) {
            hit_record rec;
            if (!world.hit(current, interval(0.001, infinity), rec)) {
                accumulated += attenuation * background;
                break;
            }

            if (first_hit) {
                if (out_albedo)
                    *out_albedo = rec.mat->get_base_color(rec.u, rec.v, rec.p);

                if (out_normal)
                    *out_normal = (rec.normal + vec3(1, 1, 1)) * 0.5;

                if (out_depth)
                    *out_depth = (float)(rec.t);

                if (out_position)
                    *out_position = rec.p;

                if (out_roughness)
                    *out_roughness = rec.mat->roughness;

                if (out_mettalic)
                    *out_mettalic = rec.mat->metallic;

                if (out_emission)
                    *out_emission = rec.mat->emitted(rec.u, rec.v, rec.p);

                if (out_AO) {
                    const int ao_samples = 3;
                    int occluded_count = 0;

                    for (int s = 0; s < ao_samples; ++s) {
                        vec3 dir = random_on_hemisphere(rec.normal);
                        ray ao_ray(rec.p + rec.normal * 0.001, dir);
                        hit_record ao_hit;
                        if (world.hit(ao_ray, interval(.001, infinity), ao_hit)) {
                            occluded_count++;
                        }
                    }

                    *out_AO = 1.0f - (float(occluded_count) / ao_samples);
                }

                if (out_ID)
                    *out_ID = rec.object_color;

                if (out_uv)
                    *out_uv = color(rec.u, rec.v, 0);

                first_hit = false;
            }

            ray scattered;
            color emission = rec.mat->emitted(rec.u, rec.v, rec.p);
            color atten;
            if (!rec.mat->scatter(current, rec, atten, scattered)) {
                accumulated += attenuation * emission;
                break;
            }

            accumulated += attenuation * emission;
            attenuation = attenuation * atten;
            current = scattered;
        }

        return accumulated;
    }
};

#endif
