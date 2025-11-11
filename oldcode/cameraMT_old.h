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

#ifdef USE_PACKET_TRACKING
#include "packet.h"
#endif

class camera {
public: 
    //variables
    bool use_denoiser = true;
    double aspect_ratio = 16.0/9.0;
    int image_width = 100;
    int samples_per_pixel = 10;
    int max_depth = 550;
    color background;
    double vfov = 90;
    point3 lookfrom = point3(0, 0, 0);
    point3 lookat = point3(0, 0, 0);
    vec3 vup = vec3(0, 1, 0);
    double defocus_angle = 0;
    double focus_dist = 10;
    double near_plane = .1;
    double far_plane = 555;
    //variables


    void hideCursor() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = FALSE; // cacher le curseur
        SetConsoleCursorInfo(hOut, &cursorInfo);
    }
    void showCursor() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = TRUE; // montrer le curseur
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
        int tileSize = 128; // taille d’une tuile : 32x32 pixels

        initialize();

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

        std::mutex cout_mutex;

        std::vector<std::pair<uint32_t, std::pair<int, int>>> tiles;

        for (int y = 0; y < image_height; y += tileSize) {
            for (int x = 0; x < image_width; x += tileSize) {
                uint32_t morton = morton2D(x / tileSize, y / tileSize);
                tiles.push_back({ morton, {x,y} });
            }
        }

        std::sort(tiles.begin(), tiles.end(), [](auto& a, auto& b) {return a.first < b.first; });


        // --- Boucle principale : envoi des tâches au ThreadPool ---
#pragma omp parallel for schedule(dynamic)
        for (int tile_idx = 0; tile_idx < (int)tiles.size(); ++tile_idx) {
            const auto& [code, xy] = tiles[tile_idx];

            int x = xy.first;
            int y = xy.second;

            uint64_t rng_state = 1337u + y * image_width + x;

            // === à l'intérieur de render, boucle sur les pixels ===
            for (int j = y; j < std::min(y + tileSize, image_height); ++j) {
                for (int i = x; i < std::min(x + tileSize, image_width); ++i) {

                    const int idx = j * image_width + i; // index entier

                    color pixel_beauty(0, 0, 0);
                    color pixel_albedo(0, 0, 0);
                    color pixel_normal(0, 0, 0);
                    color pixel_position(0, 0, 0);
                    float pixel_depth = 0;
                    color pixel_roughness(0, 0, 0);
                    color pixel_metallic(0, 0, 0);
                    color pixel_emission(0, 0, 0);
                    color pixel_ao(0, 0, 0);
                    color pixel_id(0, 0, 0);
                    color pixel_uv(0, 0, 0);

                    const point3 pixel_base = pixel00_location + i * pixel_delta_u + j * pixel_delta_v;

                    for (int sample = 0; sample < samples_per_pixel; ++sample) {
                        float rx = randf(rng_state);
                        float ry = randf(rng_state);

                        const vec3 pixel_sample =
                            pixel_base + (rx - 0.5f) * pixel_delta_u + (ry - 0.5f) * pixel_delta_v;

                        const point3 ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
                        const vec3 ray_direction = pixel_sample - ray_origin;

                        ray r(ray_origin, ray_direction);

                        // out pointers vers variables locales
                        color alb_out, nrm_out, pos_out, emit_out, out_id, out_uv, diffDir_out, diffInd_out, specDir_out, specInd_out;
                        float depth_out = 0.0f, rough_out = 0.0f, metal_out = 0.0f, ao_out = 0.0f;
                        
                        color sample_color = ray_color(r, max_depth, world,
                            &alb_out, &nrm_out, &depth_out, &pos_out, &rough_out, &metal_out, &emit_out, &ao_out, &out_id, &out_uv);

                        
                        // accumulate
                        pixel_beauty += sample_color;
                        pixel_albedo += alb_out;
                        pixel_normal += nrm_out;
                        pixel_position += pos_out;
                        pixel_depth += depth_out;
                        pixel_metallic += color(metal_out, metal_out, metal_out);
                        pixel_roughness += color(rough_out, rough_out, rough_out);
                        pixel_emission += emit_out;
                        pixel_ao += color(ao_out, ao_out, ao_out);
                        pixel_id += out_id;
                        pixel_uv += out_uv;
                    }

                    

                    pixel_beauty *= pixel_samples_scale;
                    pixel_albedo *= pixel_samples_scale;
                    pixel_normal *= pixel_samples_scale;
                    pixel_position *= pixel_samples_scale;
                    pixel_depth *= pixel_samples_scale;
                    pixel_metallic *= pixel_samples_scale;
                    pixel_roughness *= pixel_samples_scale;
                    pixel_emission *= pixel_samples_scale;
                    pixel_ao *= pixel_samples_scale;
                    pixel_id *= pixel_samples_scale;
                    pixel_uv *= pixel_samples_scale;

                    beautybuffer[idx] = pixel_beauty;
                    albedobuffer[idx] = pixel_albedo;
                    normalbuffer[idx] = pixel_normal;
                    positionbuffer[idx] = pixel_position;
                    depthbuffer[idx] = color((pixel_depth - float(near_plane)) / float(far_plane - near_plane), (pixel_depth - float(near_plane)) / float(far_plane - near_plane), (pixel_depth - float(near_plane)) / float(far_plane - near_plane));
                    metallicbuffer[idx] = pixel_metallic;
                    roughnessbuffer[idx] = pixel_roughness;
                    emissionbuffer[idx] = pixel_emission;
                    aobuffer[idx] = pixel_ao;
                    idbuffer[idx] = pixel_id;
                    uvbuffer[idx] = pixel_uv;
                }
            }


#pragma omp critical
            {
                static int tiles_done = 0;
                int done = ++tiles_done;
                if (done % 3 == 0) {
                    double progress = 100.0 * done / ((image_height / tileSize) * (image_width / tileSize));
                    std::clog << "\rProgress: " << (int)progress << "% ";
                }
            }
        }

        // Attendre que toutes les tuiles soient calculées
        pool.wait();
        

        if (use_denoiser) {
            
            denoise_with_oidn(beautybuffer, albedobuffer, normalbuffer, image_width, image_height);

            std::vector<AOVLayer> layers = {
            {"AO", aobuffer},
            {"ID", idbuffer},
            {"RGBA", beautybuffer},
            {"UV", uvbuffer},
            {"albedo", albedobuffer},
            {"depth", depthbuffer},
            {"emission", emissionbuffer},
            {"metallic", metallicbuffer},
            {"normal", normalbuffer},
            {"position", positionbuffer},
            {"roughness", roughnessbuffer},
            };

            std::cout << "\n\n[OIDN] Denoising in progress... Please wait.\n";
            write_exr_multilayer("renders/SSRT_Linear_denoised_v001.exr", layers, image_width, image_height);
            std::clog << "\nRender ended correctly!\n\n";

        }
        else {
            std::vector<AOVLayer> layers = { 
            {"RGBA", beautybuffer},
            {"albedo", albedobuffer},
            {"normal", normalbuffer},
            {"position", positionbuffer},
            {"depth", depthbuffer},
            };
            write_exr_multilayer("renders/SSRT_Linear_v001.exr", layers, image_width, image_height);
            std::clog << "\nRender ended correctly!\n\n";
        }
    }

    

private: 
    //variables
    int    image_height;   
    double pixel_samples_scale;
    point3 center;       
    point3 pixel00_location;    
    vec3   pixel_delta_u;  
    vec3   pixel_delta_v;  
    vec3   u, v, w;
    vec3 defocus_disk_u;
    vec3 defocus_disk_v;
    //variables



    void initialize() {

        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;


        pixel_samples_scale = 1.0 / samples_per_pixel;
        center = lookfrom;

      
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2*h*focus_dist;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        auto viewport_u = viewport_width * u;
        auto viewport_v = viewport_height * -v;

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
        pixel00_location = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    vec3 sample_square() const {
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    point3 defocus_disk_sample() const {
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
        color accumulated(0.0f, 0.0f, 0.0f);
        color throughput(1.0f, 1.0f, 1.0f);
        ray current = r_in;
        


        for (int depth = 0; depth < max_depth; ++depth) {
            hit_record rec;

            if (!world.hit(current, interval(0.001, infinity), rec)) {
                accumulated += throughput * background;
                break;
            }

            const material* mat_ptr = rec.mat.get();
            color emitted = mat_ptr ? mat_ptr->emitted(rec.u, rec.v, rec.p) : color(0, 0, 0);

            // === AOVs: enregistrés uniquement au premier hit ===
            if (depth < 1) {
                if (out_albedo) {
                    *out_albedo = mat_ptr ? mat_ptr->get_base_color(rec.u, rec.v, rec.p) : color(1, 1, 1);
                }
                if (out_normal) {
                    vec3 n = unit_vector(rec.normal);
                    *out_normal = color(-n.x(), n.y(), -n.z());
                }
                if (out_depth) {
                    double dist = (rec.p - lookfrom).length();
                    *out_depth = static_cast<float>(dist);
                }
                if (out_position) {
                    *out_position = rec.p;
                }
                if (out_roughness) {

                    double rough = 0.0;

                    if (dynamic_cast<const lambertian*>(mat_ptr)) {
                        rough = 1.0;
                    }

                    else if (auto m = dynamic_cast<const metal*>(mat_ptr)) {
                        rough = m->roughness;
                    } 
                    
                    else if (auto d = dynamic_cast<const dielectric*>(mat_ptr)) {
                        rough = 0.0;
                    }

                    else if (auto q = dynamic_cast<const coat*>(mat_ptr)) {
                        rough = q->roughness;
                    }

                    *out_roughness = rough;
                }  
                if (out_mettalic) {
                    *out_mettalic = mat_ptr ? mat_ptr->metallic : 0.0f;
                }
                if (out_emission) {
                    *out_emission = emitted;
                }
                if (out_AO) {
                    const int ao_samples = 3;
                    int occluded = 0;

                    for (int s = 0; s < ao_samples; s++) {
                        vec3 dir = random_on_hemisphere(rec.normal);
                        ray ao_ray(rec.p + rec.normal * 0.001, dir);

                        hit_record ao_hit;
                        if (world.hit(ao_ray, interval(.001, 100), ao_hit)) {
                            occluded++;
                        }
                    }

                    float ao_factor = 1.0f - (float(occluded) / ao_samples);
                    *out_AO = ao_factor;
                }
                if (out_ID) {
                    uint64_t seed_r = 0xA511E853C12245ULL * rec.object_id;
                    uint64_t seed_g = 0xB53574A94F8E7ULL * rec.object_id;
                    uint64_t seed_b = 0xC6F25A5341E22LL * rec.object_id;

                    float r = (xorshift64(seed_r) % 256) / 255.0f;
                    float g = (xorshift64(seed_g) % 256) / 255.0f;
                    float b = (xorshift64(seed_b) % 256) / 255.0f;
                    *out_ID = color(r, g, b);
                }
                if (out_uv) {
                    *out_uv = color(rec.u, rec.v, 0.0);
                }
            }

            ray scattered;
            color local_atten;
            if (!mat_ptr || !mat_ptr->scatter(current, rec, local_atten, scattered)) {
                accumulated += throughput * emitted;
                break;
            }

            throughput = throughput * local_atten;
            current = scattered;

        }


        return accumulated;

    }
};
#endif